// bpk decoder (canonical Huffman decoded bit by bit, as zlib's puff does) and
// archive reader.
#include "bpk.h"
#include "bpk_tables.h"

#include <cstring>
#include <fstream>
#include <sstream>

namespace bpk {

using namespace detail;

uint64_t fnv1a64(std::string_view data) {
    uint64_t h = 0xcbf29ce484222325ull;
    for (unsigned char c : data) {
        h ^= c;
        h *= 0x100000001b3ull;
    }
    return h;
}

namespace {

class BitReader {
public:
    explicit BitReader(std::string_view in) : in_(in) {}
    uint32_t bits(int count) {
        uint32_t v = 0;
        for (int i = 0; i < count; ++i) v |= bit() << i;
        return v;
    }
    uint32_t bit() {
        if (pos_ >= in_.size()) {
            error_ = true;
            return 0;
        }
        const uint32_t b = (uint8_t(in_[pos_]) >> nbit_) & 1u;
        if (++nbit_ == 8) {
            nbit_ = 0;
            ++pos_;
        }
        return b;
    }
    bool error() const { return error_; }

private:
    std::string_view in_;
    size_t pos_ = 0;
    int nbit_ = 0;
    bool error_ = false;
};

struct Huffman {
    int count[kMaxBits + 1] = {};
    int symbol[kLitLenSymbols] = {};

    // False if the lengths over-subscribe the code space.
    bool build(const uint8_t* lengths, int n) {
        std::memset(count, 0, sizeof count);
        for (int s = 0; s < n; ++s) count[lengths[s]]++;
        count[0] = 0;
        int left = 1;
        for (int len = 1; len <= kMaxBits; ++len) {
            left = (left << 1) - count[len];
            if (left < 0) return false;
        }
        int offs[kMaxBits + 2] = {};
        for (int len = 1; len <= kMaxBits; ++len) offs[len + 1] = offs[len] + count[len];
        for (int s = 0; s < n; ++s)
            if (lengths[s]) symbol[offs[lengths[s]]++] = s;
        return true;
    }

    int decode(BitReader& br) const {
        int code = 0, first = 0, index = 0;
        for (int len = 1; len <= kMaxBits; ++len) {
            code |= int(br.bit());
            const int c = count[len];
            if (code - c < first) return symbol[index + (code - first)];
            index += c;
            first = (first + c) << 1;
            code <<= 1;
        }
        return -1;
    }
};

uint64_t get_le(std::string_view s, size_t at, int bytes) {
    uint64_t v = 0;
    for (int k = 0; k < bytes; ++k) v |= uint64_t(uint8_t(s[at + size_t(k)])) << (8 * k);
    return v;
}

} // namespace

bool decompress(std::string_view in, size_t expected_size, std::string& out) {
    out.clear();
    out.reserve(expected_size);
    BitReader br(in);
    Huffman lit, dist;
    uint8_t lengths[kLitLenSymbols + kDistSymbols];
    for (bool last = false; !last;) {
        last = br.bits(1) != 0;
        for (uint8_t& l : lengths) l = uint8_t(br.bits(4));
        if (br.error() || !lit.build(lengths, kLitLenSymbols) || !dist.build(lengths + kLitLenSymbols, kDistSymbols))
            return false;
        for (;;) {
            const int sym = lit.decode(br);
            if (sym < 0 || br.error()) return false;
            if (sym < 256) {
                if (out.size() >= expected_size) return false;
                out.push_back(char(sym));
                continue;
            }
            if (sym == 256) break;
            const int lc = sym - 257;
            if (lc >= 29) return false;
            const size_t len = kLenBase[lc] + br.bits(kLenExtra[lc]);
            const int dc = dist.decode(br);
            if (dc < 0 || dc >= kDistSymbols) return false;
            const size_t d = dist_base(dc) + br.bits(int(dist_extra(dc)));
            if (br.error() || d > out.size() || out.size() + len > expected_size) return false;
            const size_t from = out.size() - d;
            for (size_t k = 0; k < len; ++k) out.push_back(out[from + k]);  // may overlap
        }
    }
    return !br.error() && out.size() == expected_size;
}

std::optional<std::vector<Entry>> read_archive(std::string_view file, std::string* error) {
    auto fail = [&](const char* why) -> std::optional<std::vector<Entry>> {
        if (error) *error = why;
        return std::nullopt;
    };
    if (file.size() < 24 || file.substr(0, 8) != "BTPK0001") return fail("not a bpk archive");
    const uint64_t raw_size = get_le(file, 8, 8);
    const uint64_t sum = get_le(file, 16, 8);
    if (raw_size > (uint64_t(1) << 32)) return fail("implausible size");
    std::string raw;
    if (!decompress(file.substr(24), size_t(raw_size), raw)) return fail("corrupt compressed stream");
    if (fnv1a64(raw) != sum) return fail("checksum mismatch");

    size_t at = 0;
    auto need = [&](size_t n) { return at + n <= raw.size(); };
    if (!need(4)) return fail("truncated index");
    const size_t count = size_t(get_le(raw, 0, 4));
    at = 4;
    std::vector<Entry> entries;
    std::vector<size_t> sizes;
    entries.reserve(count);
    for (size_t k = 0; k < count; ++k) {
        if (!need(2)) return fail("truncated index");
        const size_t plen = size_t(get_le(raw, at, 2));
        at += 2;
        if (!need(plen + 4)) return fail("truncated index");
        entries.push_back(Entry{raw.substr(at, plen), {}});
        at += plen;
        sizes.push_back(size_t(get_le(raw, at, 4)));
        at += 4;
    }
    for (size_t k = 0; k < count; ++k) {
        if (!need(sizes[k])) return fail("truncated data");
        entries[k].data = raw.substr(at, sizes[k]);
        at += sizes[k];
    }
    if (at != raw.size()) return fail("trailing data");
    return entries;
}

std::optional<std::vector<Entry>> read_archive_file(const std::string& path, std::string* error) {
    std::ifstream f(path, std::ios::binary);
    if (!f) {
        if (error) *error = "cannot open " + path;
        return std::nullopt;
    }
    std::ostringstream ss;
    ss << f.rdbuf();
    return read_archive(ss.str(), error);
}

} // namespace bpk

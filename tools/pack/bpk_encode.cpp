// bpk encoder: LZ77 (hash chains, lazy matching) + per-block canonical Huffman.
#include "bpk.h"
#include "bpk_tables.h"

#include <algorithm>
#include <functional>
#include <queue>
#include <stdexcept>
#include <utility>

namespace bpk {

using namespace detail;

namespace {

class BitWriter {
public:
    void put(uint32_t value, int count) {  // `count` low bits of value, LSB first
        for (int i = 0; i < count; ++i) put_bit((value >> i) & 1u);
    }
    void put_code(uint32_t code, int length) {  // Huffman code, most significant bit first
        for (int i = length - 1; i >= 0; --i) put_bit((code >> i) & 1u);
    }
    std::string finish() {
        if (nbits_) out_.push_back(char(acc_));
        acc_ = 0;
        nbits_ = 0;
        return std::move(out_);
    }

private:
    void put_bit(uint32_t b) {
        acc_ |= uint8_t(b << nbits_);
        if (++nbits_ == 8) {
            out_.push_back(char(acc_));
            acc_ = 0;
            nbits_ = 0;
        }
    }
    std::string out_;
    uint8_t acc_ = 0;
    int nbits_ = 0;
};

// Code lengths for `freq`, at most `limit` bits: a Huffman tree, with the
// frequencies halved and the tree rebuilt until it fits.
std::vector<uint8_t> code_lengths(std::vector<uint64_t> freq, int limit) {
    const size_t n = freq.size();
    std::vector<uint8_t> lengths(n, 0);
    std::vector<int> used;
    for (size_t s = 0; s < n; ++s)
        if (freq[s]) used.push_back(int(s));
    if (used.empty()) return lengths;
    if (used.size() == 1) {
        lengths[size_t(used[0])] = 1;
        return lengths;
    }
    for (;;) {
        const size_t m = used.size();
        std::vector<int> parent(2 * m - 1, -1);
        using Node = std::pair<uint64_t, int>;
        std::priority_queue<Node, std::vector<Node>, std::greater<Node>> pq;
        for (size_t k = 0; k < m; ++k) pq.push({freq[size_t(used[k])], int(k)});
        int next = int(m);
        while (pq.size() > 1) {
            const Node a = pq.top();
            pq.pop();
            const Node b = pq.top();
            pq.pop();
            parent[size_t(a.second)] = next;
            parent[size_t(b.second)] = next;
            pq.push({a.first + b.first, next++});
        }
        int deepest = 0;
        for (size_t k = 0; k < m; ++k) {
            int depth = 0;
            for (int p = parent[k]; p != -1; p = parent[size_t(p)]) ++depth;
            lengths[size_t(used[k])] = uint8_t(std::min(depth, 255));
            deepest = std::max(deepest, depth);
        }
        if (deepest <= limit) return lengths;
        for (int s : used) freq[size_t(s)] = (freq[size_t(s)] + 1) / 2;
    }
}

// Canonical codes for `lengths` (DEFLATE's assignment).
std::vector<uint32_t> canonical_codes(const std::vector<uint8_t>& lengths) {
    uint32_t count[kMaxBits + 1] = {};
    for (uint8_t l : lengths) count[l]++;
    count[0] = 0;
    uint32_t next[kMaxBits + 2] = {};
    uint32_t code = 0;
    for (int bits = 1; bits <= kMaxBits; ++bits) {
        code = (code + count[bits - 1]) << 1;
        next[bits] = code;
    }
    std::vector<uint32_t> codes(lengths.size(), 0);
    for (size_t s = 0; s < lengths.size(); ++s)
        if (lengths[s]) codes[s] = next[lengths[s]]++;
    return codes;
}

int length_code(uint32_t len) {
    int c = 28;
    while (kLenBase[c] > len) --c;
    return c;
}

int distance_code(uint32_t dist) {
    int c = kDistSymbols - 1;
    while (dist_base(c) > dist) --c;
    return c;
}

struct Token {
    uint32_t value;  // literal byte, or match distance
    uint16_t length;  // 0 for a literal
};

void emit_block(BitWriter& bw, const std::vector<Token>& tokens, bool last) {
    std::vector<uint64_t> fl(kLitLenSymbols, 0), fd(kDistSymbols, 0);
    for (const Token& t : tokens) {
        if (t.length == 0) {
            fl[t.value]++;
        } else {
            fl[size_t(257 + length_code(t.length))]++;
            fd[size_t(distance_code(t.value))]++;
        }
    }
    fl[256]++;
    const auto ll = code_lengths(fl, kMaxBits);
    const auto dl = code_lengths(fd, kMaxBits);
    const auto lc = canonical_codes(ll);
    const auto dc = canonical_codes(dl);
    bw.put(last ? 1u : 0u, 1);
    for (uint8_t l : ll) bw.put(l, 4);
    for (uint8_t l : dl) bw.put(l, 4);
    for (const Token& t : tokens) {
        if (t.length == 0) {
            bw.put_code(lc[t.value], ll[t.value]);
            continue;
        }
        const int c = length_code(t.length);
        bw.put_code(lc[size_t(257 + c)], ll[size_t(257 + c)]);
        bw.put(t.length - kLenBase[c], kLenExtra[c]);
        const int d = distance_code(t.value);
        bw.put_code(dc[size_t(d)], dl[size_t(d)]);
        bw.put(t.value - dist_base(d), int(dist_extra(d)));
    }
    bw.put_code(lc[256], ll[256]);
}

class Matcher {
public:
    explicit Matcher(std::string_view in) : in_(in), head_(size_t(1) << kHashBits, -1), prev_(in.size(), -1) {}

    void insert(size_t p) {
        if (p + 2 >= in_.size()) return;
        const uint32_t h = hash(p);
        prev_[p] = head_[h];
        head_[h] = int32_t(p);
    }

    // Longest earlier match for position p (call before insert(p)).
    uint32_t longest(size_t p, uint32_t& dist) const {
        if (p + kMinMatch > in_.size()) return 0;
        const size_t max_len = std::min<size_t>(kMaxMatch, in_.size() - p);
        size_t best = 0;
        int chain = kMaxChain;
        for (int32_t cand = head_[hash(p)]; cand >= 0 && chain-- > 0; cand = prev_[size_t(cand)]) {
            const size_t d = p - size_t(cand);
            if (d > kWindow) break;
            if (in_[size_t(cand) + best] != in_[p + best]) continue;
            size_t l = 0;
            while (l < max_len && in_[size_t(cand) + l] == in_[p + l]) ++l;
            if (l > best) {
                best = l;
                dist = uint32_t(d);
                if (l == max_len) break;
            }
        }
        return best >= size_t(kMinMatch) ? uint32_t(best) : 0;
    }

private:
    static constexpr int kHashBits = 17;
    static constexpr int kMaxChain = 512;
    uint32_t hash(size_t p) const {
        const uint32_t v = uint32_t(uint8_t(in_[p])) | uint32_t(uint8_t(in_[p + 1])) << 8 |
                           uint32_t(uint8_t(in_[p + 2])) << 16;
        return (v * 2654435761u) >> (32 - kHashBits);
    }
    std::string_view in_;
    std::vector<int32_t> head_;
    std::vector<int32_t> prev_;
};

} // namespace

std::string compress(std::string_view in) {
    if (in.size() > size_t(INT32_MAX)) throw std::length_error("bpk: input too large");
    constexpr size_t kBlockTokens = size_t(1) << 18;
    constexpr uint32_t kNice = 128;  // a match this long is taken without a lazy look
    BitWriter bw;
    Matcher m(in);
    std::vector<Token> tokens;
    tokens.reserve(kBlockTokens);
    auto flush = [&](bool last) {
        emit_block(bw, tokens, last);
        tokens.clear();
    };
    auto push = [&](Token t) {
        tokens.push_back(t);
        if (tokens.size() == kBlockTokens) flush(false);
    };
    const size_t n = in.size();
    size_t i = 0;
    uint32_t prev_len = 0, prev_dist = 0;
    bool have_prev = false;
    while (i < n) {
        uint32_t dist = 0;
        const uint32_t len = (have_prev && prev_len >= kNice) ? 0 : m.longest(i, dist);
        m.insert(i);
        if (have_prev && prev_len >= uint32_t(kMinMatch) && len <= prev_len) {
            push(Token{prev_dist, uint16_t(prev_len)});  // the match at i - 1 wins
            const size_t end = i - 1 + prev_len;
            for (size_t p = i + 1; p < end; ++p) m.insert(p);
            i = end;
            have_prev = false;
            continue;
        }
        if (have_prev) push(Token{uint8_t(in[i - 1]), 0});
        have_prev = true;
        prev_len = len;
        prev_dist = dist;
        ++i;
    }
    if (have_prev) push(Token{uint8_t(in[n - 1]), 0});
    flush(true);
    return bw.finish();
}

namespace {

void put_le(std::string& out, uint64_t v, int bytes) {
    for (int k = 0; k < bytes; ++k) out.push_back(char((v >> (8 * k)) & 0xFF));
}

} // namespace

std::string write_archive(const std::vector<Entry>& entries) {
    std::string raw;
    put_le(raw, entries.size(), 4);
    for (const Entry& e : entries) {
        if (e.path.size() > 0xFFFF || e.data.size() > 0xFFFFFFFFu) throw std::length_error("bpk: entry too large");
        put_le(raw, e.path.size(), 2);
        raw += e.path;
        put_le(raw, e.data.size(), 4);
    }
    for (const Entry& e : entries) raw += e.data;

    std::string file = "BTPK0001";
    put_le(file, raw.size(), 8);
    put_le(file, fnv1a64(raw), 8);
    file += compress(raw);

    std::string error;
    const auto back = read_archive(file, &error);
    if (!back || back->size() != entries.size()) throw std::runtime_error("bpk: archive does not read back: " + error);
    for (size_t k = 0; k < entries.size(); ++k)
        if ((*back)[k].path != entries[k].path || (*back)[k].data != entries[k].data)
            throw std::runtime_error("bpk: archive does not read back: entry " + entries[k].path);
    return file;
}

} // namespace bpk

// The bpk codec and archive (tools/pack/bpk.h) that carries the
// iTerm2-Color-Schemes oracle data: exact round trips over inputs that hit
// every path (empty, incompressible, overlapping runs, matches far beyond
// DEFLATE's 32 KiB window up to the 1 MiB limit), real compression on
// redundant data, and rejection of damaged archives.
#include "check.h"
#include "pack/bpk.h"

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace {

std::string random_bytes(size_t n, uint64_t seed) {
    std::string s(n, '\0');
    for (auto& c : s) {
        seed ^= seed << 13;
        seed ^= seed >> 7;
        seed ^= seed << 17;
        c = char(seed & 0xFF);
    }
    return s;
}

size_t round_trip(const std::string& raw) {
    const std::string packed = bpk::compress(raw);
    std::string back;
    CHECK(bpk::decompress(packed, raw.size(), back));
    CHECK(back == raw);
    // A wrong expected size is refused rather than overrun or under-filled.
    std::string other;
    if (!raw.empty()) CHECK(!bpk::decompress(packed, raw.size() - 1, other));
    CHECK(!bpk::decompress(packed, raw.size() + 1, other));
    return packed.size();
}

void codec() {
    round_trip("");
    round_trip("a");
    round_trip("abc");
    round_trip(std::string(100000, 'z'));  // one long overlapping copy
    std::string all;
    for (int b = 0; b < 256; ++b) all.push_back(char(b));
    round_trip(all + all + all);

    // Incompressible input costs little more than its size.
    const std::string noise = random_bytes(300000, 0x1234);
    const size_t n = round_trip(noise);
    CHECK(n < noise.size() + noise.size() / 50 + 1024);

    // Repeats far apart: DEFLATE's window (32 KiB) would miss them, bpk's
    // (1 MiB) finds them, so the second copy is nearly free.
    for (size_t gap : {size_t(40000), size_t(500000), size_t(1000000)}) {
        const std::string block = random_bytes(gap, gap);
        const size_t packed = round_trip(block + block);
        std::cout << "  " << gap << "-byte block twice: " << packed << " bytes\n";
        CHECK(packed < gap + gap / 20 + 4096);
    }
    // Just past the window the repeat cannot be referenced.
    const std::string far = random_bytes((size_t(1) << 20) + 1000, 99);
    round_trip(far + far.substr(0, 5000));

    // Text-like redundancy (many blocks of tokens, lengths and distances of every class).
    std::string text;
    const char* words[] = {"Ansi", "Color", "Component", "<real>", "</real>", "0.", "\n\t\t", "Blue", "Red", "Green"};
    std::string digits = random_bytes(4000, 7);
    for (size_t i = 0; text.size() < 600000; ++i) {
        text += words[i % 10];
        text += std::to_string(uint8_t(digits[i % digits.size()]));
        if (i % 7 == 0) text += words[(i / 7) % 10];
    }
    const size_t tp = round_trip(text);
    std::cout << "  600 KB of template text: " << tp << " bytes\n";
    CHECK(tp < text.size() / 8);
}

void archive() {
    std::vector<bpk::Entry> entries = {
        {"LICENSE", "MIT"},
        {"schemes/empty.itermcolors", ""},
        {"ghostty/caf\xc3\xa9", random_bytes(70000, 5)},
        {"kitty/x.conf", std::string(5000, '#')},
    };
    const std::string file = bpk::write_archive(entries);
    std::string error;
    auto back = bpk::read_archive(file, &error);
    CHECK(back.has_value());
    if (back) {
        CHECK_EQ(back->size(), entries.size());
        for (size_t i = 0; i < entries.size() && i < back->size(); ++i) {
            CHECK((*back)[i].path == entries[i].path);
            CHECK((*back)[i].data == entries[i].data);
        }
    }
    CHECK(bpk::read_archive(bpk::write_archive({}), &error).has_value());

    // Damage anywhere is detected, never crashes.
    CHECK(!bpk::read_archive("BTPK0002" + file.substr(8), &error));
    CHECK(!bpk::read_archive(file.substr(0, file.size() / 2), &error));
    CHECK(!bpk::read_archive(file.substr(0, 10), &error));
    size_t rejected = 0, tried = 0;
    // (The last byte is skipped: it may end in padding bits nothing reads.)
    for (size_t at = 8; at + 1 < file.size(); at += 97) {
        std::string bad = file;
        bad[at] = char(bad[at] ^ 0x5A);
        ++tried;
        if (!bpk::read_archive(bad, &error)) ++rejected;
    }
    CHECK_EQ(rejected, tried);
}

} // namespace

int main() {
    codec();
    archive();
    return ::brotest::finish("test_pack");
}

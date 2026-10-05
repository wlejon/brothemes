#pragma once
// bpk: the compact archive the iTerm2-Color-Schemes test oracle ships as
// (tests/data/iterm2-color-schemes.bpk), and its codec.
//
// The upstream collection is ~2,900 small text files (one per scheme per
// format), 6.4 MB of content and ~9.7 MB on disk. They are highly redundant:
// every file of a format repeats one template, and colours recur across
// schemes. A bpk stores them as one solid stream compressed with LZ77 over a
// 1 MiB window plus canonical Huffman coding (DEFLATE's structure with a
// window large enough to reach across files), so the whole collection is a
// single ~0.5 MB file that decodes in well under a second.
//
// File layout (little-endian):
//   "BTPK0001"  magic
//   u64         size of the raw stream
//   u64         FNV-1a 64 of the raw stream
//   ...         compressed raw stream
// Raw stream: u32 entry count, then per entry u16 path length, path bytes
// (UTF-8, '/' separated), u32 data size; then every entry's data in order.
//
// Compressed stream: blocks, each = 1 bit "last block", 286 x 4-bit
// literal/length code lengths, 40 x 4-bit distance code lengths, then
// Huffman-coded symbols ending with 256. Symbols 0-255 are literals and
// 257-285 lengths 3-258 (DEFLATE's table); distance codes 0-39 cover
// 1 .. 2^20 (DEFLATE's scheme extended by ten codes). Huffman codes are
// canonical, at most 15 bits, packed most significant bit first into an
// LSB-first bit stream, as in DEFLATE.

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace bpk {

struct Entry {
    std::string path;
    std::string data;
};

// ---- codec (raw bytes <-> compressed stream) --------------------------------
std::string compress(std::string_view raw);
// False on a malformed stream or when the output would exceed `expected_size`.
bool decompress(std::string_view compressed, size_t expected_size, std::string& out);

uint64_t fnv1a64(std::string_view data);

// ---- archive -------------------------------------------------------------------
// The whole .bpk file for `entries` (compressed; decoded and compared before
// returning, so a codec bug cannot produce an archive that reads back wrong).
std::string write_archive(const std::vector<Entry>& entries);
// Parses a .bpk file's bytes; std::nullopt (message in *error) when it is not
// a valid archive or fails its checksum.
std::optional<std::vector<Entry>> read_archive(std::string_view file, std::string* error = nullptr);
std::optional<std::vector<Entry>> read_archive_file(const std::string& path, std::string* error = nullptr);

} // namespace bpk

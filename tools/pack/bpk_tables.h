#pragma once
// Symbol tables shared by the bpk encoder and decoder (see bpk.h).

#include <cstdint>

namespace bpk::detail {

constexpr int kLitLenSymbols = 286;  // 0-255 literals, 256 end of block, 257-285 lengths
constexpr int kDistSymbols = 40;
constexpr int kMaxBits = 15;
constexpr int kMinMatch = 3;
constexpr int kMaxMatch = 258;
constexpr uint32_t kWindow = uint32_t(1) << 20;

// DEFLATE's length codes (symbols 257 + i).
constexpr uint16_t kLenBase[29] = {3,  4,  5,  6,  7,  8,  9,  10, 11,  13,  15,  17,  19,  23, 27,
                                   31, 35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258};
constexpr uint8_t kLenExtra[29] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2,
                                   2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0};

// Distance code c: codes 0-3 are 1-4; from 4 on, c/2 - 1 extra bits over a
// base of (2 + (c & 1)) << extra, plus one. DEFLATE stops at code 29
// (32 KiB); codes 30-39 continue the same pattern to 1 MiB.
constexpr uint32_t dist_extra(int c) { return c < 4 ? 0u : uint32_t(c / 2 - 1); }
constexpr uint32_t dist_base(int c) { return c < 4 ? uint32_t(c + 1) : ((uint32_t(2 + (c & 1)) << dist_extra(c)) + 1); }

static_assert(dist_base(29) == 24577, "DEFLATE's last distance code");
static_assert(dist_base(kDistSymbols - 1) + (uint32_t(1) << dist_extra(kDistSymbols - 1)) - 1 == kWindow,
              "the distance codes cover exactly the window");

} // namespace bpk::detail

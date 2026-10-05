#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace bro::themes {

struct ColorF;

// 8-bit per channel RGBA color.
struct Color {
    uint8_t r{0};
    uint8_t g{0};
    uint8_t b{0};
    uint8_t a{255};

    constexpr Color() noexcept = default;
    constexpr Color(uint8_t r_, uint8_t g_, uint8_t b_, uint8_t a_ = 255) noexcept
        : r(r_), g(g_), b(b_), a(a_) {}

    static constexpr Color from_rgb(uint8_t r, uint8_t g, uint8_t b) noexcept {
        return Color(r, g, b, 255);
    }

    static constexpr Color from_rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a) noexcept {
        return Color(r, g, b, a);
    }

    static constexpr Color from_u32_rgba(uint32_t rgba) noexcept {
        return Color(
            static_cast<uint8_t>((rgba >> 24) & 0xFF),
            static_cast<uint8_t>((rgba >> 16) & 0xFF),
            static_cast<uint8_t>((rgba >> 8) & 0xFF),
            static_cast<uint8_t>(rgba & 0xFF)
        );
    }

    static constexpr Color from_u32_argb(uint32_t argb) noexcept {
        return Color(
            static_cast<uint8_t>((argb >> 16) & 0xFF),
            static_cast<uint8_t>((argb >> 8) & 0xFF),
            static_cast<uint8_t>(argb & 0xFF),
            static_cast<uint8_t>((argb >> 24) & 0xFF)
        );
    }

    constexpr uint32_t to_u32_rgba() const noexcept {
        return (static_cast<uint32_t>(r) << 24) |
               (static_cast<uint32_t>(g) << 16) |
               (static_cast<uint32_t>(b) << 8) |
               static_cast<uint32_t>(a);
    }

    constexpr uint32_t to_u32_argb() const noexcept {
        return (static_cast<uint32_t>(a) << 24) |
               (static_cast<uint32_t>(r) << 16) |
               (static_cast<uint32_t>(g) << 8) |
               static_cast<uint32_t>(b);
    }

    constexpr bool operator==(const Color& other) const noexcept = default;
};

// Floating point RGBA color with channels in [0.0, 1.0].
struct ColorF {
    float r{0.0f};
    float g{0.0f};
    float b{0.0f};
    float a{1.0f};

    constexpr ColorF() noexcept = default;
    constexpr ColorF(float r_, float g_, float b_, float a_ = 1.0f) noexcept
        : r(r_), g(g_), b(b_), a(a_) {}

    constexpr bool operator==(const ColorF& other) const noexcept = default;
};

// Conversions between Color (8-bit) and ColorF (float)
Color to_color(const ColorF& cf) noexcept;
ColorF to_color_f(const Color& c) noexcept;

enum class HexFormat {
    LowerRgb,   // #rrggbb
    UpperRgb,   // #RRGGBB
    LowerRgba,  // #rrggbbaa
    UpperRgba   // #RRGGBBAA
};

// Parsing functions
std::optional<Color> parse_hex(std::string_view str) noexcept;
std::optional<Color> parse_named_ansi(std::string_view name) noexcept;
std::optional<Color> parse_color(std::string_view str) noexcept;

// Formatting functions
std::string to_hex(Color c, HexFormat fmt = HexFormat::LowerRgb);
std::string to_rgb_string(Color c);
std::string to_rgba_string(Color c);

} // namespace bro::themes

#include <ostream>

namespace bro::themes {

inline std::ostream& operator<<(std::ostream& os, const Color& c) {
    return os << to_hex(c);
}

inline std::ostream& operator<<(std::ostream& os, const ColorF& c) {
    return os << "ColorF(" << c.r << ", " << c.g << ", " << c.b << ", " << c.a << ")";
}

} // namespace bro::themes


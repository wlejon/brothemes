#include <brothemes/color.h>
#include <algorithm>
#include <cctype>
#include <charconv>
#include <cmath>
#include <cstdio>
#include <string>

namespace bro::themes {

namespace {

int parse_hex_digit(char c) noexcept {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

std::string_view trim_sv(std::string_view sv) noexcept {
    while (!sv.empty() && std::isspace(static_cast<unsigned char>(sv.front()))) {
        sv.remove_prefix(1);
    }
    while (!sv.empty() && std::isspace(static_cast<unsigned char>(sv.back()))) {
        sv.remove_suffix(1);
    }
    return sv;
}

bool equals_ignore_case(std::string_view a, std::string_view b) noexcept {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(a[i])) !=
            std::tolower(static_cast<unsigned char>(b[i]))) {
            return false;
        }
    }
    return true;
}

float hsl_to_rgb_component(float p, float q, float t) noexcept {
    if (t < 0.0f) t += 1.0f;
    if (t > 1.0f) t -= 1.0f;
    if (t < 1.0f / 6.0f) return p + (q - p) * 6.0f * t;
    if (t < 1.0f / 2.0f) return q;
    if (t < 2.0f / 3.0f) return p + (q - p) * (2.0f / 3.0f - t) * 6.0f;
    return p;
}

} // namespace

Color to_color(const ColorF& cf) noexcept {
    auto clamp_channel = [](float v) -> uint8_t {
        float scaled = std::round(std::clamp(v, 0.0f, 1.0f) * 255.0f);
        return static_cast<uint8_t>(scaled);
    };
    return Color(
        clamp_channel(cf.r),
        clamp_channel(cf.g),
        clamp_channel(cf.b),
        clamp_channel(cf.a)
    );
}

ColorF to_color_f(const Color& c) noexcept {
    constexpr float inv = 1.0f / 255.0f;
    return ColorF(
        static_cast<float>(c.r) * inv,
        static_cast<float>(c.g) * inv,
        static_cast<float>(c.b) * inv,
        static_cast<float>(c.a) * inv
    );
}

std::optional<Color> parse_hex(std::string_view str) noexcept {
    str = trim_sv(str);
    if (str.starts_with('#')) {
        str.remove_prefix(1);
    } else if (str.starts_with("0x") || str.starts_with("0X")) {
        str.remove_prefix(2);
    }

    if (str.length() == 3) { // rgb
        int r = parse_hex_digit(str[0]);
        int g = parse_hex_digit(str[1]);
        int b = parse_hex_digit(str[2]);
        if (r < 0 || g < 0 || b < 0) return std::nullopt;
        return Color(
            static_cast<uint8_t>(r * 17),
            static_cast<uint8_t>(g * 17),
            static_cast<uint8_t>(b * 17),
            255
        );
    } else if (str.length() == 4) { // rgba
        int r = parse_hex_digit(str[0]);
        int g = parse_hex_digit(str[1]);
        int b = parse_hex_digit(str[2]);
        int a = parse_hex_digit(str[3]);
        if (r < 0 || g < 0 || b < 0 || a < 0) return std::nullopt;
        return Color(
            static_cast<uint8_t>(r * 17),
            static_cast<uint8_t>(g * 17),
            static_cast<uint8_t>(b * 17),
            static_cast<uint8_t>(a * 17)
        );
    } else if (str.length() == 6) { // rrggbb
        int r0 = parse_hex_digit(str[0]), r1 = parse_hex_digit(str[1]);
        int g0 = parse_hex_digit(str[2]), g1 = parse_hex_digit(str[3]);
        int b0 = parse_hex_digit(str[4]), b1 = parse_hex_digit(str[5]);
        if (r0 < 0 || r1 < 0 || g0 < 0 || g1 < 0 || b0 < 0 || b1 < 0) return std::nullopt;
        return Color(
            static_cast<uint8_t>((r0 << 4) | r1),
            static_cast<uint8_t>((g0 << 4) | g1),
            static_cast<uint8_t>((b0 << 4) | b1),
            255
        );
    } else if (str.length() == 8) { // rrggbbaa
        int r0 = parse_hex_digit(str[0]), r1 = parse_hex_digit(str[1]);
        int g0 = parse_hex_digit(str[2]), g1 = parse_hex_digit(str[3]);
        int b0 = parse_hex_digit(str[4]), b1 = parse_hex_digit(str[5]);
        int a0 = parse_hex_digit(str[6]), a1 = parse_hex_digit(str[7]);
        if (r0 < 0 || r1 < 0 || g0 < 0 || g1 < 0 || b0 < 0 || b1 < 0 || a0 < 0 || a1 < 0) {
            return std::nullopt;
        }
        return Color(
            static_cast<uint8_t>((r0 << 4) | r1),
            static_cast<uint8_t>((g0 << 4) | g1),
            static_cast<uint8_t>((b0 << 4) | b1),
            static_cast<uint8_t>((a0 << 4) | a1)
        );
    }

    return std::nullopt;
}

std::optional<Color> parse_named_ansi(std::string_view name) noexcept {
    name = trim_sv(name);

    struct NamedColor {
        std::string_view name;
        Color color;
    };

    static constexpr NamedColor table[] = {
        // Standard ANSI 16 colors (standard VGA / XTerm defaults)
        {"black",          Color(0, 0, 0, 255)},
        {"red",            Color(205, 0, 0, 255)},
        {"green",          Color(0, 205, 0, 255)},
        {"yellow",         Color(205, 205, 0, 255)},
        {"blue",           Color(0, 0, 238, 255)},
        {"magenta",        Color(205, 0, 205, 255)},
        {"purple",         Color(205, 0, 205, 255)},
        {"cyan",           Color(0, 205, 205, 255)},
        {"white",          Color(229, 229, 229, 255)},

        {"bright_black",   Color(127, 127, 127, 255)},
        {"brightblack",    Color(127, 127, 127, 255)},
        {"gray",           Color(127, 127, 127, 255)},
        {"grey",           Color(127, 127, 127, 255)},
        {"dark_gray",      Color(85, 85, 85, 255)},
        {"darkgray",       Color(85, 85, 85, 255)},
        {"bright_red",     Color(255, 0, 0, 255)},
        {"brightred",      Color(255, 0, 0, 255)},
        {"bright_green",   Color(0, 255, 0, 255)},
        {"brightgreen",    Color(0, 255, 0, 255)},
        {"bright_yellow",  Color(255, 255, 0, 255)},
        {"brightyellow",   Color(255, 255, 0, 255)},
        {"bright_blue",    Color(92, 92, 255, 255)},
        {"brightblue",     Color(92, 92, 255, 255)},
        {"bright_magenta", Color(255, 0, 255, 255)},
        {"brightmagenta",  Color(255, 0, 255, 255)},
        {"bright_purple",  Color(255, 0, 255, 255)},
        {"brightpurple",   Color(255, 0, 255, 255)},
        {"bright_cyan",    Color(0, 255, 255, 255)},
        {"brightcyan",     Color(0, 255, 255, 255)},
        {"bright_white",   Color(255, 255, 255, 255)},
        {"brightwhite",    Color(255, 255, 255, 255)},
    };

    for (const auto& item : table) {
        if (equals_ignore_case(name, item.name)) {
            return item.color;
        }
    }

    return std::nullopt;
}

namespace {

std::optional<float> parse_float_val(std::string_view sv) noexcept {
    sv = trim_sv(sv);
    if (sv.empty()) return std::nullopt;

    bool is_percent = false;
    if (sv.back() == '%') {
        is_percent = true;
        sv.remove_suffix(1);
        sv = trim_sv(sv);
    }

    std::string s(sv);
    char* end = nullptr;
    float val = std::strtof(s.c_str(), &end);
    if (end != s.c_str() + s.size()) {
        return std::nullopt;
    }
    if (is_percent) {
        val = val / 100.0f;
    }
    return val;
}

std::optional<Color> parse_rgb_function(std::string_view str) noexcept {
    str = trim_sv(str);
    bool is_rgba = false;
    if (str.starts_with("rgba(") || str.starts_with("RGBA(")) {
        is_rgba = true;
        str.remove_prefix(5);
    } else if (str.starts_with("rgb(") || str.starts_with("RGB(")) {
        str.remove_prefix(4);
    } else {
        return std::nullopt;
    }

    if (str.empty() || str.back() != ')') return std::nullopt;
    str.remove_suffix(1);

    // Split by comma or whitespace/slash
    float components[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    int count = 0;

    size_t start = 0;
    while (start < str.size() && count < 4) {
        size_t next = str.find_first_of(",/ \t", start);
        std::string_view token = (next == std::string_view::npos) ? str.substr(start) : str.substr(start, next - start);
        token = trim_sv(token);
        if (!token.empty()) {
            auto val = parse_float_val(token);
            if (!val.has_value()) return std::nullopt;

            if (count < 3) {
                // R, G, B
                if (token.back() == '%') {
                    components[count] = std::clamp(*val * 255.0f, 0.0f, 255.0f);
                } else {
                    components[count] = std::clamp(*val, 0.0f, 255.0f);
                }
            } else {
                // Alpha
                if (token.back() == '%') {
                    components[count] = std::clamp(*val, 0.0f, 1.0f);
                } else if (*val > 1.0f && *val <= 255.0f) {
                    components[count] = std::clamp(*val / 255.0f, 0.0f, 1.0f);
                } else {
                    components[count] = std::clamp(*val, 0.0f, 1.0f);
                }
            }
            count++;
        }
        if (next == std::string_view::npos) break;
        start = next + 1;
    }

    if (count < 3) return std::nullopt;
    if (is_rgba && count < 4) {
        // default alpha = 1.0f
    }

    return Color(
        static_cast<uint8_t>(std::round(components[0])),
        static_cast<uint8_t>(std::round(components[1])),
        static_cast<uint8_t>(std::round(components[2])),
        static_cast<uint8_t>(std::round(components[3] * 255.0f))
    );
}

std::optional<Color> parse_hsl_function(std::string_view str) noexcept {
    str = trim_sv(str);
    if (str.starts_with("hsla(") || str.starts_with("HSLA(")) {
        str.remove_prefix(5);
    } else if (str.starts_with("hsl(") || str.starts_with("HSL(")) {
        str.remove_prefix(4);
    } else {
        return std::nullopt;
    }


    if (str.empty() || str.back() != ')') return std::nullopt;
    str.remove_suffix(1);

    float components[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    int count = 0;

    size_t start = 0;
    while (start < str.size() && count < 4) {
        size_t next = str.find_first_of(",/ \t", start);
        std::string_view token = (next == std::string_view::npos) ? str.substr(start) : str.substr(start, next - start);
        token = trim_sv(token);
        if (!token.empty()) {
            auto val = parse_float_val(token);
            if (!val.has_value()) return std::nullopt;

            if (count == 0) {
                // Hue in degrees
                float h = std::fmod(*val, 360.0f);
                if (h < 0.0f) h += 360.0f;
                components[0] = h / 360.0f;
            } else if (count == 1 || count == 2) {
                // Saturation or Lightness in [0, 1]
                components[count] = std::clamp(*val, 0.0f, 1.0f);
            } else {
                // Alpha in [0, 1]
                components[count] = std::clamp(*val, 0.0f, 1.0f);
            }
            count++;
        }
        if (next == std::string_view::npos) break;
        start = next + 1;
    }

    if (count < 3) return std::nullopt;

    float h = components[0];
    float s = components[1];
    float l = components[2];
    float a = components[3];

    float r = l;
    float g = l;
    float b = l;

    if (s > 1e-6f) {
        float q = l < 0.5f ? l * (1.0f + s) : l + s - l * s;
        float p = 2.0f * l - q;
        r = hsl_to_rgb_component(p, q, h + 1.0f / 3.0f);
        g = hsl_to_rgb_component(p, q, h);
        b = hsl_to_rgb_component(p, q, h - 1.0f / 3.0f);
    }

    return Color(
        static_cast<uint8_t>(std::round(std::clamp(r, 0.0f, 1.0f) * 255.0f)),
        static_cast<uint8_t>(std::round(std::clamp(g, 0.0f, 1.0f) * 255.0f)),
        static_cast<uint8_t>(std::round(std::clamp(b, 0.0f, 1.0f) * 255.0f)),
        static_cast<uint8_t>(std::round(std::clamp(a, 0.0f, 1.0f) * 255.0f))
    );
}

} // namespace

std::optional<Color> parse_color(std::string_view str) noexcept {
    str = trim_sv(str);
    if (str.empty()) return std::nullopt;

    if (str.front() == '#' || str.starts_with("0x") || str.starts_with("0X")) {
        return parse_hex(str);
    }

    // Try bare hex if length is 3, 4, 6, 8 and all hex digits
    if (str.length() == 3 || str.length() == 4 || str.length() == 6 || str.length() == 8) {
        bool all_hex = true;
        for (char c : str) {
            if (parse_hex_digit(c) < 0) {
                all_hex = false;
                break;
            }
        }
        if (all_hex) {
            auto h = parse_hex(str);
            if (h.has_value()) return h;
        }
    }

    if (str.starts_with("rgb(") || str.starts_with("rgba(") ||
        str.starts_with("RGB(") || str.starts_with("RGBA(")) {
        return parse_rgb_function(str);
    }

    if (str.starts_with("hsl(") || str.starts_with("hsla(") ||
        str.starts_with("HSL(") || str.starts_with("HSLA(")) {
        return parse_hsl_function(str);
    }

    return parse_named_ansi(str);
}

std::string to_hex(Color c, HexFormat fmt) {
    char buf[10];
    switch (fmt) {
        case HexFormat::LowerRgb:
            std::snprintf(buf, sizeof(buf), "#%02x%02x%02x", c.r, c.g, c.b);
            break;
        case HexFormat::UpperRgb:
            std::snprintf(buf, sizeof(buf), "#%02X%02X%02X", c.r, c.g, c.b);
            break;
        case HexFormat::LowerRgba:
            std::snprintf(buf, sizeof(buf), "#%02x%02x%02x%02x", c.r, c.g, c.b, c.a);
            break;
        case HexFormat::UpperRgba:
            std::snprintf(buf, sizeof(buf), "#%02X%02X%02X%02X", c.r, c.g, c.b, c.a);
            break;
    }
    return std::string(buf);
}

std::string to_rgb_string(Color c) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "rgb(%u, %u, %u)", c.r, c.g, c.b);
    return std::string(buf);
}

std::string to_rgba_string(Color c) {
    char buf[48];
    if (c.a == 255) {
        std::snprintf(buf, sizeof(buf), "rgba(%u, %u, %u, 1)", c.r, c.g, c.b);
    } else {
        float alpha = static_cast<float>(c.a) / 255.0f;
        std::snprintf(buf, sizeof(buf), "rgba(%u, %u, %u, %.3f)", c.r, c.g, c.b, alpha);
    }
    return std::string(buf);
}

} // namespace bro::themes

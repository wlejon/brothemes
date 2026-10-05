#pragma once

#include <brothemes/color.h>
#include <array>
#include <optional>
#include <string>
#include <string_view>

namespace bro::themes {

enum class AnsiColor : uint8_t {
    Black = 0,
    Red = 1,
    Green = 2,
    Yellow = 3,
    Blue = 4,
    Magenta = 5,
    Cyan = 6,
    White = 7,
    BrightBlack = 8,
    BrightRed = 9,
    BrightGreen = 10,
    BrightYellow = 11,
    BrightBlue = 12,
    BrightMagenta = 13,
    BrightCyan = 14,
    BrightWhite = 15
};

struct AnsiPalette {
    std::array<Color, 16> colors{
        Color(0, 0, 0),         // 0: black
        Color(205, 0, 0),       // 1: red
        Color(0, 205, 0),       // 2: green
        Color(205, 205, 0),     // 3: yellow
        Color(0, 0, 238),       // 4: blue
        Color(205, 0, 205),     // 5: magenta
        Color(0, 205, 205),     // 6: cyan
        Color(229, 229, 229),   // 7: white
        Color(127, 127, 127),   // 8: bright black
        Color(255, 0, 0),       // 9: bright red
        Color(0, 255, 0),       // 10: bright green
        Color(255, 255, 0),     // 11: bright yellow
        Color(92, 92, 255),     // 12: bright blue
        Color(255, 0, 255),     // 13: bright magenta
        Color(0, 255, 255),     // 14: bright cyan
        Color(255, 255, 255)    // 15: bright white
    };

    constexpr AnsiPalette() noexcept = default;

    constexpr Color& operator[](size_t index) noexcept {
        return colors[index % 16];
    }
    constexpr const Color& operator[](size_t index) const noexcept {
        return colors[index % 16];
    }

    constexpr Color& operator[](AnsiColor color) noexcept {
        return colors[static_cast<size_t>(color) % 16];
    }
    constexpr const Color& operator[](AnsiColor color) const noexcept {
        return colors[static_cast<size_t>(color) % 16];
    }

    // Named accessors
    constexpr Color& black() noexcept { return colors[0]; }
    constexpr const Color& black() const noexcept { return colors[0]; }
    constexpr Color& red() noexcept { return colors[1]; }
    constexpr const Color& red() const noexcept { return colors[1]; }
    constexpr Color& green() noexcept { return colors[2]; }
    constexpr const Color& green() const noexcept { return colors[2]; }
    constexpr Color& yellow() noexcept { return colors[3]; }
    constexpr const Color& yellow() const noexcept { return colors[3]; }
    constexpr Color& blue() noexcept { return colors[4]; }
    constexpr const Color& blue() const noexcept { return colors[4]; }
    constexpr Color& magenta() noexcept { return colors[5]; }
    constexpr const Color& magenta() const noexcept { return colors[5]; }
    constexpr Color& cyan() noexcept { return colors[6]; }
    constexpr const Color& cyan() const noexcept { return colors[6]; }
    constexpr Color& white() noexcept { return colors[7]; }
    constexpr const Color& white() const noexcept { return colors[7]; }

    constexpr Color& bright_black() noexcept { return colors[8]; }
    constexpr const Color& bright_black() const noexcept { return colors[8]; }
    constexpr Color& bright_red() noexcept { return colors[9]; }
    constexpr const Color& bright_red() const noexcept { return colors[9]; }
    constexpr Color& bright_green() noexcept { return colors[10]; }
    constexpr const Color& bright_green() const noexcept { return colors[10]; }
    constexpr Color& bright_yellow() noexcept { return colors[11]; }
    constexpr const Color& bright_yellow() const noexcept { return colors[11]; }
    constexpr Color& bright_blue() noexcept { return colors[12]; }
    constexpr const Color& bright_blue() const noexcept { return colors[12]; }
    constexpr Color& bright_magenta() noexcept { return colors[13]; }
    constexpr const Color& bright_magenta() const noexcept { return colors[13]; }
    constexpr Color& bright_cyan() noexcept { return colors[14]; }
    constexpr const Color& bright_cyan() const noexcept { return colors[14]; }
    constexpr Color& bright_white() noexcept { return colors[15]; }
    constexpr const Color& bright_white() const noexcept { return colors[15]; }

    constexpr bool operator==(const AnsiPalette& other) const noexcept = default;

    static AnsiPalette standard_vga() noexcept {
        return AnsiPalette();
    }
};

struct UiColors {
    Color background{0, 0, 0};
    Color foreground{229, 229, 229};

    std::optional<Color> cursor;
    std::optional<Color> cursor_text;
    std::optional<Color> selection_background;
    std::optional<Color> selection_foreground;
    std::optional<Color> border;
    std::optional<Color> status_bar;
    std::optional<Color> line_number;
    std::optional<Color> active_line;
    std::optional<Color> match_highlight;
    std::optional<Color> search_match;
    std::optional<Color> tab_bar;
    std::optional<Color> split_divider;

    Color get_cursor() const noexcept {
        return cursor.value_or(foreground);
    }
    Color get_cursor_text() const noexcept {
        return cursor_text.value_or(background);
    }
    Color get_selection_background() const noexcept {
        if (selection_background.has_value()) return *selection_background;
        // Sensible fallback: blend or bright black
        return Color(70, 70, 70);
    }
    Color get_selection_foreground() const noexcept {
        return selection_foreground.value_or(foreground);
    }

    bool operator==(const UiColors& other) const noexcept = default;
};

struct SyntaxColors {
    std::optional<Color> comment;
    std::optional<Color> string;
    std::optional<Color> keyword;
    std::optional<Color> number;
    std::optional<Color> function;
    std::optional<Color> type;
    std::optional<Color> variable;
    std::optional<Color> constant;
    std::optional<Color> operator_color;
    std::optional<Color> punctuation;
    std::optional<Color> error;
    std::optional<Color> warning;
    std::optional<Color> info;
    std::optional<Color> hint;
    std::optional<Color> markup_heading;
    std::optional<Color> markup_link;
    std::optional<Color> markup_code;

    bool operator==(const SyntaxColors& other) const noexcept = default;
};

struct ThemeMetadata {
    std::string name;
    std::string author;
    std::string description;
    std::optional<bool> is_dark;

    bool operator==(const ThemeMetadata& other) const noexcept = default;
};

struct Theme {
    ThemeMetadata metadata;
    UiColors ui;
    AnsiPalette ansi;
    SyntaxColors syntax;

    bool is_dark() const noexcept;
    void normalize();

    bool operator==(const Theme& other) const noexcept = default;
};

} // namespace bro::themes

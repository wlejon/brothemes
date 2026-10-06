#include "themes_convert.h"
#include "object_builder.h"

#include <algorithm>
#include <cctype>
#include <cmath>

namespace bro::themes::api {

std::optional<Color> parseColorValue(Value val) {
    if (ev::isString(val)) {
        std::string s = ev::toUtf8(val);
        return parse_color(s);
    }
    if (ev::isNumber(val)) {
        uint32_t u = static_cast<uint32_t>(ev::toDouble(val));
        return Color::from_u32_rgba(u);
    }
    if (ev::isObject(val)) {
        ev::Persistent vP(val);
        Value hexVal = ev::getProperty(vP.get(), "hex");
        if (ev::isString(hexVal)) {
            std::string s = ev::toUtf8(hexVal);
            auto c = parse_color(s);
            if (c) return c;
        }

        Value rVal = ev::getProperty(vP.get(), "r");
        Value gVal = ev::getProperty(vP.get(), "g");
        Value bVal = ev::getProperty(vP.get(), "b");
        if (ev::isNumber(rVal) && ev::isNumber(gVal) && ev::isNumber(bVal)) {
            double rd = ev::toDouble(rVal);
            double gd = ev::toDouble(gVal);
            double bd = ev::toDouble(bVal);

            bool hasFraction = (rd != std::floor(rd)) || (gd != std::floor(gd)) || (bd != std::floor(bd));
            if (hasFraction && rd <= 1.0 && gd <= 1.0 && bd <= 1.0 && rd >= 0.0 && gd >= 0.0 && bd >= 0.0) {
                rd *= 255.0;
                gd *= 255.0;
                bd *= 255.0;
            }

            uint8_t r = static_cast<uint8_t>(std::clamp(std::round(rd), 0.0, 255.0));
            uint8_t g = static_cast<uint8_t>(std::clamp(std::round(gd), 0.0, 255.0));
            uint8_t b = static_cast<uint8_t>(std::clamp(std::round(bd), 0.0, 255.0));
            uint8_t a = 255;

            Value aVal = ev::getProperty(vP.get(), "a");
            if (ev::isNumber(aVal)) {
                double ad = ev::toDouble(aVal);
                if (ad >= 0.0 && ad <= 1.0) {
                    a = static_cast<uint8_t>(std::clamp(std::round(ad * 255.0), 0.0, 255.0));
                } else {
                    a = static_cast<uint8_t>(std::clamp(std::round(ad), 0.0, 255.0));
                }
            }
            return Color(r, g, b, a);
        }
    }
    return std::nullopt;
}

Value colorToJs(Color c, bool asHex) {
    if (asHex) {
        HexFormat fmt = (c.a == 255) ? HexFormat::LowerRgb : HexFormat::LowerRgba;
        return ev::fromUtf8(to_hex(c, fmt));
    }

    ObjectBuilder b;
    b.set("r", static_cast<double>(c.r));
    b.set("g", static_cast<double>(c.g));
    b.set("b", static_cast<double>(c.b));
    b.set("a", static_cast<double>(c.a));
    HexFormat fmt = (c.a == 255) ? HexFormat::LowerRgb : HexFormat::LowerRgba;
    b.set("hex", to_hex(c, fmt));
    return b.build();
}

Value themeToJs(const Theme& theme, bool asHex) {
    ObjectBuilder root;

    root.set("name", theme.metadata.name);
    root.set("author", theme.metadata.author);
    root.set("description", theme.metadata.description);
    root.set("isDark", theme.is_dark());

    // metadata
    {
        ObjectBuilder meta;
        meta.set("name", theme.metadata.name);
        meta.set("author", theme.metadata.author);
        meta.set("description", theme.metadata.description);
        meta.set("isDark", theme.is_dark());
        ev::Persistent metaVal(meta.build());
        root.set("metadata", metaVal.get());
    }

    // ui
    {
        ObjectBuilder ui;
        ev::Persistent bg(colorToJs(theme.ui.background, asHex));
        ui.set("background", bg.get());

        ev::Persistent fg(colorToJs(theme.ui.foreground, asHex));
        ui.set("foreground", fg.get());

        auto setOptColor = [&](std::string_view name, const std::optional<Color>& opt) {
            if (opt.has_value()) {
                ev::Persistent cv(colorToJs(*opt, asHex));
                ui.set(name, cv.get());
            }
        };

        setOptColor("cursor", theme.ui.cursor);
        setOptColor("cursorText", theme.ui.cursor_text);
        setOptColor("selectionBackground", theme.ui.selection_background);
        setOptColor("selectionForeground", theme.ui.selection_foreground);
        setOptColor("border", theme.ui.border);
        setOptColor("statusBar", theme.ui.status_bar);
        setOptColor("lineNumber", theme.ui.line_number);
        setOptColor("activeLine", theme.ui.active_line);
        setOptColor("matchHighlight", theme.ui.match_highlight);
        setOptColor("searchMatch", theme.ui.search_match);
        setOptColor("tabBar", theme.ui.tab_bar);
        setOptColor("splitDivider", theme.ui.split_divider);

        ev::Persistent uiVal(ui.build());
        root.set("ui", uiVal.get());
    }

    // ansi
    {
        ev::Persistent ansiArr(ev::makeArray(16));
        static const char* const ansiNames[16] = {
            "black", "red", "green", "yellow", "blue", "magenta", "cyan", "white",
            "brightBlack", "brightRed", "brightGreen", "brightYellow",
            "brightBlue", "brightMagenta", "brightCyan", "brightWhite"
        };
        for (uint32_t i = 0; i < 16; ++i) {
            ev::Persistent cVal(colorToJs(theme.ansi[i], asHex));
            ansiArr.set(ev::setElement(ansiArr.get(), i, cVal.get()));
            ansiArr.set(ev::setProperty(ansiArr.get(), ansiNames[i], cVal.get()));
        }
        root.set("ansi", ansiArr.get());
    }

    // syntax
    {
        ObjectBuilder syntax;
        auto setOptSyntax = [&](std::string_view name, const std::optional<Color>& opt) {
            if (opt.has_value()) {
                ev::Persistent cv(colorToJs(*opt, asHex));
                syntax.set(name, cv.get());
            }
        };

        setOptSyntax("comment", theme.syntax.comment);
        setOptSyntax("string", theme.syntax.string);
        setOptSyntax("keyword", theme.syntax.keyword);
        setOptSyntax("number", theme.syntax.number);
        setOptSyntax("function", theme.syntax.function);
        setOptSyntax("type", theme.syntax.type);
        setOptSyntax("variable", theme.syntax.variable);
        setOptSyntax("constant", theme.syntax.constant);
        setOptSyntax("operator", theme.syntax.operator_color);
        setOptSyntax("operatorColor", theme.syntax.operator_color);
        setOptSyntax("punctuation", theme.syntax.punctuation);
        setOptSyntax("error", theme.syntax.error);
        setOptSyntax("warning", theme.syntax.warning);
        setOptSyntax("info", theme.syntax.info);
        setOptSyntax("hint", theme.syntax.hint);
        setOptSyntax("markupHeading", theme.syntax.markup_heading);
        setOptSyntax("markupLink", theme.syntax.markup_link);
        setOptSyntax("markupCode", theme.syntax.markup_code);

        ev::Persistent syntaxVal(syntax.build());
        root.set("syntax", syntaxVal.get());
    }

    return root.build();
}

std::optional<Theme> jsToTheme(Value val) {
    if (!ev::isObject(val)) return std::nullopt;

    Theme theme;
    ev::Persistent valP(val);

    Value nameVal = ev::getProperty(valP.get(), "name");
    if (ev::isString(nameVal)) {
        theme.metadata.name = ev::toUtf8(nameVal);
    }
    Value authorVal = ev::getProperty(valP.get(), "author");
    if (ev::isString(authorVal)) {
        theme.metadata.author = ev::toUtf8(authorVal);
    }
    Value descVal = ev::getProperty(valP.get(), "description");
    if (ev::isString(descVal)) {
        theme.metadata.description = ev::toUtf8(descVal);
    }
    Value isDarkVal = ev::getProperty(valP.get(), "isDark");
    if (ev::isBool(isDarkVal)) {
        theme.metadata.is_dark = ev::toBool(isDarkVal);
    }

    Value metaVal = ev::getProperty(valP.get(), "metadata");
    if (ev::isObject(metaVal)) {
        ev::Persistent metaP(metaVal);
        Value mName = ev::getProperty(metaP.get(), "name");
        if (ev::isString(mName) && theme.metadata.name.empty()) {
            theme.metadata.name = ev::toUtf8(mName);
        }
        Value mAuthor = ev::getProperty(metaP.get(), "author");
        if (ev::isString(mAuthor) && theme.metadata.author.empty()) {
            theme.metadata.author = ev::toUtf8(mAuthor);
        }
        Value mDesc = ev::getProperty(metaP.get(), "description");
        if (ev::isString(mDesc) && theme.metadata.description.empty()) {
            theme.metadata.description = ev::toUtf8(mDesc);
        }
        Value mDark = ev::getProperty(metaP.get(), "isDark");
        if (ev::isBool(mDark) && !theme.metadata.is_dark.has_value()) {
            theme.metadata.is_dark = ev::toBool(mDark);
        }
    }

    Value uiVal = ev::getProperty(valP.get(), "ui");
    if (ev::isObject(uiVal)) {
        ev::Persistent uiP(uiVal);
        auto readColor = [&](std::string_view prop, std::string_view alt = {}) -> std::optional<Color> {
            Value cv = ev::getProperty(uiP.get(), prop);
            auto res = parseColorValue(cv);
            if (!res && !alt.empty()) {
                cv = ev::getProperty(uiP.get(), alt);
                res = parseColorValue(cv);
            }
            return res;
        };

        if (auto bg = readColor("background")) theme.ui.background = *bg;
        if (auto fg = readColor("foreground")) theme.ui.foreground = *fg;

        theme.ui.cursor = readColor("cursor");
        theme.ui.cursor_text = readColor("cursorText", "cursor_text");
        theme.ui.selection_background = readColor("selectionBackground", "selection_background");
        theme.ui.selection_foreground = readColor("selectionForeground", "selection_foreground");
        theme.ui.border = readColor("border");
        theme.ui.status_bar = readColor("statusBar", "status_bar");
        theme.ui.line_number = readColor("lineNumber", "line_number");
        theme.ui.active_line = readColor("activeLine", "active_line");
        theme.ui.match_highlight = readColor("matchHighlight", "match_highlight");
        theme.ui.search_match = readColor("searchMatch", "search_match");
        theme.ui.tab_bar = readColor("tabBar", "tab_bar");
        theme.ui.split_divider = readColor("splitDivider", "split_divider");
    }

    Value ansiVal = ev::getProperty(valP.get(), "ansi");
    if (ev::isObject(ansiVal)) {
        ev::Persistent ansiP(ansiVal);
        for (uint32_t i = 0; i < 16; ++i) {
            Value elem = ev::getElement(ansiP.get(), i);
            auto c = parseColorValue(elem);
            if (c) {
                theme.ansi[i] = *c;
            }
        }
        static const char* const ansiNames[16] = {
            "black", "red", "green", "yellow", "blue", "magenta", "cyan", "white",
            "brightBlack", "brightRed", "brightGreen", "brightYellow",
            "brightBlue", "brightMagenta", "brightCyan", "brightWhite"
        };
        for (uint32_t i = 0; i < 16; ++i) {
            Value elem = ev::getProperty(ansiP.get(), ansiNames[i]);
            auto c = parseColorValue(elem);
            if (c) {
                theme.ansi[i] = *c;
            }
        }
    }

    Value syntaxVal = ev::getProperty(valP.get(), "syntax");
    if (ev::isObject(syntaxVal)) {
        ev::Persistent synP(syntaxVal);
        auto readSynColor = [&](std::string_view prop, std::string_view alt = {}) -> std::optional<Color> {
            Value cv = ev::getProperty(synP.get(), prop);
            auto res = parseColorValue(cv);
            if (!res && !alt.empty()) {
                cv = ev::getProperty(synP.get(), alt);
                res = parseColorValue(cv);
            }
            return res;
        };

        theme.syntax.comment = readSynColor("comment");
        theme.syntax.string = readSynColor("string");
        theme.syntax.keyword = readSynColor("keyword");
        theme.syntax.number = readSynColor("number");
        theme.syntax.function = readSynColor("function");
        theme.syntax.type = readSynColor("type");
        theme.syntax.variable = readSynColor("variable");
        theme.syntax.constant = readSynColor("constant");
        theme.syntax.operator_color = readSynColor("operator", "operatorColor");
        theme.syntax.punctuation = readSynColor("punctuation");
        theme.syntax.error = readSynColor("error");
        theme.syntax.warning = readSynColor("warning");
        theme.syntax.info = readSynColor("info");
        theme.syntax.hint = readSynColor("hint");
        theme.syntax.markup_heading = readSynColor("markupHeading", "markup_heading");
        theme.syntax.markup_link = readSynColor("markupLink", "markup_link");
        theme.syntax.markup_code = readSynColor("markupCode", "markup_code");
    }

    return theme;
}

std::optional<ThemeFormat> parseFormatString(std::string_view str) {
    std::string s;
    s.reserve(str.size());
    for (char c : str) {
        s.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }

    if (s == "auto") return ThemeFormat::Auto;
    if (s == "iterm" || s == "iterm2" || s == "itermcolors") return ThemeFormat::Iterm;
    if (s == "windows-terminal" || s == "windowsterminal" || s == "windows_terminal" || s == "wt") return ThemeFormat::WindowsTerminal;
    if (s == "alacritty-toml" || s == "alacritty_toml" || s == "alacritty") return ThemeFormat::AlacrittyToml;
    if (s == "alacritty-yaml" || s == "alacritty_yaml") return ThemeFormat::AlacrittyYaml;
    if (s == "kitty" || s == "kitty.conf") return ThemeFormat::Kitty;
    if (s == "ghostty") return ThemeFormat::Ghostty;
    if (s == "base16-yaml" || s == "base16_yaml" || s == "base16") return ThemeFormat::Base16Yaml;
    if (s == "base16-json" || s == "base16_json") return ThemeFormat::Base16Json;
    if (s == "base24-yaml" || s == "base24_yaml" || s == "base24") return ThemeFormat::Base24Yaml;
    if (s == "base24-json" || s == "base24_json") return ThemeFormat::Base24Json;
    if (s == "vscode" || s == "vs-code" || s == "vs_code") return ThemeFormat::VsCode;

    return std::nullopt;
}

std::string formatToString(ThemeFormat fmt) {
    switch (fmt) {
        case ThemeFormat::Auto: return "auto";
        case ThemeFormat::Iterm: return "iterm";
        case ThemeFormat::WindowsTerminal: return "windows-terminal";
        case ThemeFormat::AlacrittyToml: return "alacritty-toml";
        case ThemeFormat::AlacrittyYaml: return "alacritty-yaml";
        case ThemeFormat::Kitty: return "kitty";
        case ThemeFormat::Ghostty: return "ghostty";
        case ThemeFormat::Base16Yaml: return "base16-yaml";
        case ThemeFormat::Base16Json: return "base16-json";
        case ThemeFormat::Base24Yaml: return "base24-yaml";
        case ThemeFormat::Base24Json: return "base24-json";
        case ThemeFormat::VsCode: return "vscode";
    }
    return "auto";
}

Value linearRgbToJs(LinearRgb c) {
    ObjectBuilder b;
    b.set("r", static_cast<double>(c.r));
    b.set("g", static_cast<double>(c.g));
    b.set("b", static_cast<double>(c.b));
    return b.build();
}

std::optional<LinearRgb> jsToLinearRgb(Value v) {
    if (!ev::isObject(v)) return std::nullopt;
    ev::Persistent vP(v);
    Value rVal = ev::getProperty(vP.get(), "r");
    Value gVal = ev::getProperty(vP.get(), "g");
    Value bVal = ev::getProperty(vP.get(), "b");
    if (ev::isNumber(rVal) && ev::isNumber(gVal) && ev::isNumber(bVal)) {
        return LinearRgb(
            static_cast<float>(ev::toDouble(rVal)),
            static_cast<float>(ev::toDouble(gVal)),
            static_cast<float>(ev::toDouble(bVal))
        );
    }
    return std::nullopt;
}

Value xyzToJs(Xyz c) {
    ObjectBuilder b;
    b.set("x", static_cast<double>(c.x));
    b.set("y", static_cast<double>(c.y));
    b.set("z", static_cast<double>(c.z));
    return b.build();
}

std::optional<Xyz> jsToXyz(Value v) {
    if (!ev::isObject(v)) return std::nullopt;
    ev::Persistent vP(v);
    Value xVal = ev::getProperty(vP.get(), "x");
    Value yVal = ev::getProperty(vP.get(), "y");
    Value zVal = ev::getProperty(vP.get(), "z");
    if (ev::isNumber(xVal) && ev::isNumber(yVal) && ev::isNumber(zVal)) {
        return Xyz(
            static_cast<float>(ev::toDouble(xVal)),
            static_cast<float>(ev::toDouble(yVal)),
            static_cast<float>(ev::toDouble(zVal))
        );
    }
    return std::nullopt;
}

Value labToJs(Lab c) {
    ObjectBuilder b;
    b.set("l", static_cast<double>(c.l));
    b.set("a", static_cast<double>(c.a));
    b.set("b", static_cast<double>(c.b));
    return b.build();
}

std::optional<Lab> jsToLab(Value v) {
    if (!ev::isObject(v)) return std::nullopt;
    ev::Persistent vP(v);
    Value lVal = ev::getProperty(vP.get(), "l");
    Value aVal = ev::getProperty(vP.get(), "a");
    Value bVal = ev::getProperty(vP.get(), "b");
    if (ev::isNumber(lVal) && ev::isNumber(aVal) && ev::isNumber(bVal)) {
        return Lab(
            static_cast<float>(ev::toDouble(lVal)),
            static_cast<float>(ev::toDouble(aVal)),
            static_cast<float>(ev::toDouble(bVal))
        );
    }
    return std::nullopt;
}

Value oklabToJs(Oklab c) {
    ObjectBuilder b;
    b.set("l", static_cast<double>(c.l));
    b.set("a", static_cast<double>(c.a));
    b.set("b", static_cast<double>(c.b));
    return b.build();
}

std::optional<Oklab> jsToOklab(Value v) {
    if (!ev::isObject(v)) return std::nullopt;
    ev::Persistent vP(v);
    Value lVal = ev::getProperty(vP.get(), "l");
    Value aVal = ev::getProperty(vP.get(), "a");
    Value bVal = ev::getProperty(vP.get(), "b");
    if (ev::isNumber(lVal) && ev::isNumber(aVal) && ev::isNumber(bVal)) {
        return Oklab(
            static_cast<float>(ev::toDouble(lVal)),
            static_cast<float>(ev::toDouble(aVal)),
            static_cast<float>(ev::toDouble(bVal))
        );
    }
    return std::nullopt;
}

Value oklchToJs(Oklch c) {
    ObjectBuilder b;
    b.set("l", static_cast<double>(c.l));
    b.set("c", static_cast<double>(c.c));
    b.set("h", static_cast<double>(c.h));
    return b.build();
}

std::optional<Oklch> jsToOklch(Value v) {
    if (!ev::isObject(v)) return std::nullopt;
    ev::Persistent vP(v);
    Value lVal = ev::getProperty(vP.get(), "l");
    Value cVal = ev::getProperty(vP.get(), "c");
    Value hVal = ev::getProperty(vP.get(), "h");
    if (ev::isNumber(lVal) && ev::isNumber(cVal) && ev::isNumber(hVal)) {
        return Oklch(
            static_cast<float>(ev::toDouble(lVal)),
            static_cast<float>(ev::toDouble(cVal)),
            static_cast<float>(ev::toDouble(hVal))
        );
    }
    return std::nullopt;
}

Value hslToJs(Hsl c) {
    ObjectBuilder b;
    b.set("h", static_cast<double>(c.h));
    b.set("s", static_cast<double>(c.s));
    b.set("l", static_cast<double>(c.l));
    return b.build();
}

std::optional<Hsl> jsToHsl(Value v) {
    if (!ev::isObject(v)) return std::nullopt;
    ev::Persistent vP(v);
    Value hVal = ev::getProperty(vP.get(), "h");
    Value sVal = ev::getProperty(vP.get(), "s");
    Value lVal = ev::getProperty(vP.get(), "l");
    if (ev::isNumber(hVal) && ev::isNumber(sVal) && ev::isNumber(lVal)) {
        return Hsl(
            static_cast<float>(ev::toDouble(hVal)),
            static_cast<float>(ev::toDouble(sVal)),
            static_cast<float>(ev::toDouble(lVal))
        );
    }
    return std::nullopt;
}

Value hsvToJs(Hsv c) {
    ObjectBuilder b;
    b.set("h", static_cast<double>(c.h));
    b.set("s", static_cast<double>(c.s));
    b.set("v", static_cast<double>(c.v));
    return b.build();
}

std::optional<Hsv> jsToHsv(Value v) {
    if (!ev::isObject(v)) return std::nullopt;
    ev::Persistent vP(v);
    Value hVal = ev::getProperty(vP.get(), "h");
    Value sVal = ev::getProperty(vP.get(), "s");
    Value vVal = ev::getProperty(vP.get(), "v");
    if (ev::isNumber(hVal) && ev::isNumber(sVal) && ev::isNumber(vVal)) {
        return Hsv(
            static_cast<float>(ev::toDouble(hVal)),
            static_cast<float>(ev::toDouble(sVal)),
            static_cast<float>(ev::toDouble(vVal))
        );
    }
    return std::nullopt;
}

} // namespace bro::themes::api

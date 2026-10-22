#include "windows_terminal.h"
#include "json_util.h"

namespace bro::themes::detail {

namespace {

std::optional<Color> json_get_color(const JsonValue& obj, std::string_view key) {
    const JsonValue* val = obj.find(key);
    if (!val || !val->is_string()) return std::nullopt;
    return parse_hex(val->as_string());
}

std::optional<Theme> parse_wt_scheme(const JsonValue& obj) {
    Theme theme;
    bool found_any = false;

    if (const JsonValue* n = obj.find("name"); n && n->is_string()) {
        theme.metadata.name = n->as_string();
    }

    auto set_color = [&](std::string_view key, auto& dest) -> bool {
        if (auto c = json_get_color(obj, key)) {
            dest = *c;
            found_any = true;
            return true;
        }
        return false;
    };

    set_color("background", theme.ui.background);
    set_color("foreground", theme.ui.foreground);
    set_color("cursorColor", theme.ui.cursor);
    set_color("selectionBackground", theme.ui.selection_background);

    set_color("black", theme.ansi[0]);
    set_color("red", theme.ansi[1]);
    set_color("green", theme.ansi[2]);
    set_color("yellow", theme.ansi[3]);
    set_color("blue", theme.ansi[4]);
    if (!set_color("purple", theme.ansi[5])) {
        set_color("magenta", theme.ansi[5]);
    }
    set_color("cyan", theme.ansi[6]);
    set_color("white", theme.ansi[7]);

    set_color("brightBlack", theme.ansi[8]);
    set_color("brightRed", theme.ansi[9]);
    set_color("brightGreen", theme.ansi[10]);
    set_color("brightYellow", theme.ansi[11]);
    set_color("brightBlue", theme.ansi[12]);
    if (!set_color("brightPurple", theme.ansi[13])) {
        set_color("brightMagenta", theme.ansi[13]);
    }
    set_color("brightCyan", theme.ansi[14]);
    set_color("brightWhite", theme.ansi[15]);

    if (!found_any) return std::nullopt;
    return theme;
}

} // namespace

std::optional<Theme> import_windows_terminal(std::string_view content) {
    auto json = JsonValue::parse(content);
    if (!json.has_value() || !json->is_object()) {
        return std::nullopt;
    }

    // Check if it's a wrapper with "schemes" array
    if (const JsonValue* schemes = json->find("schemes"); schemes && schemes->is_array()) {
        for (const auto& entry : schemes->arr_val) {
            if (entry.is_object()) {
                if (auto t = parse_wt_scheme(entry)) return t;
            }
        }
        return std::nullopt;
    }

    // Otherwise treat this object as the scheme itself
    return parse_wt_scheme(*json);
}

std::string export_windows_terminal(const Theme& theme) {
    JsonValue obj;
    obj.type = JsonType::Object;

    std::string name = theme.metadata.name.empty() ? "Custom" : theme.metadata.name;
    obj["name"] = JsonValue(name);
    obj["background"] = JsonValue(to_hex(theme.ui.background));
    obj["foreground"] = JsonValue(to_hex(theme.ui.foreground));
    obj["cursorColor"] = JsonValue(to_hex(theme.ui.get_cursor()));
    obj["selectionBackground"] = JsonValue(to_hex(theme.ui.get_selection_background()));

    obj["black"] = JsonValue(to_hex(theme.ansi[0]));
    obj["red"] = JsonValue(to_hex(theme.ansi[1]));
    obj["green"] = JsonValue(to_hex(theme.ansi[2]));
    obj["yellow"] = JsonValue(to_hex(theme.ansi[3]));
    obj["blue"] = JsonValue(to_hex(theme.ansi[4]));
    obj["purple"] = JsonValue(to_hex(theme.ansi[5]));
    obj["cyan"] = JsonValue(to_hex(theme.ansi[6]));
    obj["white"] = JsonValue(to_hex(theme.ansi[7]));

    obj["brightBlack"] = JsonValue(to_hex(theme.ansi[8]));
    obj["brightRed"] = JsonValue(to_hex(theme.ansi[9]));
    obj["brightGreen"] = JsonValue(to_hex(theme.ansi[10]));
    obj["brightYellow"] = JsonValue(to_hex(theme.ansi[11]));
    obj["brightBlue"] = JsonValue(to_hex(theme.ansi[12]));
    obj["brightPurple"] = JsonValue(to_hex(theme.ansi[13]));
    obj["brightCyan"] = JsonValue(to_hex(theme.ansi[14]));
    obj["brightWhite"] = JsonValue(to_hex(theme.ansi[15]));

    return obj.dump(2);
}

} // namespace bro::themes::detail

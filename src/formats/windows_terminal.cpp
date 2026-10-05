#include "windows_terminal.h"
#include "json_util.h"

namespace bro::themes::detail {

namespace {

std::optional<Color> json_get_color(const JsonValue& obj, std::string_view key) {
    const JsonValue* val = obj.find(key);
    if (!val || !val->is_string()) return std::nullopt;
    return parse_hex(val->as_string());
}

Theme parse_wt_scheme(const JsonValue& obj) {
    Theme theme;

    if (const JsonValue* n = obj.find("name"); n && n->is_string()) {
        theme.metadata.name = n->as_string();
    }

    if (auto c = json_get_color(obj, "background")) theme.ui.background = *c;
    if (auto c = json_get_color(obj, "foreground")) theme.ui.foreground = *c;
    if (auto c = json_get_color(obj, "cursorColor")) theme.ui.cursor = *c;
    if (auto c = json_get_color(obj, "selectionBackground")) theme.ui.selection_background = *c;

    if (auto c = json_get_color(obj, "black")) theme.ansi[0] = *c;
    if (auto c = json_get_color(obj, "red")) theme.ansi[1] = *c;
    if (auto c = json_get_color(obj, "green")) theme.ansi[2] = *c;
    if (auto c = json_get_color(obj, "yellow")) theme.ansi[3] = *c;
    if (auto c = json_get_color(obj, "blue")) theme.ansi[4] = *c;
    if (auto c = json_get_color(obj, "purple")) theme.ansi[5] = *c;
    else if (auto c2 = json_get_color(obj, "magenta")) theme.ansi[5] = *c2;
    if (auto c = json_get_color(obj, "cyan")) theme.ansi[6] = *c;
    if (auto c = json_get_color(obj, "white")) theme.ansi[7] = *c;

    if (auto c = json_get_color(obj, "brightBlack")) theme.ansi[8] = *c;
    if (auto c = json_get_color(obj, "brightRed")) theme.ansi[9] = *c;
    if (auto c = json_get_color(obj, "brightGreen")) theme.ansi[10] = *c;
    if (auto c = json_get_color(obj, "brightYellow")) theme.ansi[11] = *c;
    if (auto c = json_get_color(obj, "brightBlue")) theme.ansi[12] = *c;
    if (auto c = json_get_color(obj, "brightPurple")) theme.ansi[13] = *c;
    else if (auto c2 = json_get_color(obj, "brightMagenta")) theme.ansi[13] = *c2;
    if (auto c = json_get_color(obj, "brightCyan")) theme.ansi[14] = *c;
    if (auto c = json_get_color(obj, "brightWhite")) theme.ansi[15] = *c;

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
        if (!schemes->arr_val.empty() && schemes->arr_val.front().is_object()) {
            return parse_wt_scheme(schemes->arr_val.front());
        }
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

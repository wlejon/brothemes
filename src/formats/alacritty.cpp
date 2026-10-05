#include "alacritty.h"
#include "toml_yaml_util.h"
#include <sstream>

namespace bro::themes::detail {

namespace {

std::optional<Color> map_get_color(const ConfigMap& map, std::string_view key) {
    const std::string* val = map.get(key);
    if (!val) return std::nullopt;
    return parse_hex(*val);
}

Theme parse_alacritty_map(const ConfigMap& map) {
    Theme theme;

    if (auto c = map_get_color(map, "colors.primary.background")) theme.ui.background = *c;
    if (auto c = map_get_color(map, "colors.primary.foreground")) theme.ui.foreground = *c;

    if (auto c = map_get_color(map, "colors.cursor.cursor")) theme.ui.cursor = *c;
    else if (auto c2 = map_get_color(map, "colors.cursor.background")) theme.ui.cursor = *c2;

    if (auto c = map_get_color(map, "colors.cursor.text")) theme.ui.cursor_text = *c;

    if (auto c = map_get_color(map, "colors.selection.background")) theme.ui.selection_background = *c;
    if (auto c = map_get_color(map, "colors.selection.text")) theme.ui.selection_foreground = *c;

    // Normal colors (0-7)
    if (auto c = map_get_color(map, "colors.normal.black")) theme.ansi[0] = *c;
    if (auto c = map_get_color(map, "colors.normal.red")) theme.ansi[1] = *c;
    if (auto c = map_get_color(map, "colors.normal.green")) theme.ansi[2] = *c;
    if (auto c = map_get_color(map, "colors.normal.yellow")) theme.ansi[3] = *c;
    if (auto c = map_get_color(map, "colors.normal.blue")) theme.ansi[4] = *c;
    if (auto c = map_get_color(map, "colors.normal.magenta")) theme.ansi[5] = *c;
    if (auto c = map_get_color(map, "colors.normal.cyan")) theme.ansi[6] = *c;
    if (auto c = map_get_color(map, "colors.normal.white")) theme.ansi[7] = *c;

    // Bright colors (8-15)
    if (auto c = map_get_color(map, "colors.bright.black")) theme.ansi[8] = *c;
    if (auto c = map_get_color(map, "colors.bright.red")) theme.ansi[9] = *c;
    if (auto c = map_get_color(map, "colors.bright.green")) theme.ansi[10] = *c;
    if (auto c = map_get_color(map, "colors.bright.yellow")) theme.ansi[11] = *c;
    if (auto c = map_get_color(map, "colors.bright.blue")) theme.ansi[12] = *c;
    if (auto c = map_get_color(map, "colors.bright.magenta")) theme.ansi[13] = *c;
    if (auto c = map_get_color(map, "colors.bright.cyan")) theme.ansi[14] = *c;
    if (auto c = map_get_color(map, "colors.bright.white")) theme.ansi[15] = *c;

    return theme;
}

} // namespace

std::optional<Theme> import_alacritty(std::string_view content) {
    // Try TOML first
    ConfigMap toml_map = ConfigMap::parse_toml(content);
    if (toml_map.has("colors.primary.background") || toml_map.has("colors.normal.red")) {
        return parse_alacritty_map(toml_map);
    }

    // Try YAML
    ConfigMap yaml_map = ConfigMap::parse_yaml(content);
    if (yaml_map.has("colors.primary.background") || yaml_map.has("colors.normal.red")) {
        return parse_alacritty_map(yaml_map);
    }

    return std::nullopt;
}

std::string export_alacritty_toml(const Theme& theme) {
    std::ostringstream ss;
    if (!theme.metadata.name.empty()) {
        ss << "# " << theme.metadata.name << "\n\n";
    }

    ss << "[colors.primary]\n";
    ss << "background = \"" << to_hex(theme.ui.background) << "\"\n";
    ss << "foreground = \"" << to_hex(theme.ui.foreground) << "\"\n\n";

    ss << "[colors.cursor]\n";
    ss << "text = \"" << to_hex(theme.ui.get_cursor_text()) << "\"\n";
    ss << "cursor = \"" << to_hex(theme.ui.get_cursor()) << "\"\n\n";

    ss << "[colors.selection]\n";
    ss << "text = \"" << to_hex(theme.ui.get_selection_foreground()) << "\"\n";
    ss << "background = \"" << to_hex(theme.ui.get_selection_background()) << "\"\n\n";

    ss << "[colors.normal]\n";
    ss << "black = \"" << to_hex(theme.ansi[0]) << "\"\n";
    ss << "red = \"" << to_hex(theme.ansi[1]) << "\"\n";
    ss << "green = \"" << to_hex(theme.ansi[2]) << "\"\n";
    ss << "yellow = \"" << to_hex(theme.ansi[3]) << "\"\n";
    ss << "blue = \"" << to_hex(theme.ansi[4]) << "\"\n";
    ss << "magenta = \"" << to_hex(theme.ansi[5]) << "\"\n";
    ss << "cyan = \"" << to_hex(theme.ansi[6]) << "\"\n";
    ss << "white = \"" << to_hex(theme.ansi[7]) << "\"\n\n";

    ss << "[colors.bright]\n";
    ss << "black = \"" << to_hex(theme.ansi[8]) << "\"\n";
    ss << "red = \"" << to_hex(theme.ansi[9]) << "\"\n";
    ss << "green = \"" << to_hex(theme.ansi[10]) << "\"\n";
    ss << "yellow = \"" << to_hex(theme.ansi[11]) << "\"\n";
    ss << "blue = \"" << to_hex(theme.ansi[12]) << "\"\n";
    ss << "magenta = \"" << to_hex(theme.ansi[13]) << "\"\n";
    ss << "cyan = \"" << to_hex(theme.ansi[14]) << "\"\n";
    ss << "white = \"" << to_hex(theme.ansi[15]) << "\"\n";

    return ss.str();
}

std::string export_alacritty_yaml(const Theme& theme) {
    std::ostringstream ss;
    if (!theme.metadata.name.empty()) {
        ss << "# " << theme.metadata.name << "\n";
    }

    ss << "colors:\n";
    ss << "  primary:\n";
    ss << "    background: '" << to_hex(theme.ui.background) << "'\n";
    ss << "    foreground: '" << to_hex(theme.ui.foreground) << "'\n";

    ss << "  cursor:\n";
    ss << "    text: '" << to_hex(theme.ui.get_cursor_text()) << "'\n";
    ss << "    cursor: '" << to_hex(theme.ui.get_cursor()) << "'\n";

    ss << "  selection:\n";
    ss << "    text: '" << to_hex(theme.ui.get_selection_foreground()) << "'\n";
    ss << "    background: '" << to_hex(theme.ui.get_selection_background()) << "'\n";

    ss << "  normal:\n";
    ss << "    black: '" << to_hex(theme.ansi[0]) << "'\n";
    ss << "    red: '" << to_hex(theme.ansi[1]) << "'\n";
    ss << "    green: '" << to_hex(theme.ansi[2]) << "'\n";
    ss << "    yellow: '" << to_hex(theme.ansi[3]) << "'\n";
    ss << "    blue: '" << to_hex(theme.ansi[4]) << "'\n";
    ss << "    magenta: '" << to_hex(theme.ansi[5]) << "'\n";
    ss << "    cyan: '" << to_hex(theme.ansi[6]) << "'\n";
    ss << "    white: '" << to_hex(theme.ansi[7]) << "'\n";

    ss << "  bright:\n";
    ss << "    black: '" << to_hex(theme.ansi[8]) << "'\n";
    ss << "    red: '" << to_hex(theme.ansi[9]) << "'\n";
    ss << "    green: '" << to_hex(theme.ansi[10]) << "'\n";
    ss << "    yellow: '" << to_hex(theme.ansi[11]) << "'\n";
    ss << "    blue: '" << to_hex(theme.ansi[12]) << "'\n";
    ss << "    magenta: '" << to_hex(theme.ansi[13]) << "'\n";
    ss << "    cyan: '" << to_hex(theme.ansi[14]) << "'\n";
    ss << "    white: '" << to_hex(theme.ansi[15]) << "'\n";

    return ss.str();
}

} // namespace bro::themes::detail

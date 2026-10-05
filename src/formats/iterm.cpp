#include "iterm.h"
#include "xml_plist.h"
#include <algorithm>
#include <cmath>

namespace bro::themes::detail {

namespace {

std::optional<Color> parse_plist_color(const PlistNode* dict) {
    if (!dict || !dict->is_dict()) return std::nullopt;

    const PlistNode* r_node = dict->find("Red Component");
    const PlistNode* g_node = dict->find("Green Component");
    const PlistNode* b_node = dict->find("Blue Component");
    const PlistNode* a_node = dict->find("Alpha Component");

    if (!r_node || !g_node || !b_node) return std::nullopt;

    float r = static_cast<float>(r_node->as_real());
    float g = static_cast<float>(g_node->as_real());
    float b = static_cast<float>(b_node->as_real());
    float a = a_node ? static_cast<float>(a_node->as_real()) : 1.0f;

    auto to_u8 = [](float v) -> uint8_t {
        return static_cast<uint8_t>(std::round(std::clamp(v, 0.0f, 1.0f) * 255.0f));
    };

    return Color(to_u8(r), to_u8(g), to_u8(b), to_u8(a));
}

PlistNode make_plist_color(Color c) {
    std::map<std::string, PlistNode> d;
    d.emplace("Color Space", PlistNode("sRGB"));
    d.emplace("Red Component", PlistNode(static_cast<double>(c.r) / 255.0));
    d.emplace("Green Component", PlistNode(static_cast<double>(c.g) / 255.0));
    d.emplace("Blue Component", PlistNode(static_cast<double>(c.b) / 255.0));
    d.emplace("Alpha Component", PlistNode(static_cast<double>(c.a) / 255.0));
    return PlistNode(std::move(d));
}

} // namespace

std::optional<Theme> import_iterm(std::string_view content) {
    auto root = PlistNode::parse(content);
    if (!root.has_value() || !root->is_dict()) {
        return std::nullopt;
    }

    Theme theme;

    // ANSI Colors 0 - 15
    for (size_t i = 0; i < 16; ++i) {
        std::string key = "Ansi " + std::to_string(i) + " Color";
        if (auto col = parse_plist_color(root->find(key))) {
            theme.ansi[i] = *col;
        }
    }

    // UI Colors
    if (auto col = parse_plist_color(root->find("Background Color"))) {
        theme.ui.background = *col;
    }
    if (auto col = parse_plist_color(root->find("Foreground Color"))) {
        theme.ui.foreground = *col;
    }
    if (auto col = parse_plist_color(root->find("Cursor Color"))) {
        theme.ui.cursor = *col;
    }
    if (auto col = parse_plist_color(root->find("Cursor Text Color"))) {
        theme.ui.cursor_text = *col;
    }
    if (auto col = parse_plist_color(root->find("Selection Color"))) {
        theme.ui.selection_background = *col;
    }
    if (auto col = parse_plist_color(root->find("Selected Text Color"))) {
        theme.ui.selection_foreground = *col;
    }

    return theme;
}

std::string export_iterm(const Theme& theme) {
    std::map<std::string, PlistNode> dict;

    // ANSI colors
    for (size_t i = 0; i < 16; ++i) {
        std::string key = "Ansi " + std::to_string(i) + " Color";
        dict.emplace(key, make_plist_color(theme.ansi[i]));
    }

    // UI colors
    dict.emplace("Background Color", make_plist_color(theme.ui.background));
    dict.emplace("Foreground Color", make_plist_color(theme.ui.foreground));
    dict.emplace("Cursor Color", make_plist_color(theme.ui.get_cursor()));
    dict.emplace("Cursor Text Color", make_plist_color(theme.ui.get_cursor_text()));
    dict.emplace("Selection Color", make_plist_color(theme.ui.get_selection_background()));
    dict.emplace("Selected Text Color", make_plist_color(theme.ui.get_selection_foreground()));
    dict.emplace("Bold Color", make_plist_color(theme.ui.foreground));

    PlistNode root(std::move(dict));
    return root.serialize();
}

} // namespace bro::themes::detail

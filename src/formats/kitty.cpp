#include "kitty.h"
#include "kv_util.h"
#include <sstream>

namespace bro::themes::detail {

std::optional<Theme> import_kitty(std::string_view content) {
    auto pairs = parse_key_value_lines(content);
    if (pairs.empty()) return std::nullopt;

    Theme theme;
    bool found_any = false;

    for (const auto& [k, v] : pairs) {
        auto col = parse_hex(v);
        if (!col.has_value()) continue;

        if (k == "background") {
            theme.ui.background = *col;
            found_any = true;
        } else if (k == "foreground") {
            theme.ui.foreground = *col;
            found_any = true;
        } else if (k == "cursor") {
            theme.ui.cursor = *col;
            found_any = true;
        } else if (k == "cursor_text_color") {
            theme.ui.cursor_text = *col;
            found_any = true;
        } else if (k == "selection_background") {
            theme.ui.selection_background = *col;
            found_any = true;
        } else if (k == "selection_foreground") {
            theme.ui.selection_foreground = *col;
            found_any = true;
        } else if (k == "active_border_color") {
            theme.ui.border = *col;
        } else if (k == "tab_bar_background") {
            theme.ui.tab_bar = *col;
        } else if (k.starts_with("color")) {
            std::string_view idx_sv = std::string_view(k).substr(5);
            try {
                int idx = std::stoi(std::string(idx_sv));
                if (idx >= 0 && idx < 16) {
                    theme.ansi[idx] = *col;
                    found_any = true;
                }
            } catch (...) {}
        }
    }

    if (!found_any) return std::nullopt;
    return theme;
}

std::string export_kitty(const Theme& theme) {
    std::ostringstream ss;
    if (!theme.metadata.name.empty()) {
        ss << "# " << theme.metadata.name << "\n\n";
    }

    ss << "background " << to_hex(theme.ui.background) << "\n";
    ss << "foreground " << to_hex(theme.ui.foreground) << "\n";
    ss << "cursor " << to_hex(theme.ui.get_cursor()) << "\n";
    ss << "cursor_text_color " << to_hex(theme.ui.get_cursor_text()) << "\n";
    ss << "selection_background " << to_hex(theme.ui.get_selection_background()) << "\n";
    ss << "selection_foreground " << to_hex(theme.ui.get_selection_foreground()) << "\n";

    if (theme.ui.border.has_value()) {
        ss << "active_border_color " << to_hex(*theme.ui.border) << "\n";
    }
    if (theme.ui.tab_bar.has_value()) {
        ss << "tab_bar_background " << to_hex(*theme.ui.tab_bar) << "\n";
    }

    ss << "\n";
    for (size_t i = 0; i < 16; ++i) {
        ss << "color" << i << " " << to_hex(theme.ansi[i]) << "\n";
    }

    return ss.str();
}

} // namespace bro::themes::detail

#include "ghostty.h"
#include "kv_util.h"
#include <sstream>

namespace bro::themes::detail {

std::optional<Theme> import_ghostty(std::string_view content) {
    auto pairs = parse_key_value_lines(content);
    if (pairs.empty()) return std::nullopt;

    Theme theme;
    bool found_any = false;

    for (const auto& [k, v] : pairs) {
        if (k == "palette") {
            // value is like "0=#21222c" or "1=#ff5555"
            size_t eq = v.find('=');
            if (eq != std::string_view::npos) {
                std::string idx_str = std::string(trim(std::string_view(v).substr(0, eq)));
                std::string col_str = std::string(trim(std::string_view(v).substr(eq + 1)));
                try {
                    int idx = std::stoi(idx_str);
                    if (idx >= 0 && idx < 16) {
                        if (auto c = parse_hex(col_str)) {
                            theme.ansi[idx] = *c;
                            found_any = true;
                        }
                    }
                } catch (...) {}
            }
            continue;
        }

        auto col = parse_hex(v);
        if (!col.has_value()) continue;

        if (k == "background") {
            theme.ui.background = *col;
            found_any = true;
        } else if (k == "foreground") {
            theme.ui.foreground = *col;
            found_any = true;
        } else if (k == "cursor-color" || k == "cursor_color") {
            theme.ui.cursor = *col;
            found_any = true;
        } else if (k == "cursor-text" || k == "cursor_text") {
            theme.ui.cursor_text = *col;
            found_any = true;
        } else if (k == "selection-background" || k == "selection_background") {
            theme.ui.selection_background = *col;
            found_any = true;
        } else if (k == "selection-foreground" || k == "selection_foreground") {
            theme.ui.selection_foreground = *col;
            found_any = true;
        }
    }

    if (!found_any) return std::nullopt;
    return theme;
}

std::string export_ghostty(const Theme& theme) {
    std::ostringstream ss;
    if (!theme.metadata.name.empty()) {
        ss << "# " << theme.metadata.name << "\n\n";
    }

    for (size_t i = 0; i < 16; ++i) {
        ss << "palette = " << i << "=" << to_hex(theme.ansi[i]) << "\n";
    }

    ss << "\n";
    ss << "background = " << to_hex(theme.ui.background) << "\n";
    ss << "foreground = " << to_hex(theme.ui.foreground) << "\n";
    ss << "cursor-color = " << to_hex(theme.ui.get_cursor()) << "\n";
    ss << "cursor-text = " << to_hex(theme.ui.get_cursor_text()) << "\n";
    ss << "selection-background = " << to_hex(theme.ui.get_selection_background()) << "\n";
    ss << "selection-foreground = " << to_hex(theme.ui.get_selection_foreground()) << "\n";

    return ss.str();
}

} // namespace bro::themes::detail

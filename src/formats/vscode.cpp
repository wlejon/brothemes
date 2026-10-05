#include "vscode.h"
#include "json_util.h"
#include <vector>

namespace bro::themes::detail {

namespace {

std::optional<Color> json_get_color(const JsonValue& obj, std::string_view key) {
    const JsonValue* val = obj.find(key);
    if (!val || !val->is_string()) return std::nullopt;
    return parse_hex(val->as_string());
}

} // namespace

std::optional<Theme> import_vscode(std::string_view content) {
    auto json = JsonValue::parse(content);
    if (!json.has_value() || !json->is_object()) {
        return std::nullopt;
    }

    Theme theme;

    if (const JsonValue* n = json->find("name"); n && n->is_string()) {
        theme.metadata.name = n->as_string();
    }
    if (const JsonValue* t = json->find("type"); t && t->is_string()) {
        theme.metadata.is_dark = (t->as_string() != "light");
    }

    const JsonValue* colors = json->find("colors");
    if (colors && colors->is_object()) {
        // UI colors
        if (auto c = json_get_color(*colors, "editor.background")) theme.ui.background = *c;
        else if (auto c2 = json_get_color(*colors, "terminal.background")) theme.ui.background = *c2;

        if (auto c = json_get_color(*colors, "editor.foreground")) theme.ui.foreground = *c;
        else if (auto c2 = json_get_color(*colors, "terminal.foreground")) theme.ui.foreground = *c2;

        if (auto c = json_get_color(*colors, "editorCursor.foreground")) theme.ui.cursor = *c;
        else if (auto c2 = json_get_color(*colors, "terminalCursor.foreground")) theme.ui.cursor = *c2;

        if (auto c = json_get_color(*colors, "editor.selectionBackground")) theme.ui.selection_background = *c;
        if (auto c = json_get_color(*colors, "editor.selectionForeground")) theme.ui.selection_foreground = *c;
        if (auto c = json_get_color(*colors, "editor.lineHighlightBackground")) theme.ui.active_line = *c;
        if (auto c = json_get_color(*colors, "editorLineNumber.foreground")) theme.ui.line_number = *c;
        if (auto c = json_get_color(*colors, "statusBar.background")) theme.ui.status_bar = *c;
        if (auto c = json_get_color(*colors, "sideBar.border")) theme.ui.border = *c;
        else if (auto c2 = json_get_color(*colors, "editorGroup.border")) theme.ui.border = *c2;

        // Terminal ANSI colors
        if (auto c = json_get_color(*colors, "terminal.ansiBlack")) theme.ansi[0] = *c;
        if (auto c = json_get_color(*colors, "terminal.ansiRed")) theme.ansi[1] = *c;
        if (auto c = json_get_color(*colors, "terminal.ansiGreen")) theme.ansi[2] = *c;
        if (auto c = json_get_color(*colors, "terminal.ansiYellow")) theme.ansi[3] = *c;
        if (auto c = json_get_color(*colors, "terminal.ansiBlue")) theme.ansi[4] = *c;
        if (auto c = json_get_color(*colors, "terminal.ansiMagenta")) theme.ansi[5] = *c;
        if (auto c = json_get_color(*colors, "terminal.ansiCyan")) theme.ansi[6] = *c;
        if (auto c = json_get_color(*colors, "terminal.ansiWhite")) theme.ansi[7] = *c;

        if (auto c = json_get_color(*colors, "terminal.ansiBrightBlack")) theme.ansi[8] = *c;
        if (auto c = json_get_color(*colors, "terminal.ansiBrightRed")) theme.ansi[9] = *c;
        if (auto c = json_get_color(*colors, "terminal.ansiBrightGreen")) theme.ansi[10] = *c;
        if (auto c = json_get_color(*colors, "terminal.ansiBrightYellow")) theme.ansi[11] = *c;
        if (auto c = json_get_color(*colors, "terminal.ansiBrightBlue")) theme.ansi[12] = *c;
        if (auto c = json_get_color(*colors, "terminal.ansiBrightMagenta")) theme.ansi[13] = *c;
        if (auto c = json_get_color(*colors, "terminal.ansiBrightCyan")) theme.ansi[14] = *c;
        if (auto c = json_get_color(*colors, "terminal.ansiBrightWhite")) theme.ansi[15] = *c;
    }

    // Token colors
    const JsonValue* tokens = json->find("tokenColors");
    if (tokens && tokens->is_array()) {
        for (const auto& item : tokens->arr_val) {
            if (!item.is_object()) continue;
            const JsonValue* settings = item.find("settings");
            if (!settings || !settings->is_object()) continue;
            auto fg = json_get_color(*settings, "foreground");
            if (!fg.has_value()) continue;

            auto match_scope = [&](std::string_view target) -> bool {
                const JsonValue* scope = item.find("scope");
                if (!scope) return false;
                if (scope->is_string()) {
                    return scope->as_string().find(target) != std::string::npos;
                }
                if (scope->is_array()) {
                    for (const auto& s : scope->arr_val) {
                        if (s.is_string() && s.as_string().find(target) != std::string::npos) {
                            return true;
                        }
                    }
                }
                return false;
            };

            if (!theme.syntax.comment && match_scope("comment")) theme.syntax.comment = *fg;
            if (!theme.syntax.string && match_scope("string")) theme.syntax.string = *fg;
            if (!theme.syntax.keyword && match_scope("keyword")) theme.syntax.keyword = *fg;
            if (!theme.syntax.number && (match_scope("constant.numeric") || match_scope("number"))) theme.syntax.number = *fg;
            if (!theme.syntax.function && (match_scope("entity.name.function") || match_scope("support.function"))) theme.syntax.function = *fg;
            if (!theme.syntax.type && (match_scope("entity.name.type") || match_scope("entity.name.class"))) theme.syntax.type = *fg;
            if (!theme.syntax.variable && match_scope("variable")) theme.syntax.variable = *fg;
            if (!theme.syntax.constant && match_scope("constant")) theme.syntax.constant = *fg;
            if (!theme.syntax.operator_color && match_scope("operator")) theme.syntax.operator_color = *fg;
            if (!theme.syntax.punctuation && match_scope("punctuation")) theme.syntax.punctuation = *fg;
        }
    }

    return theme;
}

std::string export_vscode(const Theme& theme) {
    JsonValue root;
    root.type = JsonType::Object;

    root["name"] = JsonValue(theme.metadata.name.empty() ? "Custom" : theme.metadata.name);
    root["type"] = JsonValue(theme.is_dark() ? "dark" : "light");

    JsonValue colors;
    colors.type = JsonType::Object;

    colors["editor.background"] = JsonValue(to_hex(theme.ui.background));
    colors["editor.foreground"] = JsonValue(to_hex(theme.ui.foreground));
    colors["editorCursor.foreground"] = JsonValue(to_hex(theme.ui.get_cursor()));
    colors["editor.selectionBackground"] = JsonValue(to_hex(theme.ui.get_selection_background()));
    colors["editor.selectionForeground"] = JsonValue(to_hex(theme.ui.get_selection_foreground()));

    if (theme.ui.active_line.has_value()) {
        colors["editor.lineHighlightBackground"] = JsonValue(to_hex(*theme.ui.active_line));
    }
    if (theme.ui.line_number.has_value()) {
        colors["editorLineNumber.foreground"] = JsonValue(to_hex(*theme.ui.line_number));
    }
    if (theme.ui.status_bar.has_value()) {
        colors["statusBar.background"] = JsonValue(to_hex(*theme.ui.status_bar));
    }
    if (theme.ui.border.has_value()) {
        colors["sideBar.border"] = JsonValue(to_hex(*theme.ui.border));
        colors["editorGroup.border"] = JsonValue(to_hex(*theme.ui.border));
    }

    colors["terminal.background"] = JsonValue(to_hex(theme.ui.background));
    colors["terminal.foreground"] = JsonValue(to_hex(theme.ui.foreground));

    colors["terminal.ansiBlack"] = JsonValue(to_hex(theme.ansi[0]));
    colors["terminal.ansiRed"] = JsonValue(to_hex(theme.ansi[1]));
    colors["terminal.ansiGreen"] = JsonValue(to_hex(theme.ansi[2]));
    colors["terminal.ansiYellow"] = JsonValue(to_hex(theme.ansi[3]));
    colors["terminal.ansiBlue"] = JsonValue(to_hex(theme.ansi[4]));
    colors["terminal.ansiMagenta"] = JsonValue(to_hex(theme.ansi[5]));
    colors["terminal.ansiCyan"] = JsonValue(to_hex(theme.ansi[6]));
    colors["terminal.ansiWhite"] = JsonValue(to_hex(theme.ansi[7]));

    colors["terminal.ansiBrightBlack"] = JsonValue(to_hex(theme.ansi[8]));
    colors["terminal.ansiBrightRed"] = JsonValue(to_hex(theme.ansi[9]));
    colors["terminal.ansiBrightGreen"] = JsonValue(to_hex(theme.ansi[10]));
    colors["terminal.ansiBrightYellow"] = JsonValue(to_hex(theme.ansi[11]));
    colors["terminal.ansiBrightBlue"] = JsonValue(to_hex(theme.ansi[12]));
    colors["terminal.ansiBrightMagenta"] = JsonValue(to_hex(theme.ansi[13]));
    colors["terminal.ansiBrightCyan"] = JsonValue(to_hex(theme.ansi[14]));
    colors["terminal.ansiBrightWhite"] = JsonValue(to_hex(theme.ansi[15]));

    root["colors"] = std::move(colors);

    // Token colors
    JsonValue token_colors;
    token_colors.type = JsonType::Array;

    auto add_token = [&](const char* scope_name, const std::optional<Color>& color) {
        if (!color.has_value()) return;
        JsonValue item;
        item.type = JsonType::Object;
        item["scope"] = JsonValue(scope_name);
        JsonValue settings;
        settings.type = JsonType::Object;
        settings["foreground"] = JsonValue(to_hex(*color));
        item["settings"] = std::move(settings);
        token_colors.arr_val.push_back(std::move(item));
    };

    add_token("comment", theme.syntax.comment);
    add_token("string", theme.syntax.string);
    add_token("keyword", theme.syntax.keyword);
    add_token("constant.numeric", theme.syntax.number);
    add_token("entity.name.function", theme.syntax.function);
    add_token("entity.name.type", theme.syntax.type);
    add_token("variable", theme.syntax.variable);
    add_token("constant", theme.syntax.constant);
    add_token("keyword.operator", theme.syntax.operator_color);
    add_token("punctuation", theme.syntax.punctuation);

    root["tokenColors"] = std::move(token_colors);

    return root.dump(2);
}

} // namespace bro::themes::detail

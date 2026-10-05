#include <brothemes/theme.h>
#include <brothemes/contrast.h>

namespace bro::themes {

bool Theme::is_dark() const noexcept {
    if (metadata.is_dark.has_value()) {
        return *metadata.is_dark;
    }
    return relative_luminance(ui.background) < 0.5f;
}

void Theme::normalize() {
    bool dark = is_dark();
    if (!metadata.is_dark.has_value()) {
        metadata.is_dark = dark;
    }

    if (!ui.cursor.has_value()) {
        ui.cursor = ui.foreground;
    }
    if (!ui.cursor_text.has_value()) {
        ui.cursor_text = ui.background;
    }
    if (!ui.selection_background.has_value()) {
        ui.selection_background = dark ? Color(60, 60, 80) : Color(200, 200, 220);
    }
    if (!ui.selection_foreground.has_value()) {
        ui.selection_foreground = ui.foreground;
    }
    if (!ui.border.has_value()) {
        ui.border = ansi.bright_black();
    }
    if (!ui.status_bar.has_value()) {
        ui.status_bar = ui.background;
    }
    if (!ui.line_number.has_value()) {
        ui.line_number = ansi.bright_black();
    }
    if (!ui.active_line.has_value()) {
        ui.active_line = dark ? Color(40, 40, 50) : Color(240, 240, 245);
    }
    if (!ui.match_highlight.has_value()) {
        ui.match_highlight = ansi.yellow();
    }
    if (!ui.search_match.has_value()) {
        ui.search_match = ansi.bright_yellow();
    }
    if (!ui.tab_bar.has_value()) {
        ui.tab_bar = ui.background;
    }
    if (!ui.split_divider.has_value()) {
        ui.split_divider = ansi.bright_black();
    }

    // Syntax token defaults
    if (!syntax.comment.has_value()) syntax.comment = ansi.bright_black();
    if (!syntax.string.has_value()) syntax.string = ansi.green();
    if (!syntax.keyword.has_value()) syntax.keyword = ansi.magenta();
    if (!syntax.number.has_value()) syntax.number = ansi.yellow();
    if (!syntax.function.has_value()) syntax.function = ansi.blue();
    if (!syntax.type.has_value()) syntax.type = ansi.cyan();
    if (!syntax.variable.has_value()) syntax.variable = ansi.red();
    if (!syntax.constant.has_value()) syntax.constant = ansi.bright_yellow();
    if (!syntax.operator_color.has_value()) syntax.operator_color = ansi.cyan();
    if (!syntax.punctuation.has_value()) syntax.punctuation = ui.foreground;
    if (!syntax.error.has_value()) syntax.error = ansi.bright_red();
    if (!syntax.warning.has_value()) syntax.warning = ansi.bright_yellow();
    if (!syntax.info.has_value()) syntax.info = ansi.bright_blue();
    if (!syntax.hint.has_value()) syntax.hint = ansi.bright_cyan();
    if (!syntax.markup_heading.has_value()) syntax.markup_heading = ansi.blue();
    if (!syntax.markup_link.has_value()) syntax.markup_link = ansi.cyan();
    if (!syntax.markup_code.has_value()) syntax.markup_code = ansi.green();
}

} // namespace bro::themes

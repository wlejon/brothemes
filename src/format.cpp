#include <brothemes/format.h>
#include "formats/iterm.h"
#include "formats/windows_terminal.h"
#include "formats/alacritty.h"
#include "formats/kitty.h"
#include "formats/ghostty.h"
#include "formats/base16.h"
#include "formats/vscode.h"
#include <fstream>
#include <sstream>

namespace bro::themes {

std::string_view format_name(ThemeFormat format) noexcept {
    switch (format) {
        case ThemeFormat::Auto: return "Auto";
        case ThemeFormat::Iterm: return "iTerm2";
        case ThemeFormat::WindowsTerminal: return "Windows Terminal";
        case ThemeFormat::AlacrittyToml: return "Alacritty (TOML)";
        case ThemeFormat::AlacrittyYaml: return "Alacritty (YAML)";
        case ThemeFormat::Kitty: return "Kitty";
        case ThemeFormat::Ghostty: return "Ghostty";
        case ThemeFormat::Base16Yaml: return "Base16 (YAML)";
        case ThemeFormat::Base16Json: return "Base16 (JSON)";
        case ThemeFormat::Base24Yaml: return "Base24 (YAML)";
        case ThemeFormat::Base24Json: return "Base24 (JSON)";
        case ThemeFormat::VsCode: return "VS Code";
    }
    return "Unknown";
}

ThemeFormat detect_format(std::string_view content, std::string_view file_extension) {
    // Check by file extension first if provided
    if (!file_extension.empty()) {
        if (file_extension.front() == '.') {
            file_extension.remove_prefix(1);
        }
        if (file_extension == "itermcolors") {
            return ThemeFormat::Iterm;
        } else if (file_extension == "toml") {
            return ThemeFormat::AlacrittyToml;
        } else if (file_extension == "conf") {
            return ThemeFormat::Kitty;
        } else if (file_extension == "ghostty") {
            return ThemeFormat::Ghostty;
        }
    }

    // Inspect content
    if (content.find("<plist") != std::string_view::npos || content.find("<!DOCTYPE plist") != std::string_view::npos) {
        return ThemeFormat::Iterm;
    }

    if (content.find("palette =") != std::string_view::npos || content.find("palette=") != std::string_view::npos ||
        content.find("cursor-color") != std::string_view::npos) {
        return ThemeFormat::Ghostty;
    }

    if (content.find("cursor_text_color") != std::string_view::npos ||
        content.find("active_border_color") != std::string_view::npos ||
        content.find("color0 ") != std::string_view::npos ||
        content.find("selection_foreground") != std::string_view::npos) {
        if (content.find("[colors.") == std::string_view::npos && content.find("colors:") == std::string_view::npos) {
            return ThemeFormat::Kitty;
        }
    }

    if (content.find("[colors.") != std::string_view::npos || content.find("[colors]") != std::string_view::npos) {
        return ThemeFormat::AlacrittyToml;
    }

    // JSON checks
    size_t first_non_space = content.find_first_not_of(" \t\r\n");
    if (first_non_space != std::string_view::npos && (content[first_non_space] == '{' || content[first_non_space] == '[')) {
        if (content.find("\"tokenColors\"") != std::string_view::npos || content.find("\"editor.background\"") != std::string_view::npos) {
            return ThemeFormat::VsCode;
        }
        if (content.find("\"schemes\"") != std::string_view::npos || content.find("\"cursorColor\"") != std::string_view::npos ||
            content.find("\"brightBlack\"") != std::string_view::npos) {
            return ThemeFormat::WindowsTerminal;
        }
        if (content.find("\"base00\"") != std::string_view::npos) {
            if (content.find("\"base10\"") != std::string_view::npos || content.find("\"base17\"") != std::string_view::npos) {
                return ThemeFormat::Base24Json;
            }
            return ThemeFormat::Base16Json;
        }
    }

    // YAML checks
    if (content.find("base00:") != std::string_view::npos || content.find("scheme:") != std::string_view::npos) {
        if (content.find("base10:") != std::string_view::npos || content.find("base17:") != std::string_view::npos) {
            return ThemeFormat::Base24Yaml;
        }
        return ThemeFormat::Base16Yaml;
    }

    if (content.find("colors:") != std::string_view::npos) {
        return ThemeFormat::AlacrittyYaml;
    }

    return ThemeFormat::Auto;
}

std::optional<Theme> import_theme(std::string_view content, ThemeFormat format) {
    if (format == ThemeFormat::Auto) {
        format = detect_format(content);
    }

    switch (format) {
        case ThemeFormat::Iterm:
            return detail::import_iterm(content);
        case ThemeFormat::WindowsTerminal:
            return detail::import_windows_terminal(content);
        case ThemeFormat::AlacrittyToml:
        case ThemeFormat::AlacrittyYaml:
            return detail::import_alacritty(content);
        case ThemeFormat::Kitty:
            return detail::import_kitty(content);
        case ThemeFormat::Ghostty:
            return detail::import_ghostty(content);
        case ThemeFormat::Base16Yaml:
        case ThemeFormat::Base24Yaml:
            return detail::import_base16_yaml(content);
        case ThemeFormat::Base16Json:
        case ThemeFormat::Base24Json:
            return detail::import_base16_json(content);
        case ThemeFormat::VsCode:
            return detail::import_vscode(content);
        case ThemeFormat::Auto: {
            // Try all importers sequentially as fallback
            if (auto t = detail::import_windows_terminal(content)) return t;
            if (auto t = detail::import_vscode(content)) return t;
            if (auto t = detail::import_iterm(content)) return t;
            if (auto t = detail::import_alacritty(content)) return t;
            if (auto t = detail::import_kitty(content)) return t;
            if (auto t = detail::import_ghostty(content)) return t;
            if (auto t = detail::import_base16_yaml(content)) return t;
            if (auto t = detail::import_base16_json(content)) return t;
            return std::nullopt;
        }
    }

    return std::nullopt;
}

std::optional<Theme> load_theme_file(const std::string& path, ThemeFormat format) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) return std::nullopt;

    std::ostringstream ss;
    ss << file.rdbuf();
    std::string content = ss.str();

    if (format == ThemeFormat::Auto) {
        size_t dot = path.rfind('.');
        std::string ext = (dot != std::string::npos) ? path.substr(dot) : "";
        format = detect_format(content, ext);
    }

    auto theme = import_theme(content, format);
    if (theme.has_value() && theme->metadata.name.empty()) {
        // Use filename stem as theme name if not set
        size_t sep = path.find_last_of("/\\");
        std::string filename = (sep != std::string::npos) ? path.substr(sep + 1) : path;
        size_t dot = filename.rfind('.');
        theme->metadata.name = (dot != std::string::npos) ? filename.substr(0, dot) : filename;
    }
    return theme;
}

std::string export_theme(const Theme& theme, ThemeFormat format) {
    switch (format) {
        case ThemeFormat::Iterm:
            return detail::export_iterm(theme);
        case ThemeFormat::WindowsTerminal:
        case ThemeFormat::Auto:
            return detail::export_windows_terminal(theme);
        case ThemeFormat::AlacrittyToml:
            return detail::export_alacritty_toml(theme);
        case ThemeFormat::AlacrittyYaml:
            return detail::export_alacritty_yaml(theme);
        case ThemeFormat::Kitty:
            return detail::export_kitty(theme);
        case ThemeFormat::Ghostty:
            return detail::export_ghostty(theme);
        case ThemeFormat::Base16Yaml:
            return detail::export_base16_yaml(theme);
        case ThemeFormat::Base16Json:
            return detail::export_base16_json(theme);
        case ThemeFormat::Base24Yaml:
            return detail::export_base24_yaml(theme);
        case ThemeFormat::Base24Json:
            return detail::export_base24_json(theme);
        case ThemeFormat::VsCode:
            return detail::export_vscode(theme);
    }
    return detail::export_windows_terminal(theme);
}

bool save_theme_file(const Theme& theme, const std::string& path, ThemeFormat format) {
    if (format == ThemeFormat::Auto) {
        size_t dot = path.rfind('.');
        std::string ext = (dot != std::string::npos) ? path.substr(dot) : "";
        format = detect_format("", ext);
        if (format == ThemeFormat::Auto) {
            format = ThemeFormat::WindowsTerminal;
        }
    }

    std::string text = export_theme(theme, format);
    std::ofstream file(path, std::ios::binary);
    if (!file.is_open()) return false;

    file.write(text.data(), static_cast<std::streamsize>(text.size()));
    return file.good();
}

} // namespace bro::themes

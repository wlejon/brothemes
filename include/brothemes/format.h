#pragma once

#include <brothemes/theme.h>
#include <optional>
#include <string>
#include <string_view>

namespace bro::themes {

enum class ThemeFormat {
    Auto,             // Auto-detect from content or extension
    Iterm,            // iTerm2 .itermcolors (XML plist)
    WindowsTerminal,  // Windows Terminal JSON
    AlacrittyToml,    // Alacritty TOML config
    AlacrittyYaml,    // Alacritty YAML config
    Kitty,            // kitty.conf key-value
    Ghostty,          // Ghostty theme format
    Base16Yaml,       // Base16 YAML scheme
    Base16Json,       // Base16 JSON scheme
    Base24Yaml,       // Base24 YAML scheme
    Base24Json,       // Base24 JSON scheme
    VsCode            // VS Code theme JSON
};

// Detect format based on content inspection and optional file extension
ThemeFormat detect_format(std::string_view content, std::string_view file_extension = std::string_view());

// Import a theme from text content in the specified or auto-detected format
std::optional<Theme> import_theme(std::string_view content, ThemeFormat format = ThemeFormat::Auto);

// Load and parse a theme from a file
std::optional<Theme> load_theme_file(const std::string& path, ThemeFormat format = ThemeFormat::Auto);

// Export a theme to string in the specified format
std::string export_theme(const Theme& theme, ThemeFormat format);

// Export and write a theme to a file in the specified format
bool save_theme_file(const Theme& theme, const std::string& path, ThemeFormat format);

// Format names for display/logging
std::string_view format_name(ThemeFormat format) noexcept;

} // namespace bro::themes

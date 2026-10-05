#pragma once

#include <brothemes/theme.h>
#include <optional>
#include <string>
#include <string_view>

namespace bro::themes::detail {

std::optional<Theme> import_alacritty(std::string_view content);
std::string export_alacritty_toml(const Theme& theme);
std::string export_alacritty_yaml(const Theme& theme);

} // namespace bro::themes::detail

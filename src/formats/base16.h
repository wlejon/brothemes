#pragma once

#include <brothemes/theme.h>
#include <optional>
#include <string>
#include <string_view>

namespace bro::themes::detail {

std::optional<Theme> import_base16_yaml(std::string_view content);
std::optional<Theme> import_base16_json(std::string_view content);

std::string export_base16_yaml(const Theme& theme);
std::string export_base16_json(const Theme& theme);
std::string export_base24_yaml(const Theme& theme);
std::string export_base24_json(const Theme& theme);

} // namespace bro::themes::detail

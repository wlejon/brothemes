#pragma once

#include <brothemes/theme.h>
#include <optional>
#include <string>
#include <string_view>

namespace bro::themes::detail {

std::optional<Theme> import_iterm(std::string_view content);
std::string export_iterm(const Theme& theme);

} // namespace bro::themes::detail

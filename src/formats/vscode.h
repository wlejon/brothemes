#pragma once

#include <brothemes/theme.h>
#include <optional>
#include <string>
#include <string_view>

namespace bro::themes::detail {

std::optional<Theme> import_vscode(std::string_view content);
std::string export_vscode(const Theme& theme);

} // namespace bro::themes::detail

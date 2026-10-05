#pragma once

#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace bro::themes::detail {

struct KeyValuePair {
    std::string key;
    std::string value;
};

// Parses a line-oriented config file (e.g. kitty.conf or Ghostty config).
// Supports comments starting with '#', whitespace/equals separators, and quoted values.
std::vector<KeyValuePair> parse_key_value_lines(std::string_view text);

// Strip quotes ('"' or '\'') from a string if present
std::string_view unquote(std::string_view sv) noexcept;

// Trim whitespace
std::string_view trim(std::string_view sv) noexcept;

} // namespace bro::themes::detail

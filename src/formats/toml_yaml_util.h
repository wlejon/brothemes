#pragma once

#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace bro::themes::detail {

// A dictionary of dotted key paths to string values, parsed from TOML or YAML.
// Example:
// "colors.primary.background" -> "#282a36"
// "colors.normal.black" -> "#21222c"
// "base00" -> "#282a36"
// "scheme" -> "Dracula"
class ConfigMap {
public:
    std::map<std::string, std::string> entries;

    const std::string* get(std::string_view key) const;
    std::string get_or(std::string_view key, std::string_view def) const;
    bool has(std::string_view key) const;

    static ConfigMap parse_toml(std::string_view toml_text);
    static ConfigMap parse_yaml(std::string_view yaml_text);
};

} // namespace bro::themes::detail

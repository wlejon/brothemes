#include "toml_yaml_util.h"
#include "kv_util.h"
#include <algorithm>
#include <cctype>

namespace bro::themes::detail {

namespace {

std::string to_lower(std::string_view sv) {
    std::string s;
    s.reserve(sv.size());
    for (char c : sv) {
        s.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }
    return s;
}

std::string clean_value(std::string_view sv) {
    sv = trim(sv);
    sv = unquote(sv);
    return std::string(sv);
}

// Strip comments taking quotes into account
std::string_view strip_comments(std::string_view line) {
    bool in_single = false;
    bool in_double = false;
    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (c == '"' && !in_single) {
            in_double = !in_double;
        } else if (c == '\'' && !in_double) {
            in_single = !in_single;
        } else if (c == '#' && !in_single && !in_double) {
            // Check if it's the start of a comment (e.g. at line start or preceded by whitespace)
            // or if it's an unquoted hex color like #282a36
            bool is_hex_color = false;
            if (i > 0 && (line[i - 1] == '=' || line[i - 1] == ':' || std::isspace(static_cast<unsigned char>(line[i - 1])))) {
                // If followed by valid hex characters, treat as color value
                if (i + 1 < line.size() && std::isxdigit(static_cast<unsigned char>(line[i + 1]))) {
                    is_hex_color = true;
                }
            }
            if (!is_hex_color) {
                return line.substr(0, i);
            }
        }
    }
    return line;
}

} // namespace

const std::string* ConfigMap::get(std::string_view key) const {
    std::string lower_k = to_lower(key);
    auto it = entries.find(lower_k);
    if (it != entries.end()) return &it->second;
    return nullptr;
}

std::string ConfigMap::get_or(std::string_view key, std::string_view def) const {
    const std::string* val = get(key);
    return val ? *val : std::string(def);
}

bool ConfigMap::has(std::string_view key) const {
    return get(key) != nullptr;
}

ConfigMap ConfigMap::parse_toml(std::string_view toml_text) {
    ConfigMap map;
    std::string current_section;

    size_t start = 0;
    while (start < toml_text.size()) {
        size_t end = toml_text.find('\n', start);
        std::string_view line = (end == std::string_view::npos) ? toml_text.substr(start) : toml_text.substr(start, end - start);
        start = (end == std::string_view::npos) ? toml_text.size() : end + 1;

        line = strip_comments(line);
        line = trim(line);
        if (line.empty()) continue;

        // Check for section header [section.sub]
        if (line.front() == '[' && line.back() == ']') {
            std::string_view sec = trim(line.substr(1, line.size() - 2));
            current_section = to_lower(sec);
            continue;
        }

        // Key = value
        size_t eq = line.find('=');
        if (eq == std::string_view::npos) continue;

        std::string_view key_sv = trim(line.substr(0, eq));
        std::string_view val_sv = trim(line.substr(eq + 1));
        val_sv = unquote(val_sv);

        // Check if val is inline table: e.g. { text = "#fff", background = "#000" }
        if (val_sv.starts_with('{') && val_sv.ends_with('}')) {
            std::string_view inner = trim(val_sv.substr(1, val_sv.size() - 2));
            size_t in_start = 0;
            while (in_start < inner.size()) {
                size_t in_end = inner.find(',', in_start);
                std::string_view pair = (in_end == std::string_view::npos) ? inner.substr(in_start) : inner.substr(in_start, in_end - in_start);
                in_start = (in_end == std::string_view::npos) ? inner.size() : in_end + 1;

                size_t in_eq = pair.find('=');
                if (in_eq == std::string_view::npos) continue;
                std::string_view sub_k = trim(pair.substr(0, in_eq));
                std::string_view sub_v = trim(pair.substr(in_eq + 1));
                sub_v = unquote(sub_v);

                std::string full_k = current_section.empty() ? to_lower(key_sv) : current_section + "." + to_lower(key_sv);
                full_k += "." + to_lower(sub_k);
                map.entries[full_k] = std::string(sub_v);
            }
            continue;
        }

        std::string full_key;
        if (!current_section.empty()) {
            full_key = current_section + "." + to_lower(key_sv);
        } else {
            full_key = to_lower(key_sv);
        }
        map.entries[full_key] = std::string(val_sv);
    }

    return map;
}

ConfigMap ConfigMap::parse_yaml(std::string_view yaml_text) {
    ConfigMap map;

    struct IndentLevel {
        int indent{0};
        std::string key;
    };
    std::vector<IndentLevel> stack;

    size_t start = 0;
    while (start < yaml_text.size()) {
        size_t end = yaml_text.find('\n', start);
        std::string_view line = (end == std::string_view::npos) ? yaml_text.substr(start) : yaml_text.substr(start, end - start);
        start = (end == std::string_view::npos) ? yaml_text.size() : end + 1;

        line = strip_comments(line);
        if (trim(line).empty()) continue;

        // Count leading spaces
        int indent = 0;
        while (indent < static_cast<int>(line.size()) && line[indent] == ' ') {
            indent++;
        }
        std::string_view content = line.substr(indent);

        // Find ':'
        size_t colon = content.find(':');
        if (colon == std::string_view::npos) continue;

        std::string_view key_sv = trim(content.substr(0, colon));
        std::string_view val_sv = trim(content.substr(colon + 1));
        key_sv = unquote(key_sv);
        val_sv = unquote(val_sv);

        // Pop stack to match current indentation
        while (!stack.empty() && stack.back().indent >= indent) {
            stack.pop_back();
        }

        if (val_sv.empty()) {
            // New nesting level
            stack.push_back({indent, to_lower(key_sv)});
        } else {
            // Value line
            std::string full_key;
            for (const auto& item : stack) {
                full_key += item.key + ".";
            }
            full_key += to_lower(key_sv);
            map.entries[full_key] = clean_value(val_sv);
        }
    }

    return map;
}

} // namespace bro::themes::detail

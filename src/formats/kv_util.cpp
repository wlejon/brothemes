#include "kv_util.h"
#include <cctype>

namespace bro::themes::detail {

std::string_view trim(std::string_view sv) noexcept {
    while (!sv.empty() && std::isspace(static_cast<unsigned char>(sv.front()))) {
        sv.remove_prefix(1);
    }
    while (!sv.empty() && std::isspace(static_cast<unsigned char>(sv.back()))) {
        sv.remove_suffix(1);
    }
    return sv;
}

std::string_view unquote(std::string_view sv) noexcept {
    sv = trim(sv);
    if (sv.size() >= 2) {
        if ((sv.front() == '"' && sv.back() == '"') ||
            (sv.front() == '\'' && sv.back() == '\'')) {
            sv.remove_prefix(1);
            sv.remove_suffix(1);
        }
    }
    return sv;
}

std::vector<KeyValuePair> parse_key_value_lines(std::string_view text) {
    std::vector<KeyValuePair> result;

    size_t start = 0;
    while (start < text.size()) {
        size_t end = text.find('\n', start);
        std::string_view line = (end == std::string_view::npos) ? text.substr(start) : text.substr(start, end - start);
        if (end == std::string_view::npos) {
            start = text.size();
        } else {
            start = end + 1;
        }

        line = trim(line);
        if (line.empty() || line.front() == '#') continue;

        // Split key and value on '=' or whitespace
        size_t sep_pos = std::string_view::npos;
        size_t eq_pos = line.find('=');
        if (eq_pos != std::string_view::npos) {
            sep_pos = eq_pos;
        } else {
            for (size_t i = 0; i < line.size(); ++i) {
                if (std::isspace(static_cast<unsigned char>(line[i]))) {
                    sep_pos = i;
                    break;
                }
            }
        }

        if (sep_pos == std::string_view::npos) {
            continue;
        }

        std::string_view key = trim(line.substr(0, sep_pos));
        std::string_view val = trim(line.substr(sep_pos + 1));

        // Strip trailing comment if separated by space and not part of hex
        size_t hash_pos = val.find('#');
        if (hash_pos != std::string_view::npos && hash_pos > 0) {
            if (std::isspace(static_cast<unsigned char>(val[hash_pos - 1]))) {
                val = trim(val.substr(0, hash_pos));
            }
        }

        val = unquote(val);

        if (!key.empty()) {
            result.push_back({std::string(key), std::string(val)});
        }
    }

    return result;
}

} // namespace bro::themes::detail

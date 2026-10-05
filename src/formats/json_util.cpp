#include "json_util.h"
#include <cctype>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <sstream>

namespace bro::themes::detail {

namespace {

class JsonParser {
public:
    explicit JsonParser(std::string_view input) : src_(input), pos_(0) {}

    std::optional<JsonValue> parse() {
        skip_whitespace_and_comments();
        if (pos_ >= src_.size()) return std::nullopt;
        auto val = parse_value();
        skip_whitespace_and_comments();
        return val;
    }

private:
    std::string_view src_;
    size_t pos_{0};

    char peek() const noexcept {
        return pos_ < src_.size() ? src_[pos_] : '\0';
    }

    char advance() noexcept {
        return pos_ < src_.size() ? src_[pos_++] : '\0';
    }

    void skip_whitespace_and_comments() {
        while (pos_ < src_.size()) {
            char c = src_[pos_];
            if (std::isspace(static_cast<unsigned char>(c))) {
                pos_++;
            } else if (c == '/' && pos_ + 1 < src_.size()) {
                if (src_[pos_ + 1] == '/') {
                    // Line comment
                    pos_ += 2;
                    while (pos_ < src_.size() && src_[pos_] != '\n') {
                        pos_++;
                    }
                } else if (src_[pos_ + 1] == '*') {
                    // Block comment
                    pos_ += 2;
                    while (pos_ + 1 < src_.size() && !(src_[pos_] == '*' && src_[pos_ + 1] == '/')) {
                        pos_++;
                    }
                    if (pos_ + 1 < src_.size()) {
                        pos_ += 2;
                    }
                } else {
                    break;
                }
            } else if (c == '#') {
                // Shell-style / YAML-style comment tolerated in JSON5
                pos_++;
                while (pos_ < src_.size() && src_[pos_] != '\n') {
                    pos_++;
                }
            } else {
                break;
            }
        }
    }

    std::optional<JsonValue> parse_value() {
        skip_whitespace_and_comments();
        if (pos_ >= src_.size()) return std::nullopt;

        char c = peek();
        if (c == '{') return parse_object();
        if (c == '[') return parse_array();
        if (c == '"' || c == '\'') return parse_string();
        if (c == 't' || c == 'T' || c == 'f' || c == 'F') return parse_bool();
        if (c == 'n' || c == 'N') return parse_null();
        if (c == '-' || c == '+' || (c >= '0' && c <= '9')) return parse_number();

        return std::nullopt;
    }

    std::optional<JsonValue> parse_object() {
        if (advance() != '{') return std::nullopt;

        std::map<std::string, JsonValue> obj;
        skip_whitespace_and_comments();

        if (peek() == '}') {
            advance();
            return JsonValue(std::move(obj));
        }

        while (pos_ < src_.size()) {
            skip_whitespace_and_comments();
            if (peek() == '}') {
                advance();
                return JsonValue(std::move(obj));
            }

            // Key can be quoted or unquoted identifier
            std::string key;
            if (peek() == '"' || peek() == '\'') {
                auto k_opt = parse_string_raw();
                if (!k_opt.has_value()) return std::nullopt;
                key = std::move(*k_opt);
            } else {
                // unquoted key
                size_t start = pos_;
                while (pos_ < src_.size() && (std::isalnum(static_cast<unsigned char>(src_[pos_])) ||
                       src_[pos_] == '_' || src_[pos_] == '-' || src_[pos_] == '.')) {
                    pos_++;
                }
                if (start == pos_) return std::nullopt;
                key = std::string(src_.substr(start, pos_ - start));
            }

            skip_whitespace_and_comments();
            if (advance() != ':') return std::nullopt;

            skip_whitespace_and_comments();
            auto val = parse_value();
            if (!val.has_value()) return std::nullopt;

            obj.emplace(std::move(key), std::move(*val));

            skip_whitespace_and_comments();
            if (peek() == ',') {
                advance();
                skip_whitespace_and_comments();
                if (peek() == '}') { // trailing comma allowed
                    advance();
                    return JsonValue(std::move(obj));
                }
            } else if (peek() == '}') {
                advance();
                return JsonValue(std::move(obj));
            } else {
                return std::nullopt;
            }
        }

        return std::nullopt;
    }

    std::optional<JsonValue> parse_array() {
        if (advance() != '[') return std::nullopt;

        std::vector<JsonValue> arr;
        skip_whitespace_and_comments();

        if (peek() == ']') {
            advance();
            return JsonValue(std::move(arr));
        }

        while (pos_ < src_.size()) {
            skip_whitespace_and_comments();
            if (peek() == ']') {
                advance();
                return JsonValue(std::move(arr));
            }

            auto val = parse_value();
            if (!val.has_value()) return std::nullopt;
            arr.push_back(std::move(*val));

            skip_whitespace_and_comments();
            if (peek() == ',') {
                advance();
                skip_whitespace_and_comments();
                if (peek() == ']') { // trailing comma allowed
                    advance();
                    return JsonValue(std::move(arr));
                }
            } else if (peek() == ']') {
                advance();
                return JsonValue(std::move(arr));
            } else {
                return std::nullopt;
            }
        }

        return std::nullopt;
    }

    std::optional<std::string> parse_string_raw() {
        char quote = advance();
        if (quote != '"' && quote != '\'') return std::nullopt;

        std::string res;
        while (pos_ < src_.size()) {
            char c = advance();
            if (c == quote) {
                return res;
            } else if (c == '\\') {
                if (pos_ >= src_.size()) return std::nullopt;
                char esc = advance();
                switch (esc) {
                    case '"': res.push_back('"'); break;
                    case '\'': res.push_back('\''); break;
                    case '\\': res.push_back('\\'); break;
                    case '/': res.push_back('/'); break;
                    case 'b': res.push_back('\b'); break;
                    case 'f': res.push_back('\f'); break;
                    case 'n': res.push_back('\n'); break;
                    case 'r': res.push_back('\r'); break;
                    case 't': res.push_back('\t'); break;
                    case 'u': {
                        // 4 hex digits
                        if (pos_ + 4 > src_.size()) return std::nullopt;
                        uint32_t code = 0;
                        for (int i = 0; i < 4; ++i) {
                            char h = advance();
                            int d = 0;
                            if (h >= '0' && h <= '9') d = h - '0';
                            else if (h >= 'a' && h <= 'f') d = h - 'a' + 10;
                            else if (h >= 'A' && h <= 'F') d = h - 'A' + 10;
                            else return std::nullopt;
                            code = (code << 4) | d;
                        }
                        if (code < 0x80) {
                            res.push_back(static_cast<char>(code));
                        } else if (code < 0x800) {
                            res.push_back(static_cast<char>(0xC0 | (code >> 6)));
                            res.push_back(static_cast<char>(0x80 | (code & 0x3F)));
                        } else {
                            res.push_back(static_cast<char>(0xE0 | (code >> 12)));
                            res.push_back(static_cast<char>(0x80 | ((code >> 6) & 0x3F)));
                            res.push_back(static_cast<char>(0x80 | (code & 0x3F)));
                        }
                        break;
                    }
                    default:
                        res.push_back(esc);
                        break;
                }
            } else {
                res.push_back(c);
            }
        }
        return std::nullopt;
    }

    std::optional<JsonValue> parse_string() {
        auto str = parse_string_raw();
        if (!str.has_value()) return std::nullopt;
        return JsonValue(std::move(*str));
    }

    std::optional<JsonValue> parse_number() {
        size_t start = pos_;
        if (peek() == '-' || peek() == '+') pos_++;
        while (pos_ < src_.size() && (std::isdigit(static_cast<unsigned char>(src_[pos_])) ||
               src_[pos_] == '.' || src_[pos_] == 'e' || src_[pos_] == 'E' ||
               src_[pos_] == '-' || src_[pos_] == '+')) {
            // Check for +/- only after e/E
            if ((src_[pos_] == '-' || src_[pos_] == '+') && src_[pos_ - 1] != 'e' && src_[pos_ - 1] != 'E') {
                break;
            }
            pos_++;
        }

        std::string s(src_.substr(start, pos_ - start));
        char* end = nullptr;
        double val = std::strtod(s.c_str(), &end);
        if (end != s.c_str() + s.size()) return std::nullopt;
        return JsonValue(val);
    }

    std::optional<JsonValue> parse_bool() {
        if (src_.substr(pos_, 4) == "true" || src_.substr(pos_, 4) == "TRUE") {
            pos_ += 4;
            return JsonValue(true);
        }
        if (src_.substr(pos_, 5) == "false" || src_.substr(pos_, 5) == "FALSE") {
            pos_ += 5;
            return JsonValue(false);
        }
        return std::nullopt;
    }

    std::optional<JsonValue> parse_null() {
        if (src_.substr(pos_, 4) == "null" || src_.substr(pos_, 4) == "NULL") {
            pos_ += 4;
            return JsonValue();
        }
        return std::nullopt;
    }
};

static const JsonValue g_null_val;

} // namespace

std::optional<JsonValue> JsonValue::parse(std::string_view json) {
    JsonParser parser(json);
    return parser.parse();
}

const JsonValue* JsonValue::find(std::string_view key) const {
    if (!is_object()) return nullptr;
    auto it = obj_val.find(std::string(key));
    if (it != obj_val.end()) return &it->second;
    return nullptr;
}

const JsonValue& JsonValue::operator[](std::string_view key) const {
    const JsonValue* p = find(key);
    return p ? *p : g_null_val;
}

JsonValue& JsonValue::operator[](std::string_view key) {
    if (!is_object()) {
        type = JsonType::Object;
        obj_val.clear();
    }
    return obj_val[std::string(key)];
}

std::string JsonValue::dump(int indent, int current_indent) const {
    std::ostringstream ss;
    std::string ind(current_indent, ' ');
    std::string next_ind(current_indent + indent, ' ');

    switch (type) {
        case JsonType::Null:
            return "null";
        case JsonType::Bool:
            return bool_val ? "true" : "false";
        case JsonType::Number: {
            if (std::floor(num_val) == num_val && !std::isinf(num_val)) {
                return std::to_string(static_cast<int64_t>(num_val));
            }
            char buf[64];
            std::snprintf(buf, sizeof(buf), "%.6g", num_val);
            return std::string(buf);
        }
        case JsonType::String: {
            ss << '"';
            for (char c : str_val) {
                switch (c) {
                    case '"': ss << "\\\""; break;
                    case '\\': ss << "\\\\"; break;
                    case '\b': ss << "\\b"; break;
                    case '\f': ss << "\\f"; break;
                    case '\n': ss << "\\n"; break;
                    case '\r': ss << "\\r"; break;
                    case '\t': ss << "\\t"; break;
                    default:
                        if (static_cast<unsigned char>(c) < 0x20) {
                            char ubuf[16];
                            std::snprintf(ubuf, sizeof(ubuf), "\\u%04x", static_cast<unsigned char>(c));
                            ss << ubuf;
                        } else {
                            ss << c;
                        }
                        break;
                }
            }
            ss << '"';
            return ss.str();
        }
        case JsonType::Array: {
            if (arr_val.empty()) return "[]";
            ss << "[\n";
            for (size_t i = 0; i < arr_val.size(); ++i) {
                ss << next_ind << arr_val[i].dump(indent, current_indent + indent);
                if (i + 1 < arr_val.size()) ss << ",";
                ss << "\n";
            }
            ss << ind << "]";
            return ss.str();
        }
        case JsonType::Object: {
            if (obj_val.empty()) return "{}";
            ss << "{\n";
            size_t i = 0;
            for (const auto& [k, v] : obj_val) {
                ss << next_ind << '"' << k << "\": " << v.dump(indent, current_indent + indent);
                if (++i < obj_val.size()) ss << ",";
                ss << "\n";
            }
            ss << ind << "}";
            return ss.str();
        }
    }
    return "null";
}

} // namespace bro::themes::detail

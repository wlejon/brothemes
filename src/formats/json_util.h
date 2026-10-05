#pragma once

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace bro::themes::detail {

enum class JsonType {
    Null,
    Bool,
    Number,
    String,
    Array,
    Object
};

class JsonValue {
public:
    JsonType type{JsonType::Null};

    bool bool_val{false};
    double num_val{0.0};
    std::string str_val;
    std::vector<JsonValue> arr_val;
    std::map<std::string, JsonValue> obj_val;

    JsonValue() = default;
    explicit JsonValue(bool b) : type(JsonType::Bool), bool_val(b) {}
    explicit JsonValue(double n) : type(JsonType::Number), num_val(n) {}
    explicit JsonValue(int n) : type(JsonType::Number), num_val(n) {}
    explicit JsonValue(int64_t n) : type(JsonType::Number), num_val(static_cast<double>(n)) {}
    explicit JsonValue(std::string s) : type(JsonType::String), str_val(std::move(s)) {}
    explicit JsonValue(const char* s) : type(JsonType::String), str_val(s) {}
    explicit JsonValue(std::string_view s) : type(JsonType::String), str_val(s) {}
    explicit JsonValue(std::vector<JsonValue> a) : type(JsonType::Array), arr_val(std::move(a)) {}
    explicit JsonValue(std::map<std::string, JsonValue> o) : type(JsonType::Object), obj_val(std::move(o)) {}

    static std::optional<JsonValue> parse(std::string_view json);
    std::string dump(int indent = 2, int current_indent = 0) const;

    bool is_null() const noexcept { return type == JsonType::Null; }
    bool is_bool() const noexcept { return type == JsonType::Bool; }
    bool is_number() const noexcept { return type == JsonType::Number; }
    bool is_string() const noexcept { return type == JsonType::String; }
    bool is_array() const noexcept { return type == JsonType::Array; }
    bool is_object() const noexcept { return type == JsonType::Object; }

    const std::string& as_string() const noexcept { return str_val; }
    double as_number() const noexcept { return num_val; }
    bool as_bool() const noexcept { return bool_val; }

    const JsonValue* find(std::string_view key) const;
    const JsonValue& operator[](std::string_view key) const;
    JsonValue& operator[](std::string_view key);
};

} // namespace bro::themes::detail

#pragma once

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace bro::themes::detail {

enum class PlistType {
    Null,
    Dict,
    Array,
    String,
    Real,
    Integer,
    Boolean
};

struct PlistNode {
    PlistType type{PlistType::Null};

    std::map<std::string, PlistNode> dict_val;
    std::vector<PlistNode> arr_val;
    std::string str_val;
    double real_val{0.0};
    int64_t int_val{0};
    bool bool_val{false};

    PlistNode() = default;
    explicit PlistNode(std::map<std::string, PlistNode> d) : type(PlistType::Dict), dict_val(std::move(d)) {}
    explicit PlistNode(std::vector<PlistNode> a) : type(PlistType::Array), arr_val(std::move(a)) {}
    explicit PlistNode(std::string s) : type(PlistType::String), str_val(std::move(s)) {}
    explicit PlistNode(double r) : type(PlistType::Real), real_val(r) {}
    explicit PlistNode(int64_t i) : type(PlistType::Integer), int_val(i) {}
    explicit PlistNode(bool b) : type(PlistType::Boolean), bool_val(b) {}

    bool is_dict() const noexcept { return type == PlistType::Dict; }
    bool is_real() const noexcept { return type == PlistType::Real || type == PlistType::Integer; }
    bool is_string() const noexcept { return type == PlistType::String; }

    double as_real() const noexcept {
        if (type == PlistType::Integer) return static_cast<double>(int_val);
        return real_val;
    }
    const std::string& as_string() const noexcept { return str_val; }

    const PlistNode* find(std::string_view key) const;

    static std::optional<PlistNode> parse(std::string_view xml);
    std::string serialize() const;
};

} // namespace bro::themes::detail

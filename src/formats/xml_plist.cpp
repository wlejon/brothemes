#include "xml_plist.h"
#include <cctype>
#include <cstdlib>
#include <sstream>

namespace bro::themes::detail {

namespace {

std::string unescape_xml(std::string_view sv) {
    std::string res;
    res.reserve(sv.size());
    for (size_t i = 0; i < sv.size(); ++i) {
        if (sv[i] == '&') {
            if (sv.substr(i, 5) == "&amp;") {
                res.push_back('&');
                i += 4;
            } else if (sv.substr(i, 4) == "&lt;") {
                res.push_back('<');
                i += 3;
            } else if (sv.substr(i, 4) == "&gt;") {
                res.push_back('>');
                i += 3;
            } else if (sv.substr(i, 6) == "&quot;") {
                res.push_back('"');
                i += 5;
            } else if (sv.substr(i, 6) == "&apos;") {
                res.push_back('\'');
                i += 5;
            } else {
                res.push_back('&');
            }
        } else {
            res.push_back(sv[i]);
        }
    }
    return res;
}

std::string escape_xml(std::string_view sv) {
    std::string res;
    res.reserve(sv.size() + 8);
    for (char c : sv) {
        switch (c) {
            case '&': res += "&amp;"; break;
            case '<': res += "&lt;"; break;
            case '>': res += "&gt;"; break;
            case '"': res += "&quot;"; break;
            case '\'': res += "&apos;"; break;
            default: res.push_back(c); break;
        }
    }
    return res;
}

class XmlPlistParser {
public:
    explicit XmlPlistParser(std::string_view src) : src_(src), pos_(0) {}

    std::optional<PlistNode> parse() {
        skip_prolog_and_comments();
        // Find top level tag
        auto tag = next_tag();
        if (!tag.has_value() || tag->name != "plist" || tag->is_closing) {
            return std::nullopt;
        }
        skip_prolog_and_comments();
        auto root = parse_element();
        return root;
    }

private:
    struct Tag {
        std::string name;
        bool is_closing{false};
        bool is_self_closing{false};
    };

    std::string_view src_;
    size_t pos_{0};

    void skip_prolog_and_comments() {
        while (pos_ < src_.size()) {
            while (pos_ < src_.size() && std::isspace(static_cast<unsigned char>(src_[pos_]))) {
                pos_++;
            }
            if (pos_ >= src_.size()) break;

            if (src_[pos_] == '<') {
                if (src_.substr(pos_, 4) == "<!--") {
                    // Comment
                    pos_ += 4;
                    size_t end = src_.find("-->", pos_);
                    if (end == std::string_view::npos) {
                        pos_ = src_.size();
                        break;
                    }
                    pos_ = end + 3;
                    continue;
                } else if (src_.substr(pos_, 2) == "<?" || src_.substr(pos_, 2) == "<!") {
                    // Processing instruction or DOCTYPE
                    pos_ += 2;
                    size_t end = src_.find('>', pos_);
                    if (end == std::string_view::npos) {
                        pos_ = src_.size();
                        break;
                    }
                    pos_ = end + 1;
                    continue;
                }
            }
            break;
        }
    }

    std::optional<Tag> next_tag() {
        skip_prolog_and_comments();
        if (pos_ >= src_.size() || src_[pos_] != '<') return std::nullopt;

        pos_++; // skip '<'
        Tag t;
        if (pos_ < src_.size() && src_[pos_] == '/') {
            t.is_closing = true;
            pos_++;
        }

        size_t start = pos_;
        while (pos_ < src_.size() && !std::isspace(static_cast<unsigned char>(src_[pos_])) &&
               src_[pos_] != '>' && src_[pos_] != '/') {
            pos_++;
        }
        t.name = std::string(src_.substr(start, pos_ - start));

        // Skip attributes until '>' or '/>'
        while (pos_ < src_.size() && src_[pos_] != '>') {
            if (src_[pos_] == '/' && pos_ + 1 < src_.size() && src_[pos_ + 1] == '>') {
                t.is_self_closing = true;
                pos_ += 2;
                return t;
            }
            pos_++;
        }
        if (pos_ < src_.size() && src_[pos_] == '>') {
            pos_++;
        }
        return t;
    }

    std::optional<std::string> read_text_until_closing(std::string_view tag_name) {
        size_t start = pos_;
        std::string close_tag = "</" + std::string(tag_name) + ">";
        size_t end = src_.find(close_tag, pos_);
        if (end == std::string_view::npos) return std::nullopt;
        pos_ = end + close_tag.size();
        return unescape_xml(src_.substr(start, end - start));
    }

    std::optional<PlistNode> parse_element() {
        auto tag = next_tag();
        if (!tag.has_value() || tag->is_closing) return std::nullopt;

        if (tag->name == "dict") {
            if (tag->is_self_closing) return PlistNode(std::map<std::string, PlistNode>{});
            std::map<std::string, PlistNode> d;
            while (true) {
                skip_prolog_and_comments();
                if (pos_ >= src_.size()) break;
                // Peek next tag
                auto next_t = next_tag();
                if (!next_t.has_value()) return std::nullopt;

                if (next_t->name == "dict" && next_t->is_closing) {
                    return PlistNode(std::move(d));
                }
                if (next_t->name != "key" || next_t->is_closing) {
                    return std::nullopt;
                }
                auto key_text = read_text_until_closing("key");
                if (!key_text.has_value()) return std::nullopt;

                skip_prolog_and_comments();
                auto val = parse_element();
                if (!val.has_value()) return std::nullopt;
                d.emplace(std::move(*key_text), std::move(*val));
            }
            return PlistNode(std::move(d));
        } else if (tag->name == "array") {
            if (tag->is_self_closing) return PlistNode(std::vector<PlistNode>{});
            std::vector<PlistNode> arr;
            while (true) {
                skip_prolog_and_comments();
                if (pos_ >= src_.size()) break;
                size_t saved_pos = pos_;
                auto next_t = next_tag();
                if (!next_t.has_value()) return std::nullopt;
                if (next_t->name == "array" && next_t->is_closing) {
                    return PlistNode(std::move(arr));
                }
                pos_ = saved_pos;
                auto elem = parse_element();
                if (!elem.has_value()) return std::nullopt;
                arr.push_back(std::move(*elem));
            }
            return PlistNode(std::move(arr));
        } else if (tag->name == "string") {
            if (tag->is_self_closing) return PlistNode(std::string{});
            auto text = read_text_until_closing("string");
            if (!text.has_value()) return std::nullopt;
            return PlistNode(std::move(*text));
        } else if (tag->name == "real") {
            if (tag->is_self_closing) return PlistNode(0.0);
            auto text = read_text_until_closing("real");
            if (!text.has_value()) return std::nullopt;
            return PlistNode(std::strtod(text->c_str(), nullptr));
        } else if (tag->name == "integer") {
            if (tag->is_self_closing) return PlistNode(static_cast<int64_t>(0));
            auto text = read_text_until_closing("integer");
            if (!text.has_value()) return std::nullopt;
            return PlistNode(static_cast<int64_t>(std::strtoll(text->c_str(), nullptr, 10)));
        } else if (tag->name == "true") {
            return PlistNode(true);
        } else if (tag->name == "false") {
            return PlistNode(false);
        }

        return std::nullopt;
    }
};

} // namespace

const PlistNode* PlistNode::find(std::string_view key) const {
    if (!is_dict()) return nullptr;
    auto it = dict_val.find(std::string(key));
    if (it != dict_val.end()) return &it->second;
    return nullptr;
}

std::optional<PlistNode> PlistNode::parse(std::string_view xml) {
    XmlPlistParser parser(xml);
    return parser.parse();
}

namespace {

void serialize_node(const PlistNode& node, std::ostringstream& ss, int indent) {
    std::string ind(indent, '\t');
    switch (node.type) {
        case PlistType::Null:
            break;
        case PlistType::Boolean:
            ss << ind << (node.bool_val ? "<true/>\n" : "<false/>\n");
            break;
        case PlistType::Integer:
            ss << ind << "<integer>" << node.int_val << "</integer>\n";
            break;
        case PlistType::Real:
            ss << ind << "<real>" << node.real_val << "</real>\n";
            break;
        case PlistType::String:
            ss << ind << "<string>" << escape_xml(node.str_val) << "</string>\n";
            break;
        case PlistType::Array:
            if (node.arr_val.empty()) {
                ss << ind << "<array/>\n";
            } else {
                ss << ind << "<array>\n";
                for (const auto& item : node.arr_val) {
                    serialize_node(item, ss, indent + 1);
                }
                ss << ind << "</array>\n";
            }
            break;
        case PlistType::Dict:
            if (node.dict_val.empty()) {
                ss << ind << "<dict/>\n";
            } else {
                ss << ind << "<dict>\n";
                for (const auto& [k, v] : node.dict_val) {
                    ss << ind << "\t<key>" << escape_xml(k) << "</key>\n";
                    serialize_node(v, ss, indent + 1);
                }
                ss << ind << "</dict>\n";
            }
            break;
    }
}

} // namespace

std::string PlistNode::serialize() const {
    std::ostringstream ss;
    ss << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    ss << "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n";
    ss << "<plist version=\"1.0\">\n";
    serialize_node(*this, ss, 0);
    ss << "</plist>\n";
    return ss.str();
}

} // namespace bro::themes::detail

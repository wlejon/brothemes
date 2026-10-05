#include "base16.h"
#include "json_util.h"
#include "toml_yaml_util.h"
#include <iomanip>
#include <sstream>

namespace bro::themes::detail {

namespace {

struct BaseSlots {
    std::optional<Color> base[24];
    std::string name;
    std::string author;
};

Theme slots_to_theme(const BaseSlots& s) {
    Theme theme;
    theme.metadata.name = s.name;
    theme.metadata.author = s.author;

    // UI Colors
    if (s.base[0x00]) theme.ui.background = *s.base[0x00];
    if (s.base[0x05]) theme.ui.foreground = *s.base[0x05];
    if (s.base[0x02]) theme.ui.selection_background = *s.base[0x02];
    if (s.base[0x05]) theme.ui.selection_foreground = *s.base[0x05];
    if (s.base[0x05]) theme.ui.cursor = *s.base[0x05];
    if (s.base[0x00]) theme.ui.cursor_text = *s.base[0x00];
    if (s.base[0x01]) theme.ui.active_line = *s.base[0x01];
    if (s.base[0x01]) theme.ui.status_bar = *s.base[0x01];

    // ANSI Colors
    if (s.base[0x00]) theme.ansi[0] = *s.base[0x00]; // black
    if (s.base[0x08]) theme.ansi[1] = *s.base[0x08]; // red
    if (s.base[0x0B]) theme.ansi[2] = *s.base[0x0B]; // green
    if (s.base[0x0A]) theme.ansi[3] = *s.base[0x0A]; // yellow
    if (s.base[0x0D]) theme.ansi[4] = *s.base[0x0D]; // blue
    if (s.base[0x0E]) theme.ansi[5] = *s.base[0x0E]; // magenta
    if (s.base[0x0C]) theme.ansi[6] = *s.base[0x0C]; // cyan
    if (s.base[0x05]) theme.ansi[7] = *s.base[0x05]; // white

    if (s.base[0x03]) theme.ansi[8] = *s.base[0x03]; // bright black

    // Base24 bright slots if available, else fallback to standard Base16 slots
    theme.ansi[9] = s.base[0x12].value_or(s.base[0x08].value_or(theme.ansi[1]));
    theme.ansi[10] = s.base[0x14].value_or(s.base[0x0B].value_or(theme.ansi[2]));
    theme.ansi[11] = s.base[0x13].value_or(s.base[0x0A].value_or(theme.ansi[3]));
    theme.ansi[12] = s.base[0x15].value_or(s.base[0x0D].value_or(theme.ansi[4]));
    theme.ansi[13] = s.base[0x17].value_or(s.base[0x0E].value_or(theme.ansi[5]));
    theme.ansi[14] = s.base[0x16].value_or(s.base[0x0C].value_or(theme.ansi[6]));
    theme.ansi[15] = s.base[0x07].value_or(s.base[0x06].value_or(theme.ansi[7]));

    // Syntax Colors
    if (s.base[0x03]) theme.syntax.comment = *s.base[0x03];
    if (s.base[0x0B]) theme.syntax.string = *s.base[0x0B];
    if (s.base[0x0E]) theme.syntax.keyword = *s.base[0x0E];
    if (s.base[0x09]) theme.syntax.number = *s.base[0x09];
    if (s.base[0x0D]) theme.syntax.function = *s.base[0x0D];
    if (s.base[0x0A]) theme.syntax.type = *s.base[0x0A];
    if (s.base[0x08]) theme.syntax.variable = *s.base[0x08];
    if (s.base[0x09]) theme.syntax.constant = *s.base[0x09];
    if (s.base[0x0C]) theme.syntax.operator_color = *s.base[0x0C];

    return theme;
}

} // namespace


std::optional<Theme> import_base16_yaml(std::string_view content) {
    ConfigMap map = ConfigMap::parse_yaml(content);
    if (!map.has("base00") && !map.has("scheme")) {
        return std::nullopt;
    }

    BaseSlots slots;
    slots.name = map.get_or("scheme", "");
    slots.author = map.get_or("author", "");

    bool has_any_slot = false;
    for (size_t i = 0; i < 24; ++i) {
        char k1[16], k2[16];
        std::snprintf(k1, sizeof(k1), "base%02x", static_cast<unsigned int>(i));
        std::snprintf(k2, sizeof(k2), "base%02X", static_cast<unsigned int>(i));

        const std::string* v = map.get(k1);
        if (!v) v = map.get(k2);

        if (v) {
            if (auto c = parse_color(*v)) {
                slots.base[i] = *c;
                has_any_slot = true;
            }
        }
    }

    if (!has_any_slot) return std::nullopt;
    return slots_to_theme(slots);
}

std::optional<Theme> import_base16_json(std::string_view content) {
    auto json = JsonValue::parse(content);
    if (!json.has_value() || !json->is_object()) {
        return std::nullopt;
    }

    BaseSlots slots;
    if (const JsonValue* s = json->find("scheme"); s && s->is_string()) {
        slots.name = s->as_string();
    }
    if (const JsonValue* a = json->find("author"); a && a->is_string()) {
        slots.author = a->as_string();
    }

    bool has_any_slot = false;
    for (size_t i = 0; i < 24; ++i) {
        char k1[16], k2[16];
        std::snprintf(k1, sizeof(k1), "base%02x", static_cast<unsigned int>(i));
        std::snprintf(k2, sizeof(k2), "base%02X", static_cast<unsigned int>(i));

        const JsonValue* v = json->find(k1);
        if (!v) v = json->find(k2);

        if (v && v->is_string()) {
            if (auto c = parse_color(v->as_string())) {
                slots.base[i] = *c;
                has_any_slot = true;
            }
        }
    }

    if (!has_any_slot) return std::nullopt;
    return slots_to_theme(slots);
}

std::string export_base16_yaml(const Theme& theme) {
    std::ostringstream ss;
    ss << "scheme: \"" << (theme.metadata.name.empty() ? "Custom" : theme.metadata.name) << "\"\n";
    ss << "author: \"" << (theme.metadata.author.empty() ? "brothemes" : theme.metadata.author) << "\"\n";

    Color slots[16];
    slots[0x00] = theme.ui.background;
    slots[0x01] = theme.ui.active_line.value_or(theme.ui.background);
    slots[0x02] = theme.ui.get_selection_background();
    slots[0x03] = theme.syntax.comment.value_or(theme.ansi[8]);
    slots[0x04] = theme.ansi[8];
    slots[0x05] = theme.ui.foreground;
    slots[0x06] = theme.ansi[7];
    slots[0x07] = theme.ansi[15];
    slots[0x08] = theme.ansi[1];
    slots[0x09] = theme.syntax.constant.value_or(theme.ansi[3]);
    slots[0x0A] = theme.ansi[3];
    slots[0x0B] = theme.ansi[2];
    slots[0x0C] = theme.ansi[6];
    slots[0x0D] = theme.ansi[4];
    slots[0x0E] = theme.ansi[5];
    slots[0x0F] = theme.syntax.keyword.value_or(theme.ansi[5]);

    for (size_t i = 0; i < 16; ++i) {
        char k[16];
        std::snprintf(k, sizeof(k), "base%02X", static_cast<unsigned int>(i));
        // Base16 standard typically outputs hex without '#' or with quotes
        char hex[10];
        std::snprintf(hex, sizeof(hex), "%02x%02x%02x", slots[i].r, slots[i].g, slots[i].b);
        ss << k << ": \"" << hex << "\"\n";
    }

    return ss.str();
}

std::string export_base16_json(const Theme& theme) {
    JsonValue obj;
    obj.type = JsonType::Object;

    obj["scheme"] = JsonValue(theme.metadata.name.empty() ? "Custom" : theme.metadata.name);
    obj["author"] = JsonValue(theme.metadata.author.empty() ? "brothemes" : theme.metadata.author);

    Color slots[16];
    slots[0x00] = theme.ui.background;
    slots[0x01] = theme.ui.active_line.value_or(theme.ui.background);
    slots[0x02] = theme.ui.get_selection_background();
    slots[0x03] = theme.syntax.comment.value_or(theme.ansi[8]);
    slots[0x04] = theme.ansi[8];
    slots[0x05] = theme.ui.foreground;
    slots[0x06] = theme.ansi[7];
    slots[0x07] = theme.ansi[15];
    slots[0x08] = theme.ansi[1];
    slots[0x09] = theme.syntax.constant.value_or(theme.ansi[3]);
    slots[0x0A] = theme.ansi[3];
    slots[0x0B] = theme.ansi[2];
    slots[0x0C] = theme.ansi[6];
    slots[0x0D] = theme.ansi[4];
    slots[0x0E] = theme.ansi[5];
    slots[0x0F] = theme.syntax.keyword.value_or(theme.ansi[5]);

    for (size_t i = 0; i < 16; ++i) {
        char k[16];
        std::snprintf(k, sizeof(k), "base%02X", static_cast<unsigned int>(i));
        char hex[10];
        std::snprintf(hex, sizeof(hex), "%02x%02x%02x", slots[i].r, slots[i].g, slots[i].b);
        obj[k] = JsonValue(hex);
    }

    return obj.dump(2);
}

std::string export_base24_yaml(const Theme& theme) {
    std::string b16 = export_base16_yaml(theme);
    std::ostringstream ss;
    ss << b16;

    // Additional Base24 bright slots:
    // base10, base11: darker backgrounds
    // base12: bright red (ansi 9)
    // base13: bright yellow (ansi 11)
    // base14: bright green (ansi 10)
    // base15: bright blue (ansi 12)
    // base16: bright cyan (ansi 14)
    // base17: bright magenta (ansi 13)
    Color slots24[8] = {
        theme.ui.background, // base10
        theme.ui.background, // base11
        theme.ansi[9],       // base12
        theme.ansi[11],      // base13
        theme.ansi[10],      // base14
        theme.ansi[12],      // base15
        theme.ansi[14],      // base16
        theme.ansi[13]       // base17
    };

    for (size_t i = 0; i < 8; ++i) {
        char k[16];
        std::snprintf(k, sizeof(k), "base%02X", static_cast<unsigned int>(i + 16));
        char hex[10];
        std::snprintf(hex, sizeof(hex), "%02x%02x%02x", slots24[i].r, slots24[i].g, slots24[i].b);
        ss << k << ": \"" << hex << "\"\n";
    }

    return ss.str();
}

std::string export_base24_json(const Theme& theme) {
    auto j_opt = JsonValue::parse(export_base16_json(theme));
    if (!j_opt.has_value()) return "{}";
    JsonValue obj = std::move(*j_opt);

    Color slots24[8] = {
        theme.ui.background,
        theme.ui.background,
        theme.ansi[9],
        theme.ansi[11],
        theme.ansi[10],
        theme.ansi[12],
        theme.ansi[14],
        theme.ansi[13]
    };

    for (size_t i = 0; i < 8; ++i) {
        char k[16];
        std::snprintf(k, sizeof(k), "base%02X", static_cast<unsigned int>(i + 16));
        char hex[10];
        std::snprintf(hex, sizeof(hex), "%02x%02x%02x", slots24[i].r, slots24[i].g, slots24[i].b);
        obj[k] = JsonValue(hex);
    }

    return obj.dump(2);
}

} // namespace bro::themes::detail

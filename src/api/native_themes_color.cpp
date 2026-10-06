#include "native_themes.h"
#include "themes_convert.h"
#include "arg_reader.h"
#include "object_builder.h"
#include <brothemes/color.h>
#include <brothemes/color_spaces.h>
#include <algorithm>
#include <cmath>

namespace bro::themes::api {

namespace {

bool checkAsHex(std::span<const Value> args, size_t optIndex = 1) {
    if (args.size() > optIndex) {
        ev::Persistent optP(args[optIndex]);
        if (ev::isString(optP.get())) {
            return ev::toUtf8(optP.get()) == "hex";
        }
        if (ev::isObject(optP.get())) {
            Value cf = ev::getProperty(optP.get(), "colorFormat");
            if (ev::isString(cf)) {
                return ev::toUtf8(cf) == "hex";
            }
        }
    }
    return false;
}

HexFormat parseHexFormat(Value opt) {
    ev::Persistent optP(opt);
    if (ev::isString(optP.get())) {
        std::string s = ev::toUtf8(optP.get());
        if (s == "upper-rgb" || s == "upper_rgb") return HexFormat::UpperRgb;
        if (s == "lower-rgba" || s == "lower_rgba" || s == "rgba") return HexFormat::LowerRgba;
        if (s == "upper-rgba" || s == "upper_rgba") return HexFormat::UpperRgba;
        return HexFormat::LowerRgb;
    }
    if (ev::isObject(optP.get())) {
        Value fmt = ev::getProperty(optP.get(), "format");
        if (ev::isString(fmt)) {
            return parseHexFormat(fmt);
        }
    }
    return HexFormat::LowerRgb;
}

} // namespace

void installColorOnto(ObjectBuilder& root) {

    // parseColor(str)
    root.def("parseColor", 1, [](Value, std::span<const Value> args) -> Value {
        if (args.empty() || !ev::isString(args[0])) return ev::null();
        auto c = parse_color(ev::toUtf8(args[0]));
        if (!c) return ev::null();
        return colorToJs(*c, false);
    });

    // parseHex(str) & fromHexString(str)
    auto parseHexFn = [](Value, std::span<const Value> args) -> Value {
        if (args.empty() || !ev::isString(args[0])) return ev::null();
        auto c = parse_hex(ev::toUtf8(args[0]));
        if (!c) return ev::null();
        return colorToJs(*c, false);
    };
    root.def("parseHex", 1, parseHexFn);
    root.def("fromHexString", 1, parseHexFn);

    // toHex(color, { format } = {}) & toHexString(color, { format } = {})
    auto toHexFn = [](Value, std::span<const Value> args) -> Value {
        if (args.empty()) return ev::throwTypeError("toHex expects at least 1 color argument");
        auto c = parseColorValue(args[0]);
        if (!c) return ev::throwTypeError("toHex: invalid color argument");

        HexFormat fmt = (c->a == 255) ? HexFormat::LowerRgb : HexFormat::LowerRgba;
        if (args.size() > 1) {
            fmt = parseHexFormat(args[1]);
        }
        return ev::fromUtf8(to_hex(*c, fmt));
    };
    root.def("toHex", 1, toHexFn);
    root.def("toHexString", 1, toHexFn);

    // toRgb(color)
    root.def("toRgb", 1, [](Value, std::span<const Value> args) -> Value {
        if (args.empty()) return ev::throwTypeError("toRgb expects 1 color argument");
        auto c = parseColorValue(args[0]);
        if (!c) return ev::throwTypeError("toRgb: invalid color argument");
        return ev::fromUtf8(to_rgb_string(*c));
    });

    // toRgba(color)
    root.def("toRgba", 1, [](Value, std::span<const Value> args) -> Value {
        if (args.empty()) return ev::throwTypeError("toRgba expects 1 color argument");
        auto c = parseColorValue(args[0]);
        if (!c) return ev::throwTypeError("toRgba: invalid color argument");
        return ev::fromUtf8(to_rgba_string(*c));
    });

    // createColor(r, g, b, a = 255) & Color(r, g, b, a = 255)
    auto createColorFn = [](Value, std::span<const Value> args) -> Value {
        ArgReader r(args);
        double rd = r.getDouble(0, 0.0);
        double gd = r.getDouble(1, 0.0);
        double bd = r.getDouble(2, 0.0);
        double ad = 255.0;

        if (args.size() > 3) {
            ad = r.getDouble(3, 255.0);
            if (ad >= 0.0 && ad <= 1.0) ad *= 255.0;
        }

        uint8_t cr = static_cast<uint8_t>(std::clamp(std::round(rd), 0.0, 255.0));
        uint8_t cg = static_cast<uint8_t>(std::clamp(std::round(gd), 0.0, 255.0));
        uint8_t cb = static_cast<uint8_t>(std::clamp(std::round(bd), 0.0, 255.0));
        uint8_t ca = static_cast<uint8_t>(std::clamp(std::round(ad), 0.0, 255.0));
        return colorToJs(Color(cr, cg, cb, ca), false);
    };
    root.def("createColor", 3, createColorFn);
    root.def("Color", 3, createColorFn);

    // --- Color Spaces ---

    // srgbToLinear(color)
    root.def("srgbToLinear", 1, [](Value, std::span<const Value> args) -> Value {
        if (args.empty()) return ev::throwTypeError("srgbToLinear expects 1 color argument");
        auto c = parseColorValue(args[0]);
        if (!c) return ev::throwTypeError("srgbToLinear: invalid color argument");
        return linearRgbToJs(to_linear(*c));
    });

    // linearToSrgb(linear, options?)
    root.def("linearToSrgb", 1, [](Value, std::span<const Value> args) -> Value {
        if (args.empty()) return ev::throwTypeError("linearToSrgb expects 1 argument");
        auto lin = jsToLinearRgb(args[0]);
        if (!lin) return ev::throwTypeError("linearToSrgb: invalid linear RGB object");
        return colorToJs(from_linear(*lin), checkAsHex(args));
    });

    // srgbToXyz(color)
    root.def("srgbToXyz", 1, [](Value, std::span<const Value> args) -> Value {
        if (args.empty()) return ev::throwTypeError("srgbToXyz expects 1 color argument");
        auto c = parseColorValue(args[0]);
        if (!c) return ev::throwTypeError("srgbToXyz: invalid color argument");
        return xyzToJs(to_xyz(*c));
    });

    // xyzToSrgb(xyz, options?)
    root.def("xyzToSrgb", 1, [](Value, std::span<const Value> args) -> Value {
        if (args.empty()) return ev::throwTypeError("xyzToSrgb expects 1 argument");
        auto xyz = jsToXyz(args[0]);
        if (!xyz) return ev::throwTypeError("xyzToSrgb: invalid xyz object");
        return colorToJs(from_xyz(*xyz), checkAsHex(args));
    });

    // linearToXyz(linear)
    root.def("linearToXyz", 1, [](Value, std::span<const Value> args) -> Value {
        if (args.empty()) return ev::throwTypeError("linearToXyz expects 1 argument");
        auto lin = jsToLinearRgb(args[0]);
        if (!lin) return ev::throwTypeError("linearToXyz: invalid linear RGB object");
        return xyzToJs(linear_to_xyz(*lin));
    });

    // xyzToLinear(xyz)
    root.def("xyzToLinear", 1, [](Value, std::span<const Value> args) -> Value {
        if (args.empty()) return ev::throwTypeError("xyzToLinear expects 1 argument");
        auto xyz = jsToXyz(args[0]);
        if (!xyz) return ev::throwTypeError("xyzToLinear: invalid xyz object");
        return linearRgbToJs(xyz_to_linear(*xyz));
    });

    // srgbToLab(color)
    root.def("srgbToLab", 1, [](Value, std::span<const Value> args) -> Value {
        if (args.empty()) return ev::throwTypeError("srgbToLab expects 1 color argument");
        auto c = parseColorValue(args[0]);
        if (!c) return ev::throwTypeError("srgbToLab: invalid color argument");
        return labToJs(to_lab(*c));
    });

    // labToSrgb(lab, options?)
    root.def("labToSrgb", 1, [](Value, std::span<const Value> args) -> Value {
        if (args.empty()) return ev::throwTypeError("labToSrgb expects 1 argument");
        auto lab = jsToLab(args[0]);
        if (!lab) return ev::throwTypeError("labToSrgb: invalid lab object");
        return colorToJs(from_lab(*lab), checkAsHex(args));
    });

    // xyzToLab(xyz)
    root.def("xyzToLab", 1, [](Value, std::span<const Value> args) -> Value {
        if (args.empty()) return ev::throwTypeError("xyzToLab expects 1 argument");
        auto xyz = jsToXyz(args[0]);
        if (!xyz) return ev::throwTypeError("xyzToLab: invalid xyz object");
        return labToJs(xyz_to_lab(*xyz));
    });

    // labToXyz(lab)
    root.def("labToXyz", 1, [](Value, std::span<const Value> args) -> Value {
        if (args.empty()) return ev::throwTypeError("labToXyz expects 1 argument");
        auto lab = jsToLab(args[0]);
        if (!lab) return ev::throwTypeError("labToXyz: invalid lab object");
        return xyzToJs(lab_to_xyz(*lab));
    });

    // srgbToOklab(color)
    root.def("srgbToOklab", 1, [](Value, std::span<const Value> args) -> Value {
        if (args.empty()) return ev::throwTypeError("srgbToOklab expects 1 color argument");
        auto c = parseColorValue(args[0]);
        if (!c) return ev::throwTypeError("srgbToOklab: invalid color argument");
        return oklabToJs(to_oklab(*c));
    });

    // oklabToSrgb(oklab, options?)
    root.def("oklabToSrgb", 1, [](Value, std::span<const Value> args) -> Value {
        if (args.empty()) return ev::throwTypeError("oklabToSrgb expects 1 argument");
        auto oklab = jsToOklab(args[0]);
        if (!oklab) return ev::throwTypeError("oklabToSrgb: invalid oklab object");
        return colorToJs(from_oklab(*oklab), checkAsHex(args));
    });

    // linearToOklab(linear)
    root.def("linearToOklab", 1, [](Value, std::span<const Value> args) -> Value {
        if (args.empty()) return ev::throwTypeError("linearToOklab expects 1 argument");
        auto lin = jsToLinearRgb(args[0]);
        if (!lin) return ev::throwTypeError("linearToOklab: invalid linear RGB object");
        return oklabToJs(linear_to_oklab(*lin));
    });

    // oklabToLinear(oklab)
    root.def("oklabToLinear", 1, [](Value, std::span<const Value> args) -> Value {
        if (args.empty()) return ev::throwTypeError("oklabToLinear expects 1 argument");
        auto oklab = jsToOklab(args[0]);
        if (!oklab) return ev::throwTypeError("oklabToLinear: invalid oklab object");
        return linearRgbToJs(oklab_to_linear(*oklab));
    });

    // srgbToOklch(color)
    root.def("srgbToOklch", 1, [](Value, std::span<const Value> args) -> Value {
        if (args.empty()) return ev::throwTypeError("srgbToOklch expects 1 color argument");
        auto c = parseColorValue(args[0]);
        if (!c) return ev::throwTypeError("srgbToOklch: invalid color argument");
        return oklchToJs(to_oklch(*c));
    });

    // oklchToSrgb(oklch, options?)
    root.def("oklchToSrgb", 1, [](Value, std::span<const Value> args) -> Value {
        if (args.empty()) return ev::throwTypeError("oklchToSrgb expects 1 argument");
        auto oklch = jsToOklch(args[0]);
        if (!oklch) return ev::throwTypeError("oklchToSrgb: invalid oklch object");
        return colorToJs(from_oklch(*oklch), checkAsHex(args));
    });

    // oklabToOklch(oklab)
    root.def("oklabToOklch", 1, [](Value, std::span<const Value> args) -> Value {
        if (args.empty()) return ev::throwTypeError("oklabToOklch expects 1 argument");
        auto oklab = jsToOklab(args[0]);
        if (!oklab) return ev::throwTypeError("oklabToOklch: invalid oklab object");
        return oklchToJs(oklab_to_oklch(*oklab));
    });

    // oklchToOklab(oklch)
    root.def("oklchToOklab", 1, [](Value, std::span<const Value> args) -> Value {
        if (args.empty()) return ev::throwTypeError("oklchToOklab expects 1 argument");
        auto oklch = jsToOklch(args[0]);
        if (!oklch) return ev::throwTypeError("oklchToOklab: invalid oklch object");
        return oklabToJs(oklch_to_oklab(*oklch));
    });

    // srgbToHsl(color)
    root.def("srgbToHsl", 1, [](Value, std::span<const Value> args) -> Value {
        if (args.empty()) return ev::throwTypeError("srgbToHsl expects 1 color argument");
        auto c = parseColorValue(args[0]);
        if (!c) return ev::throwTypeError("srgbToHsl: invalid color argument");
        return hslToJs(to_hsl(*c));
    });

    // hslToSrgb(hsl, options?)
    root.def("hslToSrgb", 1, [](Value, std::span<const Value> args) -> Value {
        if (args.empty()) return ev::throwTypeError("hslToSrgb expects 1 argument");
        auto hsl = jsToHsl(args[0]);
        if (!hsl) return ev::throwTypeError("hslToSrgb: invalid hsl object");
        return colorToJs(from_hsl(*hsl), checkAsHex(args));
    });

    // srgbToHsv(color)
    root.def("srgbToHsv", 1, [](Value, std::span<const Value> args) -> Value {
        if (args.empty()) return ev::throwTypeError("srgbToHsv expects 1 color argument");
        auto c = parseColorValue(args[0]);
        if (!c) return ev::throwTypeError("srgbToHsv: invalid color argument");
        return hsvToJs(to_hsv(*c));
    });

    // hsvToSrgb(hsv, options?)
    root.def("hsvToSrgb", 1, [](Value, std::span<const Value> args) -> Value {
        if (args.empty()) return ev::throwTypeError("hsvToSrgb expects 1 argument");
        auto hsv = jsToHsv(args[0]);
        if (!hsv) return ev::throwTypeError("hsvToSrgb: invalid hsv object");
        return colorToJs(from_hsv(*hsv), checkAsHex(args));
    });

    // fitOklchToGamut(oklch)
    root.def("fitOklchToGamut", 1, [](Value, std::span<const Value> args) -> Value {
        if (args.empty()) return ev::throwTypeError("fitOklchToGamut expects 1 argument");
        auto oklch = jsToOklch(args[0]);
        if (!oklch) return ev::throwTypeError("fitOklchToGamut: invalid oklch object");
        return oklchToJs(fit_oklch_to_gamut(*oklch));
    });

    // isInSrgbGamut(linear)
    root.def("isInSrgbGamut", 1, [](Value, std::span<const Value> args) -> Value {
        if (args.empty()) return ev::throwTypeError("isInSrgbGamut expects 1 argument");
        auto lin = jsToLinearRgb(args[0]);
        if (!lin) return ev::throwTypeError("isInSrgbGamut: invalid linear RGB object");
        return ev::fromBool(is_in_srgb_gamut(*lin));
    });

    // clampToSrgbGamut(linear)
    root.def("clampToSrgbGamut", 1, [](Value, std::span<const Value> args) -> Value {
        if (args.empty()) return ev::throwTypeError("clampToSrgbGamut expects 1 argument");
        auto lin = jsToLinearRgb(args[0]);
        if (!lin) return ev::throwTypeError("clampToSrgbGamut: invalid linear RGB object");
        return linearRgbToJs(clamp_to_srgb_gamut(*lin));
    });
}

} // namespace bro::themes::api

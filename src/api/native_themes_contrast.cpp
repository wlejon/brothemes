#include "native_themes.h"
#include "themes_convert.h"
#include "arg_reader.h"
#include "object_builder.h"
#include <brothemes/contrast.h>
#include <algorithm>
#include <cmath>

namespace bro::themes::api {

void installContrastOnto(ObjectBuilder& root) {

    // contrast(fg, bg, { method = "wcag" } = {})
    root.def("contrast", 2, [](Value, std::span<const Value> args) -> Value {
        ArgReader r(args);
        if (args.size() < 2) return ev::throwTypeError("contrast expects at least 2 arguments: (fg, bg)");
        auto fg = parseColorValue(r.get(0));
        auto bg = parseColorValue(r.get(1));
        if (!fg || !bg) return ev::throwTypeError("contrast: invalid color argument");

        std::string method = "wcag";
        if (args.size() > 2) {
            ev::Persistent optP(r.get(2));
            if (ev::isString(optP.get())) {
                method = ev::toUtf8(optP.get());
            } else if (ev::isObject(optP.get())) {
                Value m = ev::getProperty(optP.get(), "method");
                if (ev::isString(m)) method = ev::toUtf8(m);
            }
        }

        if (method == "apca") {
            return ev::fromDouble(apca_contrast(*fg, *bg));
        }
        return ev::fromDouble(wcag_contrast_ratio(*fg, *bg));
    });

    // relativeLuminance(color)
    root.def("relativeLuminance", 1, [](Value, std::span<const Value> args) -> Value {
        if (args.empty()) return ev::throwTypeError("relativeLuminance expects 1 argument: (color)");
        auto c = parseColorValue(args[0]);
        if (!c) return ev::throwTypeError("relativeLuminance: invalid color argument");
        return ev::fromDouble(relative_luminance(*c));
    });

    // wcagLevel(fg, bg)
    root.def("wcagLevel", 2, [](Value, std::span<const Value> args) -> Value {
        if (args.size() < 2) return ev::throwTypeError("wcagLevel expects 2 arguments: (fg, bg)");
        auto fg = parseColorValue(args[0]);
        auto bg = parseColorValue(args[1]);
        if (!fg || !bg) return ev::throwTypeError("wcagLevel: invalid color argument");

        WcagLevel lvl = wcag_level(*fg, *bg);
        switch (lvl) {
            case WcagLevel::Fail: return ev::fromUtf8("fail");
            case WcagLevel::AaLarge: return ev::fromUtf8("aa-large");
            case WcagLevel::AaNormal: return ev::fromUtf8("aa");
            case WcagLevel::AaaNormal: return ev::fromUtf8("aaa");
        }
        return ev::fromUtf8("fail");
    });

    // meetsAa(fg, bg, { isLargeText } = {})
    root.def("meetsAa", 2, [](Value, std::span<const Value> args) -> Value {
        if (args.size() < 2) return ev::throwTypeError("meetsAa expects at least 2 arguments: (fg, bg)");
        auto fg = parseColorValue(args[0]);
        auto bg = parseColorValue(args[1]);
        if (!fg || !bg) return ev::throwTypeError("meetsAa: invalid color argument");

        bool isLarge = false;
        if (args.size() > 2) {
            ev::Persistent optP(args[2]);
            if (ev::isBool(optP.get())) {
                isLarge = ev::toBool(optP.get());
            } else if (ev::isObject(optP.get())) {
                Value l = ev::getProperty(optP.get(), "isLargeText");
                if (ev::isBool(l)) isLarge = ev::toBool(l);
            }
        }
        return ev::fromBool(meets_wcag_aa(*fg, *bg, isLarge));
    });

    // meetsAaa(fg, bg, { isLargeText } = {})
    root.def("meetsAaa", 2, [](Value, std::span<const Value> args) -> Value {
        if (args.size() < 2) return ev::throwTypeError("meetsAaa expects at least 2 arguments: (fg, bg)");
        auto fg = parseColorValue(args[0]);
        auto bg = parseColorValue(args[1]);
        if (!fg || !bg) return ev::throwTypeError("meetsAaa: invalid color argument");

        bool isLarge = false;
        if (args.size() > 2) {
            ev::Persistent optP(args[2]);
            if (ev::isBool(optP.get())) {
                isLarge = ev::toBool(optP.get());
            } else if (ev::isObject(optP.get())) {
                Value l = ev::getProperty(optP.get(), "isLargeText");
                if (ev::isBool(l)) isLarge = ev::toBool(l);
            }
        }
        return ev::fromBool(meets_wcag_aaa(*fg, *bg, isLarge));
    });

    // apcaContrast(fg, bg, { absolute } = {})
    root.def("apcaContrast", 2, [](Value, std::span<const Value> args) -> Value {
        if (args.size() < 2) return ev::throwTypeError("apcaContrast expects at least 2 arguments: (fg, bg)");
        auto fg = parseColorValue(args[0]);
        auto bg = parseColorValue(args[1]);
        if (!fg || !bg) return ev::throwTypeError("apcaContrast: invalid color argument");

        bool absolute = false;
        if (args.size() > 2) {
            ev::Persistent optP(args[2]);
            if (ev::isBool(optP.get())) {
                absolute = ev::toBool(optP.get());
            } else if (ev::isObject(optP.get())) {
                Value a = ev::getProperty(optP.get(), "absolute");
                if (ev::isBool(a)) absolute = ev::toBool(a);
            }
        }

        float res = absolute ? apca_contrast_abs(*fg, *bg) : apca_contrast(*fg, *bg);
        return ev::fromDouble(res);
    });

    // adjustContrast(fg, bg, { minRatio = 4.5, colorFormat } = {})
    root.def("adjustContrast", 2, [](Value, std::span<const Value> args) -> Value {
        if (args.size() < 2) return ev::throwTypeError("adjustContrast expects at least 2 arguments: (fg, bg)");
        auto fg = parseColorValue(args[0]);
        auto bg = parseColorValue(args[1]);
        if (!fg || !bg) return ev::throwTypeError("adjustContrast: invalid color argument");

        float minRatio = 4.5f;
        bool asHex = ev::isString(args[0]);
        if (args.size() > 2) {
            ev::Persistent optP(args[2]);
            if (ev::isNumber(optP.get())) {
                minRatio = static_cast<float>(ev::toDouble(optP.get()));
            } else if (ev::isObject(optP.get())) {
                Value mr = ev::getProperty(optP.get(), "minRatio");
                if (ev::isNumber(mr)) minRatio = static_cast<float>(ev::toDouble(mr));
                Value cf = ev::getProperty(optP.get(), "colorFormat");
                if (ev::isString(cf)) {
                    std::string s = ev::toUtf8(cf);
                    if (s == "hex") asHex = true;
                    else if (s == "object") asHex = false;
                }
            }
        }

        Color adjusted = adjust_contrast(*fg, *bg, minRatio);
        return colorToJs(adjusted, asHex);
    });

    // adjustPaletteContrast(themeObj, { minRatio = 4.5, colorFormat } = {})
    root.def("adjustPaletteContrast", 1, [](Value, std::span<const Value> args) -> Value {
        if (args.empty() || !ev::isObject(args[0])) {
            return ev::throwTypeError("adjustPaletteContrast expects 1 argument: (themeObj)");
        }
        auto theme = jsToTheme(args[0]);
        if (!theme) return ev::throwTypeError("adjustPaletteContrast: invalid theme object");

        float minRatio = 4.5f;
        bool asHex = true;
        if (args.size() > 1) {
            ev::Persistent optP(args[1]);
            if (ev::isNumber(optP.get())) {
                minRatio = static_cast<float>(ev::toDouble(optP.get()));
            } else if (ev::isObject(optP.get())) {
                Value mr = ev::getProperty(optP.get(), "minRatio");
                if (ev::isNumber(mr)) minRatio = static_cast<float>(ev::toDouble(mr));
                Value cf = ev::getProperty(optP.get(), "colorFormat");
                if (ev::isString(cf)) {
                    std::string s = ev::toUtf8(cf);
                    if (s == "object") asHex = false;
                }
            }
        }

        adjust_palette_contrast(*theme, minRatio);
        return themeToJs(*theme, asHex);
    });
}

} // namespace bro::themes::api

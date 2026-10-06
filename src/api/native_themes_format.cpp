#include "native_themes.h"
#include "api.h"
#include "themes_convert.h"
#include "arg_reader.h"
#include "object_builder.h"
#include <brothemes/format.h>

namespace bro::themes::api {

void installFormatOnto(ObjectBuilder& root) {

    // detectFormat(content, filenameOrExt?)
    root.def("detectFormat", 1, [](Value, std::span<const Value> args) -> Value {
        if (args.empty() || !ev::isString(args[0])) {
            return ev::throwTypeError("detectFormat expects at least 1 string argument: (content)");
        }
        std::string content = ev::toUtf8(args[0]);
        std::string ext = "";
        if (args.size() > 1 && ev::isString(args[1])) {
            ext = ev::toUtf8(args[1]);
        }
        ThemeFormat fmt = detect_format(content, ext);
        return ev::fromUtf8(formatToString(fmt));
    });

    // formatName(format)
    root.def("formatName", 1, [](Value, std::span<const Value> args) -> Value {
        if (args.empty() || !ev::isString(args[0])) {
            return ev::throwTypeError("formatName expects 1 string argument: (format)");
        }
        std::string fmtStr = ev::toUtf8(args[0]);
        auto fmt = parseFormatString(fmtStr);
        if (!fmt) return ev::fromUtf8("Unknown");
        return ev::fromUtf8(format_name(*fmt));
    });

    // import(content, { format = "auto", colorFormat = "hex" } = {})
    root.def("import", 1, [](Value, std::span<const Value> args) -> Value {
        if (args.empty() || !ev::isString(args[0])) {
            return ev::throwTypeError("import expects at least 1 string argument: (content)");
        }
        std::string content = ev::toUtf8(args[0]);
        ThemeFormat fmt = ThemeFormat::Auto;
        bool asHex = true;

        if (args.size() > 1) {
            ev::Persistent optP(args[1]);
            if (ev::isString(optP.get())) {
                auto parsed = parseFormatString(ev::toUtf8(optP.get()));
                if (parsed) fmt = *parsed;
            } else if (ev::isObject(optP.get())) {
                Value f = ev::getProperty(optP.get(), "format");
                if (ev::isString(f)) {
                    auto parsed = parseFormatString(ev::toUtf8(f));
                    if (parsed) fmt = *parsed;
                }
                Value cf = ev::getProperty(optP.get(), "colorFormat");
                if (ev::isString(cf) && ev::toUtf8(cf) == "object") {
                    asHex = false;
                }
            }
        }

        auto theme = import_theme(content, fmt);
        if (!theme.has_value()) {
            return ev::throwError("Failed to import theme: unrecognized format or invalid content");
        }
        return themeToJs(*theme, asHex);
    });

    // export(themeObj, { format = "windows-terminal" } = {})
    root.def("export", 1, [](Value, std::span<const Value> args) -> Value {
        if (args.empty() || !ev::isObject(args[0])) {
            return ev::throwTypeError("export expects at least 1 object argument: (themeObj)");
        }
        auto theme = jsToTheme(args[0]);
        if (!theme.has_value()) {
            return ev::throwTypeError("export: invalid theme object");
        }

        ThemeFormat fmt = ThemeFormat::WindowsTerminal;
        if (args.size() > 1) {
            ev::Persistent optP(args[1]);
            if (ev::isString(optP.get())) {
                auto parsed = parseFormatString(ev::toUtf8(optP.get()));
                if (parsed) fmt = *parsed;
            } else if (ev::isObject(optP.get())) {
                Value f = ev::getProperty(optP.get(), "format");
                if (ev::isString(f)) {
                    auto parsed = parseFormatString(ev::toUtf8(f));
                    if (parsed) fmt = *parsed;
                }
            }
        }

        std::string text = export_theme(*theme, fmt);
        return ev::fromUtf8(text);
    });

    // load(path, { format = "auto", colorFormat = "hex" } = {})
    root.def("load", 1, [](Value, std::span<const Value> args) -> Value {
        if (args.empty() || !ev::isString(args[0])) {
            return ev::throwTypeError("load expects at least 1 string argument: (path)");
        }
        std::string rawPath = ev::toUtf8(args[0]);
        std::string resolved = resolvePath(rawPath);

        ThemeFormat fmt = ThemeFormat::Auto;
        bool asHex = true;
        if (args.size() > 1) {
            ev::Persistent optP(args[1]);
            if (ev::isString(optP.get())) {
                auto parsed = parseFormatString(ev::toUtf8(optP.get()));
                if (parsed) fmt = *parsed;
            } else if (ev::isObject(optP.get())) {
                Value f = ev::getProperty(optP.get(), "format");
                if (ev::isString(f)) {
                    auto parsed = parseFormatString(ev::toUtf8(f));
                    if (parsed) fmt = *parsed;
                }
                Value cf = ev::getProperty(optP.get(), "colorFormat");
                if (ev::isString(cf) && ev::toUtf8(cf) == "object") {
                    asHex = false;
                }
            }
        }

        auto theme = load_theme_file(resolved, fmt);
        if (!theme.has_value()) {
            return ev::throwError("Failed to load theme file: " + rawPath);
        }
        return themeToJs(*theme, asHex);
    });

    // save(themeObj, path, { format = "auto" } = {})
    root.def("save", 2, [](Value, std::span<const Value> args) -> Value {
        if (args.size() < 2 || !ev::isObject(args[0]) || !ev::isString(args[1])) {
            return ev::throwTypeError("save expects at least 2 arguments: (themeObj, path)");
        }
        auto theme = jsToTheme(args[0]);
        if (!theme.has_value()) {
            return ev::throwTypeError("save: invalid theme object");
        }

        std::string rawPath = ev::toUtf8(args[1]);
        std::string resolved = resolvePath(rawPath);

        ThemeFormat fmt = ThemeFormat::Auto;
        if (args.size() > 2) {
            ev::Persistent optP(args[2]);
            if (ev::isString(optP.get())) {
                auto parsed = parseFormatString(ev::toUtf8(optP.get()));
                if (parsed) fmt = *parsed;
            } else if (ev::isObject(optP.get())) {
                Value f = ev::getProperty(optP.get(), "format");
                if (ev::isString(f)) {
                    auto parsed = parseFormatString(ev::toUtf8(f));
                    if (parsed) fmt = *parsed;
                }
            }
        }

        bool ok = save_theme_file(*theme, resolved, fmt);
        return ev::fromBool(ok);
    });
}

} // namespace bro::themes::api

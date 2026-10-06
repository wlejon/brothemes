#pragma once

#include <brothemes/themes.h>
#include "embed/embed.h"
#include <optional>
#include <string>
#include <string_view>

namespace bro::themes::api {

namespace ev = bronze::embed;
using Value = bronze::Value;

// Color conversions
std::optional<Color> parseColorValue(Value val);
Value colorToJs(Color c, bool asHex = false);

// Theme conversions
Value themeToJs(const Theme& theme, bool asHex = true);
std::optional<Theme> jsToTheme(Value val);

// Format conversions
std::optional<ThemeFormat> parseFormatString(std::string_view str);
std::string formatToString(ThemeFormat fmt);

// Color space helpers
Value linearRgbToJs(LinearRgb c);
std::optional<LinearRgb> jsToLinearRgb(Value v);

Value xyzToJs(Xyz c);
std::optional<Xyz> jsToXyz(Value v);

Value labToJs(Lab c);
std::optional<Lab> jsToLab(Value v);

Value oklabToJs(Oklab c);
std::optional<Oklab> jsToOklab(Value v);

Value oklchToJs(Oklch c);
std::optional<Oklch> jsToOklch(Value v);

Value hslToJs(Hsl c);
std::optional<Hsl> jsToHsl(Value v);

Value hsvToJs(Hsv c);
std::optional<Hsv> jsToHsv(Value v);

} // namespace bro::themes::api

#pragma once

#include <brothemes/color.h>

namespace bro::themes {

struct LinearRgb {
    float r{0.0f};
    float g{0.0f};
    float b{0.0f};

    constexpr LinearRgb() noexcept = default;
    constexpr LinearRgb(float r_, float g_, float b_) noexcept : r(r_), g(g_), b(b_) {}
    constexpr bool operator==(const LinearRgb& other) const noexcept = default;
};

// CIE 1931 XYZ (D65 illuminant, Y normalized to [0.0, 1.0])
struct Xyz {
    float x{0.0f};
    float y{0.0f};
    float z{0.0f};

    constexpr Xyz() noexcept = default;
    constexpr Xyz(float x_, float y_, float z_) noexcept : x(x_), y(y_), z(z_) {}
    constexpr bool operator==(const Xyz& other) const noexcept = default;
};

// CIE L*a*b* (D65 standard illuminant, L* in [0, 100])
struct Lab {
    float l{0.0f};
    float a{0.0f};
    float b{0.0f};

    constexpr Lab() noexcept = default;
    constexpr Lab(float l_, float a_, float b_) noexcept : l(l_), a(a_), b(b_) {}
    constexpr bool operator==(const Lab& other) const noexcept = default;
};

// Oklab (Björn Ottosson, L in [0.0, 1.0])
struct Oklab {
    float l{0.0f};
    float a{0.0f};
    float b{0.0f};

    constexpr Oklab() noexcept = default;
    constexpr Oklab(float l_, float a_, float b_) noexcept : l(l_), a(a_), b(b_) {}
    constexpr bool operator==(const Oklab& other) const noexcept = default;
};

// Oklch (Cylindrical Oklab: L in [0, 1], C >= 0, h in degrees [0, 360))
struct Oklch {
    float l{0.0f};
    float c{0.0f};
    float h{0.0f}; // in degrees

    constexpr Oklch() noexcept = default;
    constexpr Oklch(float l_, float c_, float h_) noexcept : l(l_), c(c_), h(h_) {}
    constexpr bool operator==(const Oklch& other) const noexcept = default;
};

// HSL: h in [0, 360), s in [0, 1], l in [0, 1]
struct Hsl {
    float h{0.0f};
    float s{0.0f};
    float l{0.0f};

    constexpr Hsl() noexcept = default;
    constexpr Hsl(float h_, float s_, float l_) noexcept : h(h_), s(s_), l(l_) {}
    constexpr bool operator==(const Hsl& other) const noexcept = default;
};

// HSV: h in [0, 360), s in [0, 1], v in [0, 1]
struct Hsv {
    float h{0.0f};
    float s{0.0f};
    float v{0.0f};

    constexpr Hsv() noexcept = default;
    constexpr Hsv(float h_, float s_, float v_) noexcept : h(h_), s(s_), v(v_) {}
    constexpr bool operator==(const Hsv& other) const noexcept = default;
};

// Core color space transforms
LinearRgb srgb_to_linear(ColorF c) noexcept;
ColorF linear_to_srgb(LinearRgb c) noexcept;

Xyz linear_to_xyz(LinearRgb c) noexcept;
LinearRgb xyz_to_linear(Xyz c) noexcept;

Lab xyz_to_lab(Xyz c) noexcept;
Xyz lab_to_xyz(Lab c) noexcept;

Oklab linear_to_oklab(LinearRgb c) noexcept;
LinearRgb oklab_to_linear(Oklab c) noexcept;

Oklch oklab_to_oklch(Oklab c) noexcept;
Oklab oklch_to_oklab(Oklch c) noexcept;

Hsl srgb_to_hsl(ColorF c) noexcept;
ColorF hsl_to_srgb(Hsl c) noexcept;

Hsv srgb_to_hsv(ColorF c) noexcept;
ColorF hsv_to_srgb(Hsv c) noexcept;

// Convenience bridges directly to Color (8-bit sRGB)
Color from_linear(LinearRgb c) noexcept;
LinearRgb to_linear(Color c) noexcept;

Color from_xyz(Xyz c) noexcept;
Xyz to_xyz(Color c) noexcept;

Color from_lab(Lab c) noexcept;
Lab to_lab(Color c) noexcept;

Color from_oklab(Oklab c) noexcept;
Oklab to_oklab(Color c) noexcept;

Color from_oklch(Oklch c) noexcept;
Oklch to_oklch(Color c) noexcept;

Color from_hsl(Hsl c) noexcept;
Hsl to_hsl(Color c) noexcept;

Color from_hsv(Hsv c) noexcept;
Hsv to_hsv(Color c) noexcept;

// In-gamut check and gamut mapping for sRGB
bool is_in_srgb_gamut(LinearRgb c) noexcept;
LinearRgb clamp_to_srgb_gamut(LinearRgb c) noexcept;

// Fit Oklch to sRGB gamut while preserving hue (reduces chroma if needed)
Oklch fit_oklch_to_gamut(Oklch c) noexcept;

} // namespace bro::themes

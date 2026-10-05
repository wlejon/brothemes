#include <brothemes/color_spaces.h>
#include <algorithm>
#include <cmath>
#include <numbers>

namespace bro::themes {

namespace {

constexpr float PI = static_cast<float>(std::numbers::pi);
constexpr float RAD2DEG = 180.0f / PI;
constexpr float DEG2RAD = PI / 180.0f;

float srgb_transfer_to_linear(float c) noexcept {
    c = std::clamp(c, 0.0f, 1.0f);
    if (c <= 0.04045f) {
        return c / 12.92f;
    }
    return std::pow((c + 0.055f) / 1.055f, 2.4f);
}

float linear_transfer_to_srgb(float c) noexcept {
    if (c <= 0.0031308f) {
        return c * 12.92f;
    }
    return 1.055f * std::pow(c, 1.0f / 2.4f) - 0.055f;
}

// Signed cube root handling small negatives gracefully
float safe_cbrt(float v) noexcept {
    if (v < 0.0f) {
        return -std::cbrt(-v);
    }
    return std::cbrt(v);
}

} // namespace

LinearRgb srgb_to_linear(ColorF c) noexcept {
    return LinearRgb(
        srgb_transfer_to_linear(c.r),
        srgb_transfer_to_linear(c.g),
        srgb_transfer_to_linear(c.b)
    );
}

ColorF linear_to_srgb(LinearRgb c) noexcept {
    auto clamp01 = [](float v) { return std::clamp(v, 0.0f, 1.0f); };
    return ColorF(
        clamp01(linear_transfer_to_srgb(c.r)),
        clamp01(linear_transfer_to_srgb(c.g)),
        clamp01(linear_transfer_to_srgb(c.b)),
        1.0f
    );
}

Xyz linear_to_xyz(LinearRgb c) noexcept {
    return Xyz(
        0.4124564f * c.r + 0.3575761f * c.g + 0.1804375f * c.b,
        0.2126729f * c.r + 0.7151522f * c.g + 0.0721750f * c.b,
        0.0193339f * c.r + 0.1191920f * c.g + 0.9503041f * c.b
    );
}

LinearRgb xyz_to_linear(Xyz c) noexcept {
    return LinearRgb(
         3.2404542f * c.x - 1.5371385f * c.y - 0.4985314f * c.z,
        -0.9692660f * c.x + 1.8760108f * c.y + 0.0415560f * c.z,
         0.0556434f * c.x - 0.2040259f * c.y + 1.0572252f * c.z
    );
}

Lab xyz_to_lab(Xyz c) noexcept {
    // Reference white D65
    constexpr float Xn = 0.95047f;
    constexpr float Yn = 1.00000f;
    constexpr float Zn = 1.08883f;

    constexpr float delta = 6.0f / 29.0f;
    constexpr float delta3 = delta * delta * delta;
    constexpr float factor = 1.0f / (3.0f * delta * delta);

    auto f = [](float t) -> float {
        if (t > delta3) {
            return std::cbrt(t);
        }
        return factor * t + 4.0f / 29.0f;
    };

    float fx = f(c.x / Xn);
    float fy = f(c.y / Yn);
    float fz = f(c.z / Zn);

    float l = 116.0f * fy - 16.0f;
    float a = 500.0f * (fx - fy);
    float b = 200.0f * (fy - fz);

    return Lab(l, a, b);
}

Xyz lab_to_xyz(Lab c) noexcept {
    constexpr float Xn = 0.95047f;
    constexpr float Yn = 1.00000f;
    constexpr float Zn = 1.08883f;

    constexpr float delta = 6.0f / 29.0f;
    constexpr float factor = 3.0f * delta * delta;

    float fy = (c.l + 16.0f) / 116.0f;
    float fx = fy + c.a / 500.0f;
    float fz = fy - c.b / 200.0f;

    auto inv_f = [](float t) -> float {
        if (t > delta) {
            return t * t * t;
        }
        return factor * (t - 4.0f / 29.0f);
    };


    return Xyz(
        Xn * inv_f(fx),
        Yn * inv_f(fy),
        Zn * inv_f(fz)
    );
}

Oklab linear_to_oklab(LinearRgb c) noexcept {
    float l = 0.4122214708f * c.r + 0.5363325363f * c.g + 0.0514459929f * c.b;
    float m = 0.2119034982f * c.r + 0.6806995451f * c.g + 0.1073969566f * c.b;
    float s = 0.0883024619f * c.r + 0.2817188376f * c.g + 0.6299787005f * c.b;

    float l_ = safe_cbrt(l);
    float m_ = safe_cbrt(m);
    float s_ = safe_cbrt(s);

    return Oklab(
        0.2104542553f * l_ + 0.7936177850f * m_ - 0.0040720468f * s_,
        1.9779984951f * l_ - 2.4285922050f * m_ + 0.4505937099f * s_,
        0.0259040371f * l_ + 0.7827717662f * m_ - 0.8086757660f * s_
    );
}

LinearRgb oklab_to_linear(Oklab c) noexcept {
    float l_ = c.l + 0.3963377774f * c.a + 0.2158037573f * c.b;
    float m_ = c.l - 0.1055613458f * c.a - 0.0638541728f * c.b;
    float s_ = c.l - 0.0894841775f * c.a - 1.2914855480f * c.b;

    float l = l_ * l_ * l_;
    float m = m_ * m_ * m_;
    float s = s_ * s_ * s_;

    return LinearRgb(
        +4.0767416621f * l - 3.3077115913f * m + 0.2309699292f * s,
        -1.2684380046f * l + 2.6097574011f * m - 0.3413193965f * s,
        -0.0041960863f * l - 0.7034186147f * m + 1.7076147010f * s
    );
}

Oklch oklab_to_oklch(Oklab c) noexcept {
    float chroma = std::sqrt(c.a * c.a + c.b * c.b);
    float hue = 0.0f;
    if (chroma > 1e-6f) {
        hue = std::atan2(c.b, c.a) * RAD2DEG;
        if (hue < 0.0f) hue += 360.0f;
    }
    return Oklch(c.l, chroma, hue);
}

Oklab oklch_to_oklab(Oklch c) noexcept {
    float rad = c.h * DEG2RAD;
    return Oklab(
        c.l,
        c.c * std::cos(rad),
        c.c * std::sin(rad)
    );
}

Hsl srgb_to_hsl(ColorF c) noexcept {
    float r = std::clamp(c.r, 0.0f, 1.0f);
    float g = std::clamp(c.g, 0.0f, 1.0f);
    float b = std::clamp(c.b, 0.0f, 1.0f);

    float max_v = std::max({r, g, b});
    float min_v = std::min({r, g, b});
    float delta = max_v - min_v;

    float l = (max_v + min_v) * 0.5f;
    float h = 0.0f;
    float s = 0.0f;

    if (delta > 1e-6f) {
        s = (l > 0.5f) ? delta / (2.0f - max_v - min_v) : delta / (max_v + min_v);

        if (max_v == r) {
            h = (g - b) / delta + (g < b ? 6.0f : 0.0f);
        } else if (max_v == g) {
            h = (b - r) / delta + 2.0f;
        } else {
            h = (r - g) / delta + 4.0f;
        }
        h *= 60.0f;
    }

    return Hsl(h, s, l);
}

ColorF hsl_to_srgb(Hsl c) noexcept {
    float h = c.h / 360.0f;
    float s = std::clamp(c.s, 0.0f, 1.0f);
    float l = std::clamp(c.l, 0.0f, 1.0f);

    if (s <= 1e-6f) {
        return ColorF(l, l, l, 1.0f);
    }

    auto hue_to_rgb = [](float p, float q, float t) -> float {
        if (t < 0.0f) t += 1.0f;
        if (t > 1.0f) t -= 1.0f;
        if (t < 1.0f / 6.0f) return p + (q - p) * 6.0f * t;
        if (t < 1.0f / 2.0f) return q;
        if (t < 2.0f / 3.0f) return p + (q - p) * (2.0f / 3.0f - t) * 6.0f;
        return p;
    };

    float q = l < 0.5f ? l * (1.0f + s) : l + s - l * s;
    float p = 2.0f * l - q;

    return ColorF(
        hue_to_rgb(p, q, h + 1.0f / 3.0f),
        hue_to_rgb(p, q, h),
        hue_to_rgb(p, q, h - 1.0f / 3.0f),
        1.0f
    );
}

Hsv srgb_to_hsv(ColorF c) noexcept {
    float r = std::clamp(c.r, 0.0f, 1.0f);
    float g = std::clamp(c.g, 0.0f, 1.0f);
    float b = std::clamp(c.b, 0.0f, 1.0f);

    float max_v = std::max({r, g, b});
    float min_v = std::min({r, g, b});
    float delta = max_v - min_v;

    float v = max_v;
    float s = (max_v > 1e-6f) ? (delta / max_v) : 0.0f;
    float h = 0.0f;

    if (delta > 1e-6f) {
        if (max_v == r) {
            h = (g - b) / delta + (g < b ? 6.0f : 0.0f);
        } else if (max_v == g) {
            h = (b - r) / delta + 2.0f;
        } else {
            h = (r - g) / delta + 4.0f;
        }
        h *= 60.0f;
    }

    return Hsv(h, s, v);
}

ColorF hsv_to_srgb(Hsv c) noexcept {
    float h = std::fmod(c.h, 360.0f);
    if (h < 0.0f) h += 360.0f;
    float s = std::clamp(c.s, 0.0f, 1.0f);
    float v = std::clamp(c.v, 0.0f, 1.0f);

    if (s <= 1e-6f) {
        return ColorF(v, v, v, 1.0f);
    }

    float sector = h / 60.0f;
    int i = static_cast<int>(std::floor(sector));
    float f = sector - static_cast<float>(i);
    float p = v * (1.0f - s);
    float q = v * (1.0f - s * f);
    float t = v * (1.0f - s * (1.0f - f));

    switch (i % 6) {
        case 0: return ColorF(v, t, p, 1.0f);
        case 1: return ColorF(q, v, p, 1.0f);
        case 2: return ColorF(p, v, t, 1.0f);
        case 3: return ColorF(p, q, v, 1.0f);
        case 4: return ColorF(t, p, v, 1.0f);
        case 5: default: return ColorF(v, p, q, 1.0f);
    }
}

Color from_linear(LinearRgb c) noexcept {
    return to_color(linear_to_srgb(c));
}

LinearRgb to_linear(Color c) noexcept {
    return srgb_to_linear(to_color_f(c));
}

Color from_xyz(Xyz c) noexcept {
    return from_linear(xyz_to_linear(c));
}

Xyz to_xyz(Color c) noexcept {
    return linear_to_xyz(to_linear(c));
}

Color from_lab(Lab c) noexcept {
    return from_xyz(lab_to_xyz(c));
}

Lab to_lab(Color c) noexcept {
    return xyz_to_lab(to_xyz(c));
}

Color from_oklab(Oklab c) noexcept {
    return from_linear(oklab_to_linear(c));
}

Oklab to_oklab(Color c) noexcept {
    return linear_to_oklab(to_linear(c));
}

Color from_oklch(Oklch c) noexcept {
    return from_oklab(oklch_to_oklab(c));
}

Oklch to_oklch(Color c) noexcept {
    return oklab_to_oklch(to_oklab(c));
}

Color from_hsl(Hsl c) noexcept {
    return to_color(hsl_to_srgb(c));
}

Hsl to_hsl(Color c) noexcept {
    return srgb_to_hsl(to_color_f(c));
}

Color from_hsv(Hsv c) noexcept {
    return to_color(hsv_to_srgb(c));
}

Hsv to_hsv(Color c) noexcept {
    return srgb_to_hsv(to_color_f(c));
}

bool is_in_srgb_gamut(LinearRgb c) noexcept {
    constexpr float eps = 1e-4f;
    return c.r >= -eps && c.r <= 1.0f + eps &&
           c.g >= -eps && c.g <= 1.0f + eps &&
           c.b >= -eps && c.b <= 1.0f + eps;
}

LinearRgb clamp_to_srgb_gamut(LinearRgb c) noexcept {
    return LinearRgb(
        std::clamp(c.r, 0.0f, 1.0f),
        std::clamp(c.g, 0.0f, 1.0f),
        std::clamp(c.b, 0.0f, 1.0f)
    );
}

Oklch fit_oklch_to_gamut(Oklch c) noexcept {
    c.l = std::clamp(c.l, 0.0f, 1.0f);
    if (c.l <= 0.0f || c.l >= 1.0f) {
        c.c = 0.0f;
        return c;
    }

    auto test_c = [](Oklch trial) -> bool {
        Oklab lab = oklch_to_oklab(trial);
        LinearRgb lin = oklab_to_linear(lab);
        return is_in_srgb_gamut(lin);
    };

    if (test_c(c)) {
        return c;
    }

    // Binary search chroma down to 0 while keeping L and h identical
    float low = 0.0f;
    float high = c.c;
    for (int iter = 0; iter < 16; ++iter) {
        float mid = (low + high) * 0.5f;
        Oklch trial(c.l, mid, c.h);
        if (test_c(trial)) {
            low = mid;
        } else {
            high = mid;
        }
    }

    c.c = low;
    return c;
}

} // namespace bro::themes

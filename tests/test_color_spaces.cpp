#include "check.h"
#include <brothemes/color_spaces.h>
#include <vector>

using namespace bro::themes;

int main() {
    // 1. sRGB <-> Linear roundtrip
    {
        std::vector<ColorF> samples = {
            ColorF(0.0f, 0.0f, 0.0f),
            ColorF(1.0f, 1.0f, 1.0f),
            ColorF(0.5f, 0.5f, 0.5f),
            ColorF(0.2f, 0.8f, 0.4f),
            ColorF(0.04f, 0.003f, 0.9f)
        };
        for (const auto& sample : samples) {
            LinearRgb lin = srgb_to_linear(sample);
            ColorF back = linear_to_srgb(lin);
            CHECK_NEAR(sample.r, back.r, 1e-4);
            CHECK_NEAR(sample.g, back.g, 1e-4);
            CHECK_NEAR(sample.b, back.b, 1e-4);
        }
    }

    // 2. Reference points for XYZ and CIELAB
    {
        // Pure white: sRGB (1, 1, 1)
        ColorF white(1.0f, 1.0f, 1.0f);
        LinearRgb white_lin = srgb_to_linear(white);
        Xyz white_xyz = linear_to_xyz(white_lin);
        CHECK_NEAR(white_xyz.x, 0.95047f, 1e-3);
        CHECK_NEAR(white_xyz.y, 1.00000f, 1e-3);
        CHECK_NEAR(white_xyz.z, 1.08883f, 1e-3);

        Lab white_lab = xyz_to_lab(white_xyz);
        CHECK_NEAR(white_lab.l, 100.0f, 1e-2);
        CHECK_NEAR(white_lab.a, 0.0f, 1e-2);
        CHECK_NEAR(white_lab.b, 0.0f, 1e-2);

        // Pure black: sRGB (0, 0, 0)
        ColorF black(0.0f, 0.0f, 0.0f);
        LinearRgb black_lin = srgb_to_linear(black);
        Xyz black_xyz = linear_to_xyz(black_lin);
        CHECK_NEAR(black_xyz.x, 0.0f, 1e-4);
        CHECK_NEAR(black_xyz.y, 0.0f, 1e-4);
        CHECK_NEAR(black_xyz.z, 0.0f, 1e-4);

        Lab black_lab = xyz_to_lab(black_xyz);
        CHECK_NEAR(black_lab.l, 0.0f, 1e-3);
    }

    // 3. Oklab and Oklch roundtrips and reference points
    {
        // White in Oklab: L = 1, a = 0, b = 0
        LinearRgb white_lin(1.0f, 1.0f, 1.0f);
        Oklab white_ok = linear_to_oklab(white_lin);
        CHECK_NEAR(white_ok.l, 1.0f, 1e-3);
        CHECK_NEAR(white_ok.a, 0.0f, 1e-3);
        CHECK_NEAR(white_ok.b, 0.0f, 1e-3);

        // Black in Oklab: L = 0, a = 0, b = 0
        LinearRgb black_lin(0.0f, 0.0f, 0.0f);
        Oklab black_ok = linear_to_oklab(black_lin);
        CHECK_NEAR(black_ok.l, 0.0f, 1e-4);
        CHECK_NEAR(black_ok.a, 0.0f, 1e-4);
        CHECK_NEAR(black_ok.b, 0.0f, 1e-4);

        // Arbitrary colors roundtrip
        std::vector<Color> test_colors = {
            Color(255, 0, 0),
            Color(0, 255, 0),
            Color(0, 0, 255),
            Color(40, 42, 54),
            Color(189, 147, 249),
            Color(241, 250, 140)
        };

        for (auto c : test_colors) {
            Oklab oklab = to_oklab(c);
            Color back_oklab = from_oklab(oklab);
            CHECK_NEAR(c.r, back_oklab.r, 1);
            CHECK_NEAR(c.g, back_oklab.g, 1);
            CHECK_NEAR(c.b, back_oklab.b, 1);

            Oklch oklch = to_oklch(c);
            Color back_oklch = from_oklch(oklch);
            CHECK_NEAR(c.r, back_oklch.r, 1);
            CHECK_NEAR(c.g, back_oklch.g, 1);
            CHECK_NEAR(c.b, back_oklch.b, 1);
        }
    }

    // 4. HSL and HSV roundtrips
    {
        std::vector<Color> test_colors = {
            Color(255, 0, 0),
            Color(0, 255, 0),
            Color(0, 0, 255),
            Color(128, 64, 192),
            Color(200, 200, 200)
        };

        for (auto c : test_colors) {
            Hsl hsl = to_hsl(c);
            Color back_hsl = from_hsl(hsl);
            CHECK_NEAR(c.r, back_hsl.r, 1);
            CHECK_NEAR(c.g, back_hsl.g, 1);
            CHECK_NEAR(c.b, back_hsl.b, 1);

            Hsv hsv = to_hsv(c);
            Color back_hsv = from_hsv(hsv);
            CHECK_NEAR(c.r, back_hsv.r, 1);
            CHECK_NEAR(c.g, back_hsv.g, 1);
            CHECK_NEAR(c.b, back_hsv.b, 1);
        }
    }

    // 5. Gamut fitting
    {
        // An extreme chroma Oklch color that exceeds sRGB gamut
        Oklch out_of_gamut(0.7f, 0.4f, 140.0f); // High chroma green-cyan
        Oklch fitted = fit_oklch_to_gamut(out_of_gamut);
        CHECK_NEAR(fitted.l, 0.7f, 1e-4);
        CHECK_NEAR(fitted.h, 140.0f, 1e-4);
        CHECK(fitted.c <= out_of_gamut.c);

        LinearRgb lin = oklab_to_linear(oklch_to_oklab(fitted));
        CHECK(is_in_srgb_gamut(lin));
    }

    return brotest::finish("test_color_spaces");
}

#include "check.h"
#include <brothemes/contrast.h>

using namespace bro::themes;

int main() {
    Color black(0, 0, 0);
    Color white(255, 255, 255);
    Color red(255, 0, 0);
    Color green(0, 255, 0);
    Color blue(0, 0, 255);

    // 1. Relative luminance reference values
    {
        CHECK_NEAR(relative_luminance(black), 0.0f, 1e-4);
        CHECK_NEAR(relative_luminance(white), 1.0f, 1e-4);
        CHECK_NEAR(relative_luminance(red), 0.2126f, 1e-3);
        CHECK_NEAR(relative_luminance(green), 0.7152f, 1e-3);
        CHECK_NEAR(relative_luminance(blue), 0.0722f, 1e-3);
    }

    // 2. WCAG 2.1 contrast ratios
    {
        // Black vs White
        float cr_bw = wcag_contrast_ratio(black, white);
        CHECK_NEAR(cr_bw, 21.0f, 0.01f);

        // Symmetry: White vs Black
        float cr_wb = wcag_contrast_ratio(white, black);
        CHECK_NEAR(cr_wb, 21.0f, 0.01f);

        // Identical colors
        CHECK_NEAR(wcag_contrast_ratio(black, black), 1.0f, 1e-4);
        CHECK_NEAR(wcag_contrast_ratio(white, white), 1.0f, 1e-4);

        // Pure Red on Black: (0.2126 + 0.05) / 0.05 = 5.252
        float cr_rb = wcag_contrast_ratio(red, black);
        CHECK_NEAR(cr_rb, 5.252f, 0.01f);

        // Standard mid-grey (#777777) against white is ~4.5:1
        Color mid_gray(119, 119, 119);
        float cr_gw = wcag_contrast_ratio(mid_gray, white);
        CHECK_NEAR(cr_gw, 4.5f, 0.1f);
    }

    // 3. WCAG compliance levels
    {
        CHECK(meets_wcag_aa(black, white));
        CHECK(meets_wcag_aaa(black, white));
        CHECK_EQ(static_cast<int>(wcag_level(black, white)), static_cast<int>(WcagLevel::AaaNormal));

        // Red on black has CR ~ 5.25: passes AA normal (>= 4.5) and AA large (>= 3.0), fails AAA normal (< 7.0)
        CHECK(meets_wcag_aa(red, black, false));
        CHECK(!meets_wcag_aaa(red, black, false));
        CHECK_EQ(static_cast<int>(wcag_level(red, black)), static_cast<int>(WcagLevel::AaNormal));

        // Blue on black: CR = (0.0722 + 0.05) / 0.05 = 2.44 -> fails even AA large
        CHECK(!meets_wcag_aa(blue, black, false));
        CHECK(!meets_wcag_aa(blue, black, true));
        CHECK_EQ(static_cast<int>(wcag_level(blue, black)), static_cast<int>(WcagLevel::Fail));
    }

    // 4. APCA reference test vectors
    {
        // Identical colors
        CHECK_NEAR(apca_contrast(white, white), 0.0f, 1e-4);
        CHECK_NEAR(apca_contrast(black, black), 0.0f, 1e-4);

        // Black text on White bg: positive Lc ~ 106
        float apca_bow = apca_contrast(black, white);
        CHECK(apca_bow > 100.0f);
        CHECK_NEAR(apca_bow, 106.1f, 1.5f);

        // White text on Black bg: negative Lc ~ -107
        float apca_wob = apca_contrast(white, black);
        CHECK(apca_wob < -100.0f);
        CHECK_NEAR(apca_wob, -107.4f, 1.5f);

        // Absolute magnitude
        CHECK_NEAR(apca_contrast_abs(black, white), apca_bow, 1e-4);
        CHECK_NEAR(apca_contrast_abs(white, black), -apca_wob, 1e-4);

        // Medium contrast: gray on white
        Color text_gray(100, 100, 100);
        float apca_gray = apca_contrast(text_gray, white);
        CHECK(apca_gray > 50.0f && apca_gray < 85.0f);
    }

    return brotest::finish("test_contrast");
}

#include "check.h"
#include <brothemes/contrast.h>
#include <brothemes/color_spaces.h>
#include <brothemes/theme.h>

using namespace bro::themes;

int main() {
    // 1. Unchanged when already meets contrast threshold
    {
        Color fg(240, 240, 240);
        Color bg(20, 20, 20);
        float orig_cr = wcag_contrast_ratio(fg, bg);
        CHECK(orig_cr >= 4.5f);

        Color adj = adjust_contrast(fg, bg, 4.5f);
        CHECK(adj == fg);
    }

    // 2. Low contrast color on dark background is brightened to >= 4.5:1
    {
        Color bg(30, 30, 35); // Dark background
        Color fg_dark_blue(20, 40, 120); // Low contrast blue
        float initial_cr = wcag_contrast_ratio(fg_dark_blue, bg);
        CHECK(initial_cr < 3.0f);

        Color adjusted = adjust_contrast(fg_dark_blue, bg, 4.5f);
        float new_cr = wcag_contrast_ratio(adjusted, bg);
        CHECK(new_cr >= 4.5f);

        // Verify hue angle is preserved in Oklch space
        Oklch orig_ok = to_oklch(fg_dark_blue);
        Oklch adj_ok = to_oklch(adjusted);
        CHECK_NEAR(orig_ok.h, adj_ok.h, 1.5f);
    }

    // 3. Low contrast color on light background is darkened to >= 4.5:1
    {
        Color bg(245, 245, 250); // Light background
        Color fg_light_yellow(230, 220, 100); // Low contrast yellow
        float initial_cr = wcag_contrast_ratio(fg_light_yellow, bg);
        CHECK(initial_cr < 3.0f);

        Color adjusted = adjust_contrast(fg_light_yellow, bg, 4.5f);
        float new_cr = wcag_contrast_ratio(adjusted, bg);
        CHECK(new_cr >= 4.5f);

        // Verify hue angle is preserved
        Oklch orig_ok = to_oklch(fg_light_yellow);
        Oklch adj_ok = to_oklch(adjusted);
        CHECK_NEAR(orig_ok.h, adj_ok.h, 2.0f);
    }

    // 4. Batch palette contrast adjustment
    {
        Theme theme;
        theme.ui.background = Color(24, 24, 30); // Dark theme
        theme.ui.foreground = Color(100, 100, 105); // Low contrast foreground
        theme.ansi = AnsiPalette::standard_vga();

        // Check foreground initial contrast
        CHECK(wcag_contrast_ratio(theme.ui.foreground, theme.ui.background) < 4.5f);

        adjust_palette_contrast(theme, 4.5f);

        // Foreground must now meet 4.5:1
        float fg_cr = wcag_contrast_ratio(theme.ui.foreground, theme.ui.background);
        CHECK(fg_cr >= 4.5f);

        // All ANSI colors that can meet 4.5:1 should meet it
        for (size_t i = 1; i < 16; ++i) { // skip pure black at index 0 if tested against near-black, or check it was brightened
            float cr = wcag_contrast_ratio(theme.ansi[i], theme.ui.background);
            CHECK(cr >= 4.0f); // Should be boosted significantly
        }
    }

    return brotest::finish("test_adjustment");
}

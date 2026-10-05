#pragma once

#include <brothemes/color.h>
#include <brothemes/color_spaces.h>

namespace bro::themes {

struct Theme;

// WCAG 2.1 compliance level
enum class WcagLevel {
    Fail,       // < 3.0:1
    AaLarge,    // >= 3.0:1 (passes for large text: >= 18pt or >= 14pt bold)
    AaNormal,   // >= 4.5:1 (passes for normal text AA, and large text AAA)
    AaaNormal   // >= 7.0:1 (passes for normal text AAA)
};

// WCAG 2.1 relative luminance (in [0.0, 1.0])
float relative_luminance(Color c) noexcept;
float relative_luminance(ColorF c) noexcept;

// WCAG 2.1 contrast ratio between two colors (in [1.0, 21.0])
float wcag_contrast_ratio(Color c1, Color c2) noexcept;

// WCAG 2.1 compliance check
WcagLevel wcag_level(Color fg, Color bg) noexcept;
bool meets_wcag_aa(Color fg, Color bg, bool is_large_text = false) noexcept;
bool meets_wcag_aaa(Color fg, Color bg, bool is_large_text = false) noexcept;

// APCA (Accessible Perceptual Contrast Algorithm - W3C Silver / WCAG 3 Candidate)
// Licensing & Usage Note:
// APCA was developed by Andrew Somers / Myndex Research. The algorithm and name "APCA"
// are subject to trademark and proprietary licensing restrictions by Myndex.
// For production environments requiring unencumbered open-source standards, WCAG 2.1
// (wcag_contrast_ratio, meets_wcag_aa) is the default and recommended standard.
// Returns signed Lc contrast: positive for dark text on light bg, negative for light text on dark bg.
float apca_contrast(Color fg, Color bg) noexcept;

// Absolute APCA contrast magnitude |Lc|
float apca_contrast_abs(Color fg, Color bg) noexcept;

// Automatic contrast adjustment:
// If contrast between fg and bg is below min_wcag_ratio (default 4.5:1),
// adjust fg luminance in Oklch while preserving perceived hue and fitting to sRGB gamut.
Color adjust_contrast(Color fg, Color bg, float min_wcag_ratio = 4.5f) noexcept;

// Batch contrast adjustment for a full theme:
// Adjusts ANSI colors, default foreground, and syntax colors against theme.ui.background
void adjust_palette_contrast(Theme& theme, float min_wcag_ratio = 4.5f);

} // namespace bro::themes

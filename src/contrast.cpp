#include <brothemes/contrast.h>
#include <brothemes/theme.h>
#include <algorithm>
#include <cmath>

namespace bro::themes {

namespace {

// APCA 0.98G reference constants
constexpr double APCA_MAIN_TRC = 2.4;
constexpr double APCA_R_CO = 0.2126729;
constexpr double APCA_G_CO = 0.7151522;
constexpr double APCA_B_CO = 0.0721750;

constexpr double APCA_BLK_THRS = 0.022;
constexpr double APCA_BLK_CLMP = 1.414;

constexpr double APCA_SCALE = 1.14;
constexpr double APCA_OFFSET = 0.027;

constexpr double APCA_NORM_BG = 0.56;
constexpr double APCA_NORM_TXT = 0.57;
constexpr double APCA_REV_BG = 0.62;
constexpr double APCA_REV_TXT = 0.65;

double compute_apca_y(Color c) noexcept {
    double r = std::pow(static_cast<double>(c.r) / 255.0, APCA_MAIN_TRC);
    double g = std::pow(static_cast<double>(c.g) / 255.0, APCA_MAIN_TRC);
    double b = std::pow(static_cast<double>(c.b) / 255.0, APCA_MAIN_TRC);

    double y = APCA_R_CO * r + APCA_G_CO * g + APCA_B_CO * b;
    if (y < APCA_BLK_THRS) {
        y += std::pow(APCA_BLK_THRS - y, APCA_BLK_CLMP);
    }
    return y;
}

} // namespace

float relative_luminance(Color c) noexcept {
    return relative_luminance(to_color_f(c));
}

float relative_luminance(ColorF c) noexcept {
    LinearRgb lin = srgb_to_linear(c);
    return 0.2126f * lin.r + 0.7152f * lin.g + 0.0722f * lin.b;
}

float wcag_contrast_ratio(Color c1, Color c2) noexcept {
    float l1 = relative_luminance(c1);
    float l2 = relative_luminance(c2);

    float brighter = std::max(l1, l2);
    float darker = std::min(l1, l2);

    return (brighter + 0.05f) / (darker + 0.05f);
}

WcagLevel wcag_level(Color fg, Color bg) noexcept {
    float cr = wcag_contrast_ratio(fg, bg);
    if (cr >= 7.0f) return WcagLevel::AaaNormal;
    if (cr >= 4.5f) return WcagLevel::AaNormal;
    if (cr >= 3.0f) return WcagLevel::AaLarge;
    return WcagLevel::Fail;
}

bool meets_wcag_aa(Color fg, Color bg, bool is_large_text) noexcept {
    float cr = wcag_contrast_ratio(fg, bg);
    return is_large_text ? (cr >= 3.0f) : (cr >= 4.5f);
}

bool meets_wcag_aaa(Color fg, Color bg, bool is_large_text) noexcept {
    float cr = wcag_contrast_ratio(fg, bg);
    return is_large_text ? (cr >= 4.5f) : (cr >= 7.0f);
}

float apca_contrast(Color fg, Color bg) noexcept {
    double y_txt = compute_apca_y(fg);
    double y_bg = compute_apca_y(bg);

    if (std::abs(y_bg - y_txt) < 1e-7) {
        return 0.0f;
    }

    if (y_bg > y_txt) {
        // Dark text on light background
        double sapca = (std::pow(y_bg, APCA_NORM_BG) - std::pow(y_txt, APCA_NORM_TXT)) * APCA_SCALE;
        if (sapca > APCA_OFFSET) {
            return static_cast<float>((sapca - APCA_OFFSET) * 100.0);
        }
        return 0.0f;
    } else {
        // Light text on dark background
        double sapca = (std::pow(y_bg, APCA_REV_BG) - std::pow(y_txt, APCA_REV_TXT)) * APCA_SCALE;
        if (sapca < -APCA_OFFSET) {
            return static_cast<float>((sapca + APCA_OFFSET) * 100.0);
        }
        return 0.0f;
    }
}

float apca_contrast_abs(Color fg, Color bg) noexcept {
    return std::abs(apca_contrast(fg, bg));
}

Color adjust_contrast(Color fg, Color bg, float min_wcag_ratio) noexcept {
    float cur_cr = wcag_contrast_ratio(fg, bg);
    if (cur_cr >= min_wcag_ratio) {
        return fg;
    }

    Oklch orig_oklch = to_oklch(fg);
    float bg_lum = relative_luminance(bg);

    // Helper: evaluate candidate L
    auto eval_color = [&](float trial_l) -> std::pair<Color, float> {
        Oklch trial(trial_l, orig_oklch.c, orig_oklch.h);
        Oklch fitted = fit_oklch_to_gamut(trial);
        Color cand = from_oklch(fitted);
        float cr = wcag_contrast_ratio(cand, bg);
        return {cand, cr};
    };

    // Determine primary search direction
    // If background is dark (bg_lum < 0.18f) or fg is brighter than bg, prefer brightening fg
    bool brighten_first = (bg_lum < 0.18f) || (relative_luminance(fg) >= bg_lum);

    auto search_range = [&](float l_start, float l_target) -> std::pair<Color, float> {
        float min_l = std::min(l_start, l_target);
        float max_l = std::max(l_start, l_target);
        Color best_color = fg;
        float best_cr = cur_cr;

        for (int iter = 0; iter < 24; ++iter) {
            float mid = (min_l + max_l) * 0.5f;
            auto [cand, cr] = eval_color(mid);
            if (cr > best_cr) {
                best_cr = cr;
                best_color = cand;
            }

            if (l_target > l_start) { // searching brighter (target 1.0)
                if (cr >= min_wcag_ratio) {
                    max_l = mid; // can we use lower brightness and still satisfy?
                    best_color = cand;
                } else {
                    min_l = mid; // need more brightness
                }
            } else { // searching darker (target 0.0)
                if (cr >= min_wcag_ratio) {
                    min_l = mid; // can we use higher L and still satisfy?
                    best_color = cand;
                } else {
                    max_l = mid; // need more darkening
                }
            }
        }
        return {best_color, best_cr};
    };

    std::pair<Color, float> primary;
    std::pair<Color, float> secondary;

    if (brighten_first) {
        primary = search_range(orig_oklch.l, 1.0f);
        if (primary.second >= min_wcag_ratio) {
            return primary.first;
        }
        secondary = search_range(orig_oklch.l, 0.0f);
    } else {
        primary = search_range(orig_oklch.l, 0.0f);
        if (primary.second >= min_wcag_ratio) {
            return primary.first;
        }
        secondary = search_range(orig_oklch.l, 1.0f);
    }

    if (secondary.second >= min_wcag_ratio) {
        return secondary.first;
    }

    // Pick whichever achieved higher contrast
    return (primary.second >= secondary.second) ? primary.first : secondary.first;
}

void adjust_palette_contrast(Theme& theme, float min_wcag_ratio) {
    Color bg = theme.ui.background;

    // Adjust foreground
    theme.ui.foreground = adjust_contrast(theme.ui.foreground, bg, min_wcag_ratio);

    // Adjust cursor text against cursor, cursor against background
    if (theme.ui.cursor.has_value()) {
        theme.ui.cursor = adjust_contrast(*theme.ui.cursor, bg, min_wcag_ratio);
    }
    if (theme.ui.cursor_text.has_value() && theme.ui.cursor.has_value()) {
        theme.ui.cursor_text = adjust_contrast(*theme.ui.cursor_text, *theme.ui.cursor, min_wcag_ratio);
    }

    // Adjust ANSI 16 colors
    for (size_t i = 0; i < 16; ++i) {
        theme.ansi[i] = adjust_contrast(theme.ansi[i], bg, min_wcag_ratio);
    }

    // Adjust syntax token colors against background
    auto adjust_opt = [&](std::optional<Color>& slot) {
        if (slot.has_value()) {
            slot = adjust_contrast(*slot, bg, min_wcag_ratio);
        }
    };

    adjust_opt(theme.syntax.comment);
    adjust_opt(theme.syntax.string);
    adjust_opt(theme.syntax.keyword);
    adjust_opt(theme.syntax.number);
    adjust_opt(theme.syntax.function);
    adjust_opt(theme.syntax.type);
    adjust_opt(theme.syntax.variable);
    adjust_opt(theme.syntax.constant);
    adjust_opt(theme.syntax.operator_color);
    adjust_opt(theme.syntax.punctuation);
    adjust_opt(theme.syntax.error);
    adjust_opt(theme.syntax.warning);
    adjust_opt(theme.syntax.info);
    adjust_opt(theme.syntax.hint);
    adjust_opt(theme.syntax.markup_heading);
    adjust_opt(theme.syntax.markup_link);
    adjust_opt(theme.syntax.markup_code);
}

} // namespace bro::themes

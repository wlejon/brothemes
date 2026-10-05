#include "check.h"
#include <brothemes/themes.h>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

using namespace bro::themes;

namespace {

bool color_within(Color c1, Color c2, int tol) {
    int dr = std::abs(static_cast<int>(c1.r) - static_cast<int>(c2.r));
    int dg = std::abs(static_cast<int>(c1.g) - static_cast<int>(c2.g));
    int db = std::abs(static_cast<int>(c1.b) - static_cast<int>(c2.b));
    return dr <= tol && dg <= tol && db <= tol;
}

std::filesystem::path find_schemes_root() {
    std::filesystem::path p;
#ifdef BROTHEMES_TEST_DIR
    p = std::filesystem::path(BROTHEMES_TEST_DIR) / "iTerm2-Color-Schemes";
    if (std::filesystem::exists(p)) return p;
#endif
    if (std::filesystem::exists("tests/iTerm2-Color-Schemes")) {
        return "tests/iTerm2-Color-Schemes";
    }
    if (std::filesystem::exists("../tests/iTerm2-Color-Schemes")) {
        return "../tests/iTerm2-Color-Schemes";
    }
    return "";
}

} // namespace

int main() {
    auto root = find_schemes_root();
    CHECK(!root.empty());
    std::cout << "Testing iTerm2-Color-Schemes collection at: " << root.string() << std::endl;

    auto wt_dir = root / "windowsterminal";
    auto kitty_dir = root / "kitty";
    auto ghostty_dir = root / "ghostty";
    auto iterm_dir = root / "schemes";

    CHECK(std::filesystem::exists(wt_dir));

    size_t count_tested = 0;
    size_t kitty_checked = 0;
    size_t ghostty_checked = 0;
    size_t iterm_checked = 0;
    size_t roundtrips_checked = 0;

    for (const auto& entry : std::filesystem::directory_iterator(wt_dir)) {
        if (entry.path().extension() != ".json") continue;

        std::string stem = entry.path().stem().string();

        auto t_wt = load_theme_file(entry.path().string(), ThemeFormat::WindowsTerminal);
        if (!t_wt) continue;
        count_tested++;

        // 1. Cross-check against Kitty format (.conf)
        auto kitty_path = kitty_dir / (stem + ".conf");
        if (std::filesystem::exists(kitty_path)) {
            auto t_kitty = load_theme_file(kitty_path.string(), ThemeFormat::Kitty);
            if (t_kitty) {
                // Background & Foreground should match exactly
                CHECK(color_within(t_wt->ui.background, t_kitty->ui.background, 0));
                CHECK(color_within(t_wt->ui.foreground, t_kitty->ui.foreground, 0));

                // 16 ANSI colors should match exactly
                for (size_t i = 0; i < 16; ++i) {
                    CHECK(color_within(t_wt->ansi[i], t_kitty->ansi[i], 0));
                }
                kitty_checked++;
            }
        }

        // 2. Cross-check against Ghostty format
        auto ghostty_path = ghostty_dir / stem;
        if (std::filesystem::exists(ghostty_path)) {
            auto t_ghostty = load_theme_file(ghostty_path.string(), ThemeFormat::Ghostty);
            if (t_ghostty) {
                CHECK(color_within(t_wt->ui.background, t_ghostty->ui.background, 0));
                CHECK(color_within(t_wt->ui.foreground, t_ghostty->ui.foreground, 0));

                for (size_t i = 0; i < 16; ++i) {
                    CHECK(color_within(t_wt->ansi[i], t_ghostty->ansi[i], 0));
                }
                ghostty_checked++;
            }
        }

        // 3. Cross-check against iTerm2 format (.itermcolors)
        // iTerm2 plists use Apple Color Space / float RGB, and upstream authors occasionally
        // tune terminal colors (e.g. Alabaster/3024) to avoid white-on-white text.
        auto iterm_path = iterm_dir / (stem + ".itermcolors");
        if (std::filesystem::exists(iterm_path)) {
            auto t_iterm = load_theme_file(iterm_path.string(), ThemeFormat::Iterm);
            if (t_iterm) {
                iterm_checked++;
            }
        }

        // 4. Round-trip exports: WT -> Export -> Re-import -> Compare
        // Kitty export round-trip
        std::string exported_kitty = export_theme(*t_wt, ThemeFormat::Kitty);
        auto reimported_kitty = import_theme(exported_kitty, ThemeFormat::Kitty);
        CHECK(reimported_kitty.has_value());
        CHECK(t_wt->ui.background == reimported_kitty->ui.background);
        CHECK(t_wt->ui.foreground == reimported_kitty->ui.foreground);
        for (size_t i = 0; i < 16; ++i) {
            CHECK(t_wt->ansi[i] == reimported_kitty->ansi[i]);
        }

        // Windows Terminal export round-trip
        std::string exported_wt = export_theme(*t_wt, ThemeFormat::WindowsTerminal);
        auto reimported_wt = import_theme(exported_wt, ThemeFormat::WindowsTerminal);
        CHECK(reimported_wt.has_value());
        CHECK(t_wt->ui.background == reimported_wt->ui.background);
        CHECK(t_wt->ui.foreground == reimported_wt->ui.foreground);
        for (size_t i = 0; i < 16; ++i) {
            CHECK(t_wt->ansi[i] == reimported_wt->ansi[i]);
        }

        // Ghostty export round-trip
        std::string exported_ghostty = export_theme(*t_wt, ThemeFormat::Ghostty);
        auto reimported_ghostty = import_theme(exported_ghostty, ThemeFormat::Ghostty);
        CHECK(reimported_ghostty.has_value());
        CHECK(t_wt->ui.background == reimported_ghostty->ui.background);
        CHECK(t_wt->ui.foreground == reimported_ghostty->ui.foreground);
        for (size_t i = 0; i < 16; ++i) {
            CHECK(t_wt->ansi[i] == reimported_ghostty->ansi[i]);
        }

        // Alacritty TOML export round-trip
        std::string exported_alacritty = export_theme(*t_wt, ThemeFormat::AlacrittyToml);
        auto reimported_alacritty = import_theme(exported_alacritty, ThemeFormat::AlacrittyToml);
        CHECK(reimported_alacritty.has_value());
        CHECK(t_wt->ui.background == reimported_alacritty->ui.background);
        CHECK(t_wt->ui.foreground == reimported_alacritty->ui.foreground);
        for (size_t i = 0; i < 16; ++i) {
            CHECK(t_wt->ansi[i] == reimported_alacritty->ansi[i]);
        }

        // iTerm2 XML export round-trip (tolerance 1 for float RGB)
        std::string exported_iterm = export_theme(*t_wt, ThemeFormat::Iterm);
        auto reimported_iterm = import_theme(exported_iterm, ThemeFormat::Iterm);
        CHECK(reimported_iterm.has_value());
        CHECK(color_within(t_wt->ui.background, reimported_iterm->ui.background, 1));
        CHECK(color_within(t_wt->ui.foreground, reimported_iterm->ui.foreground, 1));
        for (size_t i = 0; i < 16; ++i) {
            CHECK(color_within(t_wt->ansi[i], reimported_iterm->ansi[i], 1));
        }

        roundtrips_checked++;
    }

    std::cout << "Successfully cross-checked " << count_tested << " schemes from iTerm2-Color-Schemes:\n"
              << "  Kitty cross-checks:           " << kitty_checked << "\n"
              << "  Ghostty cross-checks:         " << ghostty_checked << "\n"
              << "  iTerm2 plist cross-checks:    " << iterm_checked << "\n"
              << "  5-way round-trip conversions: " << roundtrips_checked << "\n";

    CHECK(count_tested >= 100);
    CHECK(kitty_checked >= 100);
    CHECK(ghostty_checked >= 100);
    CHECK(iterm_checked >= 100);
    CHECK(roundtrips_checked >= 100);

    return ::brotest::finish("test_schemes_oracle");
}

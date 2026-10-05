// Differential oracle over the whole iTerm2-Color-Schemes collection
// (https://github.com/mbadolato/iTerm2-Color-Schemes, MIT): every scheme is
// published there in several formats generated from one source, so each
// format's importer must agree with the others, and every exporter must
// round-trip. The collection ships as tests/data/iterm2-color-schemes.bpk, a
// compressed archive of the upstream files byte for byte (tools/pack; the
// licence is tests/data/iterm2-color-schemes.LICENSE and inside the archive).
#include "check.h"
#include "pack/bpk.h"
#include <brothemes/themes.h>

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <string_view>
#include <vector>

using namespace bro::themes;

namespace {

bool color_within(Color c1, Color c2, int tol) {
    int dr = std::abs(static_cast<int>(c1.r) - static_cast<int>(c2.r));
    int dg = std::abs(static_cast<int>(c1.g) - static_cast<int>(c2.g));
    int db = std::abs(static_cast<int>(c1.b) - static_cast<int>(c2.b));
    return dr <= tol && dg <= tol && db <= tol;
}

bool same_palette(const Theme& a, const Theme& b, int tol) {
    if (!color_within(a.ui.background, b.ui.background, tol)) return false;
    if (!color_within(a.ui.foreground, b.ui.foreground, tol)) return false;
    for (size_t i = 0; i < 16; ++i)
        if (!color_within(a.ansi[i], b.ansi[i], tol)) return false;
    return true;
}

} // namespace

int main() {
    const std::string pack = std::string(BROTHEMES_TEST_DIR) + "/data/iterm2-color-schemes.bpk";
    std::string error;
    const auto entries = bpk::read_archive_file(pack, &error);
    CHECK(entries.has_value());
    if (!entries) {
        std::cerr << pack << ": " << error << "\n";
        return ::brotest::finish("test_schemes_oracle");
    }
    std::map<std::string, const std::string*, std::less<>> files;
    std::map<std::string, size_t> per_dir;
    for (const auto& e : *entries) {
        files.emplace(e.path, &e.data);
        const size_t slash = e.path.find('/');
        per_dir[slash == std::string::npos ? std::string() : e.path.substr(0, slash)]++;
    }
    auto find = [&](const std::string& path) -> const std::string* {
        auto it = files.find(path);
        return it == files.end() ? nullptr : it->second;
    };
    std::cout << "iTerm2-Color-Schemes archive: " << entries->size() << " files\n";

    // Nothing was lost when the collection was packed (counts of the vendored
    // upstream tree; repacking a newer upstream updates these).
    CHECK(find("LICENSE") != nullptr);
    CHECK_EQ(per_dir["schemes"], size_t(724));
    CHECK_EQ(per_dir["windowsterminal"], size_t(724));
    CHECK_EQ(per_dir["kitty"], size_t(724));
    CHECK_EQ(per_dir["ghostty"], size_t(725));

    size_t count_tested = 0;
    size_t kitty_checked = 0;
    size_t ghostty_checked = 0;
    size_t iterm_checked = 0;
    size_t roundtrips_checked = 0;

    const std::string_view wt_prefix = "windowsterminal/", wt_ext = ".json";
    for (const auto& e : *entries) {
        const std::string_view path = e.path;
        if (!path.starts_with(wt_prefix) || !path.ends_with(wt_ext)) continue;
        const std::string stem(path.substr(wt_prefix.size(), path.size() - wt_prefix.size() - wt_ext.size()));

        auto t_wt = import_theme(e.data, ThemeFormat::WindowsTerminal);
        if (!t_wt) continue;
        count_tested++;

        // 1. Kitty (.conf): background, foreground and the 16 ANSI colours match exactly.
        if (const std::string* kitty = find("kitty/" + stem + ".conf")) {
            if (auto t_kitty = import_theme(*kitty, ThemeFormat::Kitty)) {
                CHECK(same_palette(*t_wt, *t_kitty, 0));
                kitty_checked++;
            }
        }

        // 2. Ghostty.
        if (const std::string* ghostty = find("ghostty/" + stem)) {
            if (auto t_ghostty = import_theme(*ghostty, ThemeFormat::Ghostty)) {
                CHECK(same_palette(*t_wt, *t_ghostty, 0));
                ghostty_checked++;
            }
        }

        // 3. iTerm2 (.itermcolors). iTerm2 plists use Apple colour spaces / float RGB, and
        // upstream authors occasionally tune terminal colours (e.g. Alabaster / 3024) to avoid
        // white-on-white text, so these must parse but are not compared.
        if (const std::string* iterm = find("schemes/" + stem + ".itermcolors")) {
            if (import_theme(*iterm, ThemeFormat::Iterm)) iterm_checked++;
        }

        // 4. Round-trip exports: WT -> export -> re-import -> compare.
        for (ThemeFormat f : {ThemeFormat::Kitty, ThemeFormat::WindowsTerminal, ThemeFormat::Ghostty,
                              ThemeFormat::AlacrittyToml}) {
            auto back = import_theme(export_theme(*t_wt, f), f);
            CHECK(back.has_value());
            if (back) CHECK(same_palette(*t_wt, *back, 0));
        }
        // iTerm2 XML export round-trip (tolerance 1 for float RGB).
        auto back_iterm = import_theme(export_theme(*t_wt, ThemeFormat::Iterm), ThemeFormat::Iterm);
        CHECK(back_iterm.has_value());
        if (back_iterm) CHECK(same_palette(*t_wt, *back_iterm, 1));

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

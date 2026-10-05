# brothemes

[![CI](https://github.com/wlejon/brothemes/actions/workflows/ci.yml/badge.svg)](https://github.com/wlejon/brothemes/actions/workflows/ci.yml)

Standalone, reusable C++20 colour schemes and theme engine library for the
[bro](https://github.com/wlejon/bro) desktop substrate and terminal applications. Zero dependencies, its own
CMake and ctest, building cleanly on Windows (MSVC), Linux (GCC 12+), and
macOS (Apple Clang).

## Features & Scope

- **Unified Color Model**:
  - 8-bit per channel RGBA (`Color` / `Rgba8`) and float RGBA (`ColorF`).
  - Parsing and formatting for hex strings (`#rgb`, `#rgba`, `#rrggbb`, `#rrggbbaa`, `0x...`, bare hex), CSS functions (`rgb(...)`, `rgba(...)`, `hsl(...)`, `hsla(...)`), and standard named ANSI colors (`red`, `bright_blue`, `gray`, etc.).
- **Perceptual & Physical Color Spaces**:
  - Full bidirectional conversions:
    - sRGB (IEC 61966-2-1 piecewise transfer function)
    - Linear sRGB
    - CIE 1931 XYZ (D65 standard illuminant)
    - CIE $L^*a^*b^*$ (CIELAB D65)
    - Oklab (Björn Ottosson's perceptual color space)
    - Oklch (Cylindrical representation: Lightness $L$, Chroma $C$, Hue angle $h^\circ$)
    - HSL and HSV
  - In-gamut verification and hue-preserving gamut fitting for sRGB.
- **Unified Theme Schema (`bro::themes::Theme`)**:
  - Full 16-color ANSI palette (normal 0..7 and bright 8..15) with named and indexed accessors.
  - Semantic UI colors: `background`, `foreground`, `cursor`, `cursor_text`, `selection_background`, `selection_foreground`, `border`, `status_bar`, `line_number`, `active_line`, `match_highlight`, `search_match`, `tab_bar`, `split_divider`.
  - Rich syntax/token colors: `comment`, `string`, `keyword`, `number`, `function`, `type`, `variable`, `constant`, `operator_color`, `punctuation`, `error`, `warning`, `info`, `hint`, markup tokens.
  - Metadata: `name`, `author`, `description`, `is_dark`.
  - Normalization: synthesizes missing UI or syntax colors from ANSI/UI fallbacks so every slot provides consistent values.
- **Cross-Format Importers & Exporters**:
  - **iTerm2** (`.itermcolors` XML plist property list parser and serializer).
  - **Windows Terminal** (JSON scheme format and full settings `schemes` array).
  - **Alacritty** (both modern TOML `[colors.*]` and legacy YAML syntax).
  - **kitty** (`kitty.conf` key-value syntax with full color and UI options).
  - **Ghostty** (Ghostty theme syntax `palette = <i>=<color>`, cursor, selection).
  - **Base16 & Base24** (YAML and JSON mapping `base00`-`base0F` / `base17` to semantic slots and ANSI colors).
  - **VS Code** (VS Code theme JSON `colors` dictionary and `tokenColors` scopes with comment-stripping support).
  - Robust format auto-detection by file extension and content inspection.
- **Contrast Calculations & Automatic Adjustments**:
  - **WCAG 2.1**: Relative luminance and contrast ratio ($CR = \frac{L_1 + 0.05}{L_2 + 0.05} \in [1, 21]$), compliance tiers (`Fail`, `AaLarge`, `AaNormal`, `AaaNormal`).
  - **APCA** (Accessible Perceptual Contrast Algorithm - W3C Silver / WCAG 3): Signed contrast $L_c$ calculations with dark/light mode curves and black level soft-clamping.
  - **Automatic Minimum-Contrast Adjustment**: Real-time terminal color adjustment. If contrast against the background falls below a threshold (e.g. 4.5:1), the engine adjusts luminance in Oklch space while strictly preserving perceived hue ($h$) and respecting sRGB gamut bounds.
  - Batch theme adjustment: `adjust_palette_contrast(theme, min_wcag_ratio)` adjusts all ANSI, foreground, and syntax tokens against `ui.background`.

## Layout

```
include/brothemes/
  themes.h          Main umbrella header
  color.h           Color, ColorF, hex, rgb/hsl, ANSI parsing & formatting
  color_spaces.h    LinearRgb, Xyz, Lab, Oklab, Oklch, Hsl, Hsv conversions
  theme.h           AnsiPalette, UiColors, SyntaxColors, ThemeMetadata, Theme
  contrast.h        WCAG 2.1, APCA, adjust_contrast, adjust_palette_contrast
  format.h          ThemeFormat, import_theme, export_theme, load_theme_file, save_theme_file
src/
  color.cpp         Parsing, formatting, conversion implementation
  color_spaces.cpp  Mathematical color space transforms and gamut mapping
  theme.cpp         Theme normalization and helpers
  contrast.cpp      WCAG 2.1, APCA reference calculations, Oklch contrast solver
  format.cpp        Format dispatcher and auto-detection
  formats/
    json_util.*     Lightweight JSON parser/serializer with comment tolerance
    xml_plist.*     Apple Property List XML parser & serializer
    kv_util.*       Key-value line parser
    toml_yaml_util.* Dotted configuration map parser for TOML & YAML
    iterm.cpp       iTerm2 importer & exporter
    windows_terminal.cpp Windows Terminal importer & exporter
    alacritty.cpp   Alacritty TOML and YAML importer & exporter
    kitty.cpp       Kitty conf importer & exporter
    ghostty.cpp     Ghostty theme importer & exporter
    base16.cpp      Base16 & Base24 YAML/JSON importer & exporter
    vscode.cpp      VS Code theme JSON importer & exporter
tests/
  check.h           Minimal test harness (fails in Release, no assert())
  test_color.cpp    Color & ColorF unit tests
  test_color_spaces.cpp Color space roundtrips and reference points
  test_contrast.cpp WCAG 2.1 & APCA reference test vectors
  test_adjustment.cpp Automatic contrast adjustment and hue preservation
  test_formats.cpp  Direct format import/export roundtrips and file load/save
  test_matrix.cpp   Cross-format matrix tests with Dracula, Solarized, Nord, Monokai, One Dark, Gruvbox, Tokyo Night, Catppuccin
  test_schemes_oracle.cpp Differential oracle over all 724 iTerm2-Color-Schemes schemes
  test_pack.cpp     The bpk archive codec the oracle data ships in
  data/iterm2-color-schemes.bpk     The iTerm2-Color-Schemes files the oracle reads (0.5 MB)
  data/iterm2-color-schemes.LICENSE Their licence (MIT)
tools/pack/
  bpk.h, bpk_*.cpp  Archive codec: LZ77 over a 1 MiB window + canonical Huffman
  pack_schemes.cpp  brothemes_pack: rebuild / list / extract the oracle archive
```

## Usage Example

```cpp
#include <brothemes/themes.h>
#include <iostream>

using namespace bro::themes;

int main() {
    // Load a theme from any supported format (auto-detected)
    auto theme = load_theme_file("dracula.json");
    if (!theme) return 1;

    // Check WCAG contrast of foreground against background
    float cr = wcag_contrast_ratio(theme->ui.foreground, theme->ui.background);
    std::cout << "Foreground contrast ratio: " << cr << ":1\n";

    // Compute APCA contrast (W3C Silver)
    float apca = apca_contrast(theme->ui.foreground, theme->ui.background);
    std::cout << "APCA Lc: " << apca << "\n";

    // Automatically boost ANSI palette to meet WCAG AA (4.5:1)
    adjust_palette_contrast(*theme, 4.5f);

    // Export theme to Ghostty or Alacritty TOML format
    std::string ghostty_cfg = export_theme(*theme, ThemeFormat::Ghostty);
    std::cout << ghostty_cfg << "\n";

    return 0;
}
```

## Building & Testing

### Windows (MSVC, Visual Studio 2022)

```bash
cmake -B build
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

### Linux (GCC 12+, Ninja)

```bash
cmake -B build-release -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-release
ctest --test-dir build-release --output-on-failure
```

### macOS (Apple Clang, Ninja)

```bash
cmake -B build-release -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-release
ctest --test-dir build-release --output-on-failure
```

## Test Philosophy

- Tests are real ctests with checks compiled into every configuration (no `assert()`).
- All checks fail in Release builds if an invariant is violated.
- Non-destructive: tests write only to temporary files in the build tree and clean them up before exit.
- Includes authentic reference test vectors for Dracula, Solarized Dark, Solarized Light, Nord, Monokai, One Dark, Gruvbox, Tokyo Night, and Catppuccin.
- Multi-format differential oracle: round-trips all 724 schemes from the public `iTerm2-Color-Schemes` repository across Windows Terminal, Kitty, Ghostty, Alacritty, and iTerm2 property list formats.
  The upstream files (2,899 of them, 6.4 MB of content, ~9.7 MB on disk) ship byte for byte in one 0.5 MB archive,
  `tests/data/iterm2-color-schemes.bpk`, written by an in-repo C++ codec (`tools/pack`, no third-party compressor).
  To move to a newer upstream: `brothemes_pack <iTerm2-Color-Schemes checkout> tests/data/iterm2-color-schemes.bpk`
  (pack an LF checkout, e.g. `git -c core.autocrlf=false clone`, so the archive holds upstream's bytes)
  (built with the tests; `--list` / `--extract` read an archive back), then update the per-format counts in
  `test_schemes_oracle.cpp`. The library itself embeds no scheme data.

## Standards & Licensing: WCAG 2.1 vs APCA

- **WCAG 2.1 (Default & Recommended Standard)**:
  - Formulated as a formal W3C Recommendation.
  - Fully open and unencumbered for all commercial and open-source software applications.
  - Used by default in `wcag_contrast_ratio`, `meets_wcag_aa`, `meets_wcag_aaa`, `adjust_contrast`, and `adjust_palette_contrast`.

- **APCA (Accessible Perceptual Contrast Algorithm - W3C Silver / WCAG 3 Candidate)**:
  - Developed by Andrew Somers / Myndex Research.
  - APCA is currently an experimental candidate algorithm for WCAG 3.
  - The name "APCA" is a registered trademark of Myndex Research, and its specific reference code is subject to Myndex licensing restrictions.
  - Provided in `brothemes` (`apca_contrast`, `apca_contrast_abs`) for evaluation and comparison against emerging perceptual models. For applications requiring unencumbered open-source licensing, WCAG 2.1 is the authoritative standard.

## License

MIT; see [LICENSE](LICENSE). The test data in `tests/data/iterm2-color-schemes.bpk` is the
[iTerm2-Color-Schemes](https://github.com/mbadolato/iTerm2-Color-Schemes) collection, also MIT
(`tests/data/iterm2-color-schemes.LICENSE`); it is read only by the tests and is not part of the
library.


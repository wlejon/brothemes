#include "check.h"
#include <brothemes/api.h>
#include <brothemes/themes.h>
#include "embed/embed.h"
#include "eval/eval.h"

#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace ev = bronze::embed;
using namespace bronze::eval;

void testMounting() {
    auto g = ev::globalValue("bro");
    CHECK(g.found);
    CHECK(ev::isObject(g.value));

    ev::Persistent themes(ev::getProperty(g.value, "themes"));
    CHECK(ev::isObject(themes.get()));

    const char* expectedMethods[] = {
        "contrast", "relativeLuminance", "wcagLevel", "meetsAa", "meetsAaa",
        "apcaContrast", "adjustContrast", "adjustPaletteContrast",
        "detectFormat", "formatName", "import", "export", "load", "save",
        "parseColor", "parseHex", "fromHexString", "toHex", "toHexString",
        "toRgb", "toRgba", "createColor", "Color",
        "srgbToLinear", "linearToSrgb", "srgbToXyz", "xyzToSrgb",
        "linearToXyz", "xyzToLinear", "srgbToLab", "labToSrgb",
        "xyzToLab", "labToXyz", "srgbToOklab", "oklabToSrgb",
        "linearToOklab", "oklabToLinear", "srgbToOklch", "oklchToSrgb",
        "oklabToOklch", "oklchToOklab", "srgbToHsl", "hslToSrgb",
        "srgbToHsv", "hsvToSrgb", "fitOklchToGamut", "isInSrgbGamut",
        "clampToSrgbGamut"
    };

    for (const char* name : expectedMethods) {
        auto prop = ev::getProperty(themes.get(), name);
        CHECK(ev::isFunction(prop));
    }
}

void testContrastEval() {
    // 1. Contrast ratios
    {
        auto r = evalScript(
            "(function() {"
            "  const cr = bro.themes.contrast('#ffffff', '#000000');"
            "  return Math.abs(cr - 21.0) < 0.01;"
            "})()"
        );
        CHECK(!r.thrown);
        CHECK(ev::isBool(r.value) && ev::toBool(r.value));
    }

    {
        auto r = evalScript(
            "(function() {"
            "  const cr = bro.themes.contrast('#000000', '#000000');"
            "  return Math.abs(cr - 1.0) < 0.01;"
            "})()"
        );
        CHECK(!r.thrown);
        CHECK(ev::isBool(r.value) && ev::toBool(r.value));
    }

    // 2. Relative luminance
    {
        auto r = evalScript(
            "(function() {"
            "  const lBlack = bro.themes.relativeLuminance('#000000');"
            "  const lWhite = bro.themes.relativeLuminance('#ffffff');"
            "  return Math.abs(lBlack) < 1e-4 && Math.abs(lWhite - 1.0) < 1e-4;"
            "})()"
        );
        CHECK(!r.thrown);
        CHECK(ev::isBool(r.value) && ev::toBool(r.value));
    }

    // 3. WCAG levels and meetsAa / meetsAaa
    {
        auto r = evalScript(
            "(function() {"
            "  const lvl1 = bro.themes.wcagLevel('#ffffff', '#000000');"
            "  const lvl2 = bro.themes.wcagLevel('#777777', '#767676');"
            "  const aa = bro.themes.meetsAa('#ffffff', '#000000');"
            "  const aaa = bro.themes.meetsAaa('#ffffff', '#000000');"
            "  return lvl1 === 'aaa' && lvl2 === 'fail' && aa === true && aaa === true;"
            "})()"
        );
        CHECK(!r.thrown);
        CHECK(ev::isBool(r.value) && ev::toBool(r.value));
    }

    // 4. APCA contrast
    {
        auto r = evalScript(
            "(function() {"
            "  const darkOnLight = bro.themes.contrast('#000000', '#ffffff', { method: 'apca' });"
            "  const lightOnDark = bro.themes.contrast('#ffffff', '#000000', { method: 'apca' });"
            "  const absVal = bro.themes.apcaContrast('#ffffff', '#000000', { absolute: true });"
            "  return (darkOnLight > 0) && (lightOnDark < 0) && (Math.abs(absVal + lightOnDark) < 1e-3);"
            "})()"
        );
        CHECK(!r.thrown);
        CHECK(ev::isBool(r.value) && ev::toBool(r.value));
    }

    // 5. Adjust contrast
    {
        auto r = evalScript(
            "(function() {"
            "  const orig = '#555555';"
            "  const bg = '#444444';"
            "  const beforeCr = bro.themes.contrast(orig, bg);"
            "  const adjustedHex = bro.themes.adjustContrast(orig, bg, { minRatio: 4.5 });"
            "  const afterCr = bro.themes.contrast(adjustedHex, bg);"
            "  return (beforeCr < 4.5) && (afterCr >= 4.5) && (typeof adjustedHex === 'string');"
            "})()"
        );
        CHECK(!r.thrown);
        CHECK(ev::isBool(r.value) && ev::toBool(r.value));
    }

    // 6. Adjust contrast returning object
    {
        auto r = evalScript(
            "(function() {"
            "  const fg = { r: 85, g: 85, b: 85 };"
            "  const bg = { r: 68, g: 68, b: 68 };"
            "  const adj = bro.themes.adjustContrast(fg, bg, { minRatio: 4.5 });"
            "  return typeof adj === 'object' && adj.r !== undefined && adj.g !== undefined && adj.b !== undefined;"
            "})()"
        );
        CHECK(!r.thrown);
        CHECK(ev::isBool(r.value) && ev::toBool(r.value));
    }
}

void testColorUtilsEval() {
    // 1. parseColor & parseHex
    {
        auto r = evalScript(
            "(function() {"
            "  const c1 = bro.themes.parseColor('#ff55aa');"
            "  const c2 = bro.themes.parseHex('00ff00');"
            "  const c3 = bro.themes.fromHexString('#123456');"
            "  const c4 = bro.themes.parseColor('rgb(10, 20, 30)');"
            "  return (c1.r === 255 && c1.g === 85 && c1.b === 170 && c1.a === 255) &&"
            "         (c2.r === 0 && c2.g === 255 && c2.b === 0) &&"
            "         (c3.r === 0x12 && c3.g === 0x34 && c3.b === 0x56) &&"
            "         (c4.r === 10 && c4.g === 20 && c4.b === 30);"
            "})()"
        );
        CHECK(!r.thrown);
        CHECK(ev::isBool(r.value) && ev::toBool(r.value));
    }

    // 2. toHex, toRgb, toRgba
    {
        auto r = evalScript(
            "(function() {"
            "  const hex1 = bro.themes.toHex({ r: 255, g: 0, b: 128 });"
            "  const hexUpper = bro.themes.toHex({ r: 255, g: 0, b: 128 }, { format: 'upper-rgb' });"
            "  const rgbStr = bro.themes.toRgb('#ff8000');"
            "  const rgbaStr = bro.themes.toRgba({ r: 10, g: 20, b: 30, a: 255 });"
            "  return (hex1 === '#ff0080') && (hexUpper === '#FF0080') &&"
            "         (rgbStr === 'rgb(255, 128, 0)') && (rgbaStr === 'rgba(10, 20, 30, 1)');"
            "})()"
        );
        CHECK(!r.thrown);
        CHECK(ev::isBool(r.value) && ev::toBool(r.value));
    }

    // 3. createColor & Color constructor
    {
        auto r = evalScript(
            "(function() {"
            "  const c1 = bro.themes.createColor(100, 150, 200, 255);"
            "  const c2 = bro.themes.Color(50, 60, 70);"
            "  return (c1.r === 100 && c1.g === 150 && c1.b === 200 && c1.a === 255) &&"
            "         (c2.r === 50 && c2.g === 60 && c2.b === 70 && c2.a === 255);"
            "})()"
        );
        CHECK(!r.thrown);
        CHECK(ev::isBool(r.value) && ev::toBool(r.value));
    }

    // 4. Color space transforms
    {
        auto r = evalScript(
            "(function() {"
            "  const lin = bro.themes.srgbToLinear('#ffffff');"
            "  const srgbBack = bro.themes.linearToSrgb(lin, { colorFormat: 'hex' });"
            "  const oklab = bro.themes.srgbToOklab('#ff0000');"
            "  const oklabBack = bro.themes.oklabToSrgb(oklab);"
            "  const oklch = bro.themes.srgbToOklch('#00ff00');"
            "  const oklchBack = bro.themes.oklchToSrgb(oklch);"
            "  const hsl = bro.themes.srgbToHsl('#0000ff');"
            "  const hslBack = bro.themes.hslToSrgb(hsl);"
            "  const inGamut = bro.themes.isInSrgbGamut({ r: 0.5, g: 0.5, b: 0.5 });"
            "  return (Math.abs(lin.r - 1.0) < 1e-3) && (srgbBack === '#ffffff') &&"
            "         (Math.abs(oklabBack.r - 255) <= 1) && (Math.abs(oklchBack.g - 255) <= 1) &&"
            "         (Math.abs(hslBack.b - 255) <= 1) && inGamut;"
            "})()"
        );
        CHECK(!r.thrown);
        CHECK(ev::isBool(r.value) && ev::toBool(r.value));
    }
}

void testThemeFormatsEval() {
    // 1. detectFormat
    {
        auto r = evalScript(
            "(function() {"
            "  const plist = '<?xml version=\"1.0\"?><plist version=\"1.0\"><dict></dict></plist>';"
            "  const fmt1 = bro.themes.detectFormat(plist);"
            "  const wtJson = '{\\n  \"schemes\": [],\\n  \"cursorColor\": \"#ffffff\"\\n}';"
            "  const fmt2 = bro.themes.detectFormat(wtJson);"
            "  const kitty = 'foreground #ffffff\\nbackground #000000\\ncolor0 #000000';"
            "  const fmt3 = bro.themes.detectFormat(kitty);"
            "  const ghostty = 'palette = 0=#000000\\ncursor-color = #ffffff';"
            "  const fmt4 = bro.themes.detectFormat(ghostty);"
            "  return fmt1 === 'iterm' && fmt2 === 'windows-terminal' && fmt3 === 'kitty' && fmt4 === 'ghostty';"
            "})()"
        );
        CHECK(!r.thrown);
        CHECK(ev::isBool(r.value) && ev::toBool(r.value));
    }

    // 2. formatName
    {
        auto r = evalScript(
            "(function() {"
            "  return bro.themes.formatName('iterm') === 'iTerm2' &&"
            "         bro.themes.formatName('windows-terminal') === 'Windows Terminal' &&"
            "         bro.themes.formatName('kitty') === 'Kitty';"
            "})()"
        );
        CHECK(!r.thrown);
        CHECK(ev::isBool(r.value) && ev::toBool(r.value));
    }

    // 3. import & export Windows Terminal theme
    {
        auto r = evalScript(
            "(function() {"
            "  const wt = JSON.stringify({"
            "    name: 'SampleWT',"
            "    background: '#1e1e1e',"
            "    foreground: '#cccccc',"
            "    cursorColor: '#ffffff',"
            "    black: '#000000',"
            "    red: '#cd3131',"
            "    green: '#0dbc79',"
            "    yellow: '#e5e510',"
            "    blue: '#2472c8',"
            "    purple: '#bc3fbc',"
            "    cyan: '#11a8cd',"
            "    white: '#e5e5e5',"
            "    brightBlack: '#666666',"
            "    brightRed: '#f14c4c',"
            "    brightGreen: '#23d18b',"
            "    brightYellow: '#f5f543',"
            "    brightBlue: '#3b8eea',"
            "    brightPurple: '#d670d6',"
            "    brightCyan: '#29b8db',"
            "    brightWhite: '#e5e5e5'"
            "  });"
            "  const theme = bro.themes.import(wt, { format: 'windows-terminal' });"
            "  if (theme.name !== 'SampleWT') return false;"
            "  if (theme.ui.background !== '#1e1e1e') return false;"
            "  if (theme.ui.foreground !== '#cccccc') return false;"
            "  if (!Array.isArray(theme.ansi) || theme.ansi.length !== 16) return false;"
            "  if (theme.ansi[0] !== '#000000' || theme.ansi[1] !== '#cd3131') return false;"
            "  const exported = bro.themes.export(theme, { format: 'windows-terminal' });"
            "  return exported.includes('\"name\": \"SampleWT\"') && exported.includes('\"background\": \"#1e1e1e\"');"
            "})()"
        );
        CHECK(!r.thrown);
        CHECK(ev::isBool(r.value) && ev::toBool(r.value));
    }

    // 4. adjustPaletteContrast on a theme
    {
        auto r = evalScript(
            "(function() {"
            "  const theme = {"
            "    name: 'LowContrast',"
            "    ui: {"
            "      background: '#202020',"
            "      foreground: '#252525'"
            "    },"
            "    ansi: ["
            "      '#212121', '#222222', '#232323', '#242424',"
            "      '#252525', '#262626', '#272727', '#282828',"
            "      '#292929', '#2a2a2a', '#2b2b2b', '#2c2c2c',"
            "      '#2d2d2d', '#2e2e2e', '#2f2f2f', '#303030'"
            "    ]"
            "  };"
            "  const adjusted = bro.themes.adjustPaletteContrast(theme, { minRatio: 4.5 });"
            "  const fgCr = bro.themes.contrast(adjusted.ui.foreground, adjusted.ui.background);"
            "  const a0Cr = bro.themes.contrast(adjusted.ansi[0], adjusted.ui.background);"
            "  return (fgCr >= 4.5) && (a0Cr >= 4.5);"
            "})()"
        );
        CHECK(!r.thrown);
        CHECK(ev::isBool(r.value) && ev::toBool(r.value));
    }
}

void testFileLoadSave() {
    std::string testPath = std::string(BROTHEMES_TEST_DIR) + "/temp_test_theme.json";

    // Set custom path resolver
    bro::themes::api::setPathResolver([](const std::string& path) {
        return path;
    });

    auto r = evalScript(
        "(function(path) {"
        "  const theme = {"
        "    name: 'FileRoundtrip',"
        "    ui: {"
        "      background: '#123456',"
        "      foreground: '#abcdef'"
        "    },"
        "    ansi: ["
        "      '#000000', '#111111', '#222222', '#333333',"
        "      '#444444', '#555555', '#666666', '#777777',"
        "      '#888888', '#999999', '#aaaaaa', '#bbbbbb',"
        "      '#cccccc', '#dddddd', '#eeeeee', '#ffffff'"
        "    ]"
        "  };"
        "  const saved = bro.themes.save(theme, path, { format: 'windows-terminal' });"
        "  if (!saved) return false;"
        "  const loaded = bro.themes.load(path, { format: 'windows-terminal' });"
        "  return loaded.name === 'FileRoundtrip' &&"
        "         loaded.ui.background === '#123456' &&"
        "         loaded.ui.foreground === '#abcdef' &&"
        "         loaded.ansi[0] === '#000000' &&"
        "         loaded.ansi[15] === '#ffffff';"
        "})('" + testPath + "')"
    );

    // Clean up temp file
    std::filesystem::remove(testPath);

    CHECK(!r.thrown);
    CHECK(ev::isBool(r.value) && ev::toBool(r.value));
}

void testGcStress() {
    std::cout << "Running GC stress tests..." << std::endl;
    // Perform intensive theme operations allocating thousands of objects and strings
    auto r = evalScript(
        "(function() {"
        "  for (let iter = 0; iter < 50; ++iter) {"
        "    const hex = bro.themes.toHex({ r: (iter * 5) % 256, g: (iter * 7) % 256, b: (iter * 11) % 256 });"
        "    const oklch = bro.themes.srgbToOklch(hex);"
        "    const srgb = bro.themes.oklchToSrgb(oklch);"
        "    const cr = bro.themes.contrast(hex, '#000000');"
        "    const adj = bro.themes.adjustContrast(hex, '#000000', { minRatio: 4.5 });"
        "    const theme = {"
        "      name: 'StressTheme_' + iter,"
        "      ui: { background: hex, foreground: adj },"
        "      ansi: new Array(16).fill(hex)"
        "    };"
        "    const exported = bro.themes.export(theme, { format: 'windows-terminal' });"
        "    const imported = bro.themes.import(exported, { format: 'windows-terminal' });"
        "    if (imported.name !== 'StressTheme_' + iter) return false;"
        "  }"
        "  return true;"
        "})()"
    );
    CHECK(!r.thrown);
    CHECK(ev::isBool(r.value) && ev::toBool(r.value));
    std::cout << "  GC stress test passed!" << std::endl;
}

int main() {
    std::cout << "Installing bro.themes into Bronze realm..." << std::endl;
    bro::themes::api::installThemes();

    std::cout << "Running testMounting..." << std::endl;
    testMounting();

    std::cout << "Running testContrastEval..." << std::endl;
    testContrastEval();

    std::cout << "Running testColorUtilsEval..." << std::endl;
    testColorUtilsEval();

    std::cout << "Running testThemeFormatsEval..." << std::endl;
    testThemeFormatsEval();

    std::cout << "Running testFileLoadSave..." << std::endl;
    testFileLoadSave();

    testGcStress();

    return brotest::finish("test_themes_api");
}

#include "check.h"
#include <brothemes/themes.h>

using namespace bro::themes;

int main() {
    Theme original;
    original.metadata.name = "TestTheme";
    original.metadata.author = "TestAuthor";
    original.ui.background = Color(40, 42, 54);
    original.ui.foreground = Color(248, 248, 242);
    original.ui.cursor = Color(248, 248, 242);
    original.ui.cursor_text = Color(40, 42, 54);
    original.ui.selection_background = Color(68, 71, 90);
    original.ui.selection_foreground = Color(248, 248, 242);

    for (size_t i = 0; i < 16; ++i) {
        original.ansi[i] = Color(
            static_cast<uint8_t>(20 + i * 14),
            static_cast<uint8_t>(40 + i * 12),
            static_cast<uint8_t>(60 + i * 10)
        );
    }

    // 1. iTerm2 format round-trip
    {
        std::string iterm_xml = export_theme(original, ThemeFormat::Iterm);
        CHECK(iterm_xml.find("<plist") != std::string::npos);
        CHECK(iterm_xml.find("Ansi 0 Color") != std::string::npos);

        auto imported = import_theme(iterm_xml, ThemeFormat::Iterm);
        CHECK(imported.has_value());
        CHECK_NEAR(imported->ui.background.r, original.ui.background.r, 1);
        CHECK_NEAR(imported->ui.background.g, original.ui.background.g, 1);
        CHECK_NEAR(imported->ui.background.b, original.ui.background.b, 1);
        for (size_t i = 0; i < 16; ++i) {
            CHECK_NEAR(imported->ansi[i].r, original.ansi[i].r, 1);
            CHECK_NEAR(imported->ansi[i].g, original.ansi[i].g, 1);
            CHECK_NEAR(imported->ansi[i].b, original.ansi[i].b, 1);
        }
    }

    // 2. Windows Terminal format round-trip
    {
        std::string wt_json = export_theme(original, ThemeFormat::WindowsTerminal);
        CHECK(wt_json.find("\"cursorColor\"") != std::string::npos);
        CHECK(wt_json.find("\"brightBlack\"") != std::string::npos);

        auto imported = import_theme(wt_json, ThemeFormat::WindowsTerminal);
        CHECK(imported.has_value());
        CHECK_EQ(imported->ui.background, original.ui.background);
        CHECK_EQ(imported->ui.foreground, original.ui.foreground);
        for (size_t i = 0; i < 16; ++i) {
            CHECK_EQ(imported->ansi[i], original.ansi[i]);
        }
    }

    // 3. Alacritty TOML format round-trip
    {
        std::string toml_text = export_theme(original, ThemeFormat::AlacrittyToml);
        CHECK(toml_text.find("[colors.primary]") != std::string::npos);
        CHECK(toml_text.find("[colors.normal]") != std::string::npos);

        auto imported = import_theme(toml_text, ThemeFormat::AlacrittyToml);
        CHECK(imported.has_value());
        CHECK_EQ(imported->ui.background, original.ui.background);
        CHECK_EQ(imported->ui.foreground, original.ui.foreground);
        for (size_t i = 0; i < 16; ++i) {
            CHECK_EQ(imported->ansi[i], original.ansi[i]);
        }
    }

    // 4. Alacritty YAML format round-trip
    {
        std::string yaml_text = export_theme(original, ThemeFormat::AlacrittyYaml);
        CHECK(yaml_text.find("colors:") != std::string::npos);
        CHECK(yaml_text.find("primary:") != std::string::npos);

        auto imported = import_theme(yaml_text, ThemeFormat::AlacrittyYaml);
        CHECK(imported.has_value());
        CHECK_EQ(imported->ui.background, original.ui.background);
        CHECK_EQ(imported->ui.foreground, original.ui.foreground);
        for (size_t i = 0; i < 16; ++i) {
            CHECK_EQ(imported->ansi[i], original.ansi[i]);
        }
    }

    // 5. Kitty format round-trip
    {
        std::string kitty_conf = export_theme(original, ThemeFormat::Kitty);
        CHECK(kitty_conf.find("background ") != std::string::npos);
        CHECK(kitty_conf.find("color0 ") != std::string::npos);

        auto imported = import_theme(kitty_conf, ThemeFormat::Kitty);
        CHECK(imported.has_value());
        CHECK_EQ(imported->ui.background, original.ui.background);
        CHECK_EQ(imported->ui.foreground, original.ui.foreground);
        for (size_t i = 0; i < 16; ++i) {
            CHECK_EQ(imported->ansi[i], original.ansi[i]);
        }
    }

    // 6. Ghostty format round-trip
    {
        std::string ghostty_conf = export_theme(original, ThemeFormat::Ghostty);
        CHECK(ghostty_conf.find("palette = 0=") != std::string::npos);
        CHECK(ghostty_conf.find("background = ") != std::string::npos);

        auto imported = import_theme(ghostty_conf, ThemeFormat::Ghostty);
        CHECK(imported.has_value());
        CHECK_EQ(imported->ui.background, original.ui.background);
        CHECK_EQ(imported->ui.foreground, original.ui.foreground);
        for (size_t i = 0; i < 16; ++i) {
            CHECK_EQ(imported->ansi[i], original.ansi[i]);
        }
    }

    // 7. Base16 and Base24 format round-trip
    {
        std::string b16_yaml = export_theme(original, ThemeFormat::Base16Yaml);
        CHECK(b16_yaml.find("base00:") != std::string::npos);

        auto imp_b16_y = import_theme(b16_yaml, ThemeFormat::Base16Yaml);
        CHECK(imp_b16_y.has_value());
        CHECK_EQ(imp_b16_y->ui.background, original.ui.background);

        std::string b16_json = export_theme(original, ThemeFormat::Base16Json);
        CHECK(b16_json.find("\"base00\"") != std::string::npos);

        auto imp_b16_j = import_theme(b16_json, ThemeFormat::Base16Json);
        CHECK(imp_b16_j.has_value());
        CHECK_EQ(imp_b16_j->ui.background, original.ui.background);

        std::string b24_yaml = export_theme(original, ThemeFormat::Base24Yaml);
        CHECK(b24_yaml.find("base12:") != std::string::npos);

        std::string b24_json = export_theme(original, ThemeFormat::Base24Json);
        CHECK(b24_json.find("\"base12\"") != std::string::npos);
    }

    // 8. VS Code format round-trip
    {
        std::string vsc_json = export_theme(original, ThemeFormat::VsCode);
        CHECK(vsc_json.find("\"editor.background\"") != std::string::npos);
        CHECK(vsc_json.find("\"terminal.ansiRed\"") != std::string::npos);

        auto imported = import_theme(vsc_json, ThemeFormat::VsCode);
        CHECK(imported.has_value());
        CHECK_EQ(imported->ui.background, original.ui.background);
        CHECK_EQ(imported->ui.foreground, original.ui.foreground);
        for (size_t i = 0; i < 16; ++i) {
            CHECK_EQ(imported->ansi[i], original.ansi[i]);
        }
    }

    // 9. Auto-detection test
    {
        CHECK_EQ(static_cast<int>(detect_format("<plist><dict></dict></plist>")), static_cast<int>(ThemeFormat::Iterm));
        CHECK_EQ(static_cast<int>(detect_format("palette = 0=#112233")), static_cast<int>(ThemeFormat::Ghostty));
        CHECK_EQ(static_cast<int>(detect_format("background #112233\ncolor0 #112233")), static_cast<int>(ThemeFormat::Kitty));
        CHECK_EQ(static_cast<int>(detect_format("[colors.primary]\nbackground = \"#112233\"")), static_cast<int>(ThemeFormat::AlacrittyToml));
        CHECK_EQ(static_cast<int>(detect_format("{\"schemes\": [{\"name\": \"Test\"}]}")), static_cast<int>(ThemeFormat::WindowsTerminal));
        CHECK_EQ(static_cast<int>(detect_format("{\"tokenColors\": []}")), static_cast<int>(ThemeFormat::VsCode));
    }

    // 10. File save and load test (leaves no lasting change)
    {
        const std::string tmp_file = "test_theme_temp.json";
        CHECK(save_theme_file(original, tmp_file, ThemeFormat::WindowsTerminal));

        auto loaded = load_theme_file(tmp_file, ThemeFormat::Auto);
        CHECK(loaded.has_value());
        if (loaded.has_value()) {
            CHECK_EQ(loaded->ui.background, original.ui.background);
            CHECK_EQ(loaded->ui.foreground, original.ui.foreground);
        }

        std::remove(tmp_file.c_str());

        const std::string tmp_yaml = "test_theme_temp.yaml";
        CHECK(save_theme_file(original, tmp_yaml, ThemeFormat::Auto));
        CHECK_EQ(static_cast<int>(detect_format("", ".YAML")), static_cast<int>(ThemeFormat::AlacrittyYaml));
        CHECK_EQ(static_cast<int>(detect_format("", ".yml")), static_cast<int>(ThemeFormat::AlacrittyYaml));
        CHECK_EQ(static_cast<int>(detect_format("", ".JSON")), static_cast<int>(ThemeFormat::WindowsTerminal));
        CHECK_EQ(static_cast<int>(detect_format("", ".TOML")), static_cast<int>(ThemeFormat::AlacrittyToml));
        auto loaded_yaml = load_theme_file(tmp_yaml, ThemeFormat::Auto);
        CHECK(loaded_yaml.has_value());
        if (loaded_yaml.has_value()) {
            CHECK_EQ(loaded_yaml->ui.background, original.ui.background);
            CHECK_EQ(loaded_yaml->ui.foreground, original.ui.foreground);
        }
        std::remove(tmp_yaml.c_str());
    }

    // 11. Inline comments starting with hex chars, partial Alacritty, and empty/fallback validation
    {
        auto toml_commented = import_theme(
            "[colors.primary]\n"
            "background = \"#1e1e2e\" #catppuccin\n"
            "foreground = \"#cdd6f4\" #deadbeef\n",
            ThemeFormat::AlacrittyToml);
        CHECK(toml_commented.has_value());
        if (toml_commented.has_value()) {
            CHECK_EQ(toml_commented->ui.background, Color(0x1e, 0x1e, 0x2e));
            CHECK_EQ(toml_commented->ui.foreground, Color(0xcd, 0xd6, 0xf4));
        }

        auto yaml_commented = import_theme(
            "colors: #default\n"
            "  primary:\n"
            "    foreground: \"#cdd6f4\" #bright\n"
            "  normal:\n"
            "    green: #a6e3a1 #accent\n",
            ThemeFormat::AlacrittyYaml);
        CHECK(yaml_commented.has_value());
        if (yaml_commented.has_value()) {
            CHECK_EQ(yaml_commented->ui.foreground, Color(0xcd, 0xd6, 0xf4));
            CHECK_EQ(yaml_commented->ansi[2], Color(0xa6, 0xe3, 0xa1));
        }

        CHECK(!import_theme("{}", ThemeFormat::Auto).has_value());
        CHECK(!import_theme("{\"foo\": \"bar\"}", ThemeFormat::WindowsTerminal).has_value());
        CHECK(!import_theme("{\"colors\": {}}", ThemeFormat::VsCode).has_value());
        CHECK(!import_theme("<plist><dict></dict></plist>", ThemeFormat::Iterm).has_value());

        auto vsc_fallback = import_theme(
            "{\"colors\": {\"terminal.background\": \"#112233\"}}",
            ThemeFormat::Auto);
        CHECK(vsc_fallback.has_value());
        if (vsc_fallback.has_value()) {
            CHECK_EQ(vsc_fallback->ui.background, Color(0x11, 0x22, 0x33));
        }
    }

    return brotest::finish("test_formats");
}


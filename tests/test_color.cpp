#include "check.h"
#include <brothemes/color.h>

using namespace bro::themes;

int main() {
    // 1. Basic construction and equality
    {
        Color c1(255, 128, 64, 255);
        CHECK_EQ(c1.r, 255);
        CHECK_EQ(c1.g, 128);
        CHECK_EQ(c1.b, 64);
        CHECK_EQ(c1.a, 255);

        Color c2 = Color::from_rgb(255, 128, 64);
        CHECK(c1 == c2);

        Color c3 = Color::from_rgba(255, 128, 64, 200);
        CHECK_EQ(c3.a, 200);
        CHECK(!(c1 == c3));
    }

    // 2. Packed u32 conversion
    {
        Color c(0x12, 0x34, 0x56, 0x78);
        uint32_t rgba = c.to_u32_rgba();
        CHECK_EQ(rgba, 0x12345678u);
        Color from_rgba = Color::from_u32_rgba(rgba);
        CHECK(c == from_rgba);

        uint32_t argb = c.to_u32_argb();
        CHECK_EQ(argb, 0x78123456u);
        Color from_argb = Color::from_u32_argb(argb);
        CHECK(c == from_argb);
    }

    // 3. Float conversion
    {
        Color c(255, 0, 128, 255);
        ColorF cf = to_color_f(c);
        CHECK_NEAR(cf.r, 1.0f, 1e-4);
        CHECK_NEAR(cf.g, 0.0f, 1e-4);
        CHECK_NEAR(cf.b, 128.0f / 255.0f, 1e-4);
        CHECK_NEAR(cf.a, 1.0f, 1e-4);

        Color roundtrip = to_color(cf);
        CHECK(c == roundtrip);
    }

    // 4. Hex parsing: 3, 4, 6, 8 hex digits
    {
        auto c3 = parse_hex("#f80");
        CHECK(c3.has_value());
        CHECK_EQ(c3->r, 0xff);
        CHECK_EQ(c3->g, 0x88);
        CHECK_EQ(c3->b, 0x00);
        CHECK_EQ(c3->a, 0xff);

        auto c4 = parse_hex("#f804");
        CHECK(c4.has_value());
        CHECK_EQ(c4->r, 0xff);
        CHECK_EQ(c4->g, 0x88);
        CHECK_EQ(c4->b, 0x00);
        CHECK_EQ(c4->a, 0x44);

        auto c6 = parse_hex("#282a36");
        CHECK(c6.has_value());
        CHECK_EQ(c6->r, 0x28);
        CHECK_EQ(c6->g, 0x2a);
        CHECK_EQ(c6->b, 0x36);
        CHECK_EQ(c6->a, 0xff);

        // Without #
        auto c6_bare = parse_hex("282A36");
        CHECK(c6_bare.has_value());
        CHECK(*c6 == *c6_bare);

        // With 0x
        auto c6_0x = parse_hex("0x282a36");
        CHECK(c6_0x.has_value());
        CHECK(*c6 == *c6_0x);

        auto c8 = parse_hex("#11223344");
        CHECK(c8.has_value());
        CHECK_EQ(c8->r, 0x11);
        CHECK_EQ(c8->g, 0x22);
        CHECK_EQ(c8->b, 0x33);
        CHECK_EQ(c8->a, 0x44);

        // Invalid hex
        CHECK(!parse_hex("#12").has_value());
        CHECK(!parse_hex("#xyz").has_value());
        CHECK(!parse_hex("#12345").has_value());
    }

    // 5. Hex formatting
    {
        Color c(0x28, 0x2a, 0x36, 0xaa);
        CHECK_EQ(to_hex(c, HexFormat::LowerRgb), "#282a36");
        CHECK_EQ(to_hex(c, HexFormat::UpperRgb), "#282A36");
        CHECK_EQ(to_hex(c, HexFormat::LowerRgba), "#282a36aa");
        CHECK_EQ(to_hex(c, HexFormat::UpperRgba), "#282A36AA");

        CHECK_EQ(to_rgb_string(Color(10, 20, 30)), "rgb(10, 20, 30)");
        CHECK_EQ(to_rgba_string(Color(10, 20, 30, 255)), "rgba(10, 20, 30, 1)");
    }

    // 6. CSS rgb(), rgba(), hsl(), hsla() parsing
    {
        auto c1 = parse_color("rgb(100, 150, 200)");
        CHECK(c1.has_value());
        CHECK_EQ(c1->r, 100);
        CHECK_EQ(c1->g, 150);
        CHECK_EQ(c1->b, 200);
        CHECK_EQ(c1->a, 255);

        auto c2 = parse_color("rgba(100, 150, 200, 0.5)");
        CHECK(c2.has_value());
        CHECK_EQ(c2->r, 100);
        CHECK_EQ(c2->g, 150);
        CHECK_EQ(c2->b, 200);
        CHECK_NEAR(c2->a, 128, 1);

        auto c3 = parse_color("rgb(100%, 50%, 0%)");
        CHECK(c3.has_value());
        CHECK_EQ(c3->r, 255);
        CHECK_NEAR(c3->g, 128, 1);
        CHECK_EQ(c3->b, 0);

        auto c_hsl = parse_color("hsl(120, 100%, 50%)"); // Pure green
        CHECK(c_hsl.has_value());
        CHECK_EQ(c_hsl->r, 0);
        CHECK_EQ(c_hsl->g, 255);
        CHECK_EQ(c_hsl->b, 0);
    }

    // 7. Named ANSI colors
    {
        auto red = parse_color("red");
        CHECK(red.has_value());
        CHECK(red->r > red->g && red->r > red->b);

        auto bright_blue = parse_color("bright_blue");
        CHECK(bright_blue.has_value());
        CHECK(bright_blue->b > bright_blue->r);

        auto gray = parse_color("gray");
        CHECK(gray.has_value());
        CHECK_EQ(gray->r, gray->g);
        CHECK_EQ(gray->g, gray->b);
    }

    return brotest::finish("test_color");
}

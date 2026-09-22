#include "TestUtils.h"

#include <Urho3D/Core/StringUtils.h>
#include <Urho3D/Math/Color.h>

using namespace Urho3D;

TEST_CASE("Color construction defaults to opaque white", "[Math]")
{
    const Color c;
    REQUIRE(c == Color(1.0f, 1.0f, 1.0f, 1.0f));
    REQUIRE(c == Color::WHITE);

    REQUIRE(Color(0.5f, 0.25f, 0.125f).a_ == 1.0f);
    REQUIRE(Color(Color::RED, 0.5f) == Color(1.0f, 0.0f, 0.0f, 0.5f));

    const float data[] = {0.1f, 0.2f, 0.3f, 0.4f};
    REQUIRE(Color(data) == Color(0.1f, 0.2f, 0.3f, 0.4f));

    REQUIRE(Color(Vector3(0.1f, 0.2f, 0.3f)) == Color(0.1f, 0.2f, 0.3f, 1.0f));
    REQUIRE(Color(Vector4(0.1f, 0.2f, 0.3f, 0.4f)) == Color(0.1f, 0.2f, 0.3f, 0.4f));

    Color assigned;
    assigned = Color::RED;
    REQUIRE(assigned == Color::RED);
    REQUIRE(assigned != Color::BLUE);
}

TEST_CASE("Color named constants", "[Math]")
{
    REQUIRE(Color::BLACK == Color(0.0f, 0.0f, 0.0f, 1.0f));
    REQUIRE(Color::RED == Color(1.0f, 0.0f, 0.0f, 1.0f));
    REQUIRE(Color::GREEN == Color(0.0f, 1.0f, 0.0f, 1.0f));
    REQUIRE(Color::BLUE == Color(0.0f, 0.0f, 1.0f, 1.0f));
    REQUIRE(Color::CYAN == Color(0.0f, 1.0f, 1.0f, 1.0f));
    REQUIRE(Color::MAGENTA == Color(1.0f, 0.0f, 1.0f, 1.0f));
    REQUIRE(Color::YELLOW == Color(1.0f, 1.0f, 0.0f, 1.0f));
    REQUIRE(Color::TRANSPARENT_BLACK == Color(0.0f, 0.0f, 0.0f, 0.0f));
    REQUIRE(Color::GRAY == Color(0.5f, 0.5f, 0.5f, 1.0f));
}

TEST_CASE("Color arithmetic", "[Math]")
{
    const Color a(0.1f, 0.2f, 0.3f, 0.4f);
    const Color b(0.5f, 0.5f, 0.5f, 0.5f);

    REQUIRE((a + b).Equals(Color(0.6f, 0.7f, 0.8f, 0.9f)));
    REQUIRE((b - a).Equals(Color(0.4f, 0.3f, 0.2f, 0.1f)));
    REQUIRE(-a == Color(-0.1f, -0.2f, -0.3f, -0.4f));
    REQUIRE((a * 2.0f).Equals(Color(0.2f, 0.4f, 0.6f, 0.8f)));
    REQUIRE((2.0f * a).Equals(Color(0.2f, 0.4f, 0.6f, 0.8f)));

    Color acc(0.1f, 0.1f, 0.1f, 0.1f);
    acc += Color(0.2f, 0.2f, 0.2f, 0.2f);
    REQUIRE(acc.Equals(Color(0.3f, 0.3f, 0.3f, 0.3f)));

    REQUIRE(Color(-0.5f, 0.5f, -1.0f, 1.0f).Abs() == Color(0.5f, 0.5f, 1.0f, 1.0f));
    REQUIRE(Color::BLACK.Lerp(Color::WHITE, 0.5f).Equals(Color(0.5f, 0.5f, 0.5f, 1.0f)));
    REQUIRE(Color::BLACK.Lerp(Color::WHITE, 0.0f) == Color::BLACK);
    REQUIRE(Color::BLACK.Lerp(Color::WHITE, 1.0f) == Color::WHITE);
}

TEST_CASE("Color packed integer round trip", "[Math]")
{
    SECTION("ABGR is the default mask")
    {
        const Color red = Color::RED;
        const unsigned packed = red.ToUInt();
        REQUIRE((packed & 0xffu) == 0xffu);
        REQUIRE(((packed >> 24u) & 0xffu) == 0xffu);

        Color decoded;
        decoded.FromUInt(packed);
        REQUIRE(decoded.Equals(red));
    }

    SECTION("ARGB places alpha in the high byte and red below it")
    {
        const unsigned argb = Color::RED.ToUIntArgb();
        REQUIRE(argb == 0xffff0000u);
        REQUIRE(Color::RED.ToUIntMask(Color::ARGB) == argb);

        Color decoded;
        decoded.FromUIntMask(argb, Color::ARGB);
        REQUIRE(decoded.Equals(Color::RED));
    }

    SECTION("explicit unsigned constructor uses the supplied mask")
    {
        REQUIRE(Color(0xffff0000u, Color::ARGB).Equals(Color::RED));
        REQUIRE(Color(Color::GREEN.ToUInt()).Equals(Color::GREEN));
    }

    SECTION("ToHash matches the packed value")
    {
        REQUIRE(Color::BLUE.ToHash() == Color::BLUE.ToUInt());
        REQUIRE(Color::RED.ToHash() != Color::BLUE.ToHash());
    }
}

TEST_CASE("Color HSL round trip", "[Math]")
{
    const Color source(0.2f, 0.6f, 0.4f, 0.8f);
    const Vector3 hsl = source.ToHSL();

    Color decoded;
    decoded.FromHSL(hsl.x_, hsl.y_, hsl.z_, source.a_);
    REQUIRE_NEAR(decoded.r_, source.r_, 0.001f);
    REQUIRE_NEAR(decoded.g_, source.g_, 0.001f);
    REQUIRE_NEAR(decoded.b_, source.b_, 0.001f);
    REQUIRE_EQ_F(decoded.a_, source.a_);

    SECTION("pure red sits at hue zero and full saturation")
    {
        const Vector3 redHsl = Color::RED.ToHSL();
        REQUIRE_NEAR(redHsl.x_, 0.0f, 0.001f);
        REQUIRE_NEAR(redHsl.y_, 1.0f, 0.001f);
        REQUIRE_NEAR(redHsl.z_, 0.5f, 0.001f);
    }

    SECTION("greys have zero saturation")
    {
        REQUIRE_NEAR(Color::GRAY.ToHSL().y_, 0.0f, 0.001f);
        REQUIRE_NEAR(Color::GRAY.SaturationHSL(), 0.0f, 0.001f);
    }
}

TEST_CASE("Color HSV round trip", "[Math]")
{
    const Color source(0.9f, 0.3f, 0.1f, 1.0f);
    const Vector3 hsv = source.ToHSV();

    Color decoded;
    decoded.FromHSV(hsv.x_, hsv.y_, hsv.z_, source.a_);
    REQUIRE_NEAR(decoded.r_, source.r_, 0.001f);
    REQUIRE_NEAR(decoded.g_, source.g_, 0.001f);
    REQUIRE_NEAR(decoded.b_, source.b_, 0.001f);

    SECTION("value equals the largest channel")
    {
        REQUIRE_NEAR(source.Value(), 0.9f, 0.001f);
        REQUIRE_NEAR(source.ToHSV().z_, 0.9f, 0.001f);
    }

    SECTION("black has zero saturation and value")
    {
        REQUIRE_NEAR(Color::BLACK.SaturationHSV(), 0.0f, 0.001f);
        REQUIRE_NEAR(Color::BLACK.Value(), 0.0f, 0.001f);
    }
}

TEST_CASE("Color channel statistics", "[Math]")
{
    const Color c(0.2f, 0.5f, 0.8f, 1.0f);

    REQUIRE_NEAR(c.SumRGB(), 1.5f, 0.001f);
    REQUIRE_NEAR(c.Average(), 0.5f, 0.001f);
    REQUIRE_NEAR(c.MaxRGB(), 0.8f, 0.001f);
    REQUIRE_NEAR(c.MinRGB(), 0.2f, 0.001f);
    REQUIRE_NEAR(c.Range(), 0.6f, 0.001f);
    REQUIRE_NEAR(c.Chroma(), 0.6f, 0.001f);
    REQUIRE_NEAR(c.Lightness(), 0.5f, 0.001f);

    REQUIRE_NEAR(Color::RED.Luma(), 0.299f, 0.001f);
    REQUIRE_NEAR(Color::GREEN.Luma(), 0.587f, 0.001f);
    REQUIRE_NEAR(Color::BLUE.Luma(), 0.114f, 0.001f);

    REQUIRE_NEAR(Color::GRAY.Chroma(), 0.0f, 0.001f);

    SECTION("Bounds reports the channel extremes")
    {
        float min = 0.0f;
        float max = 0.0f;
        c.Bounds(&min, &max);
        REQUIRE_NEAR(min, 0.2f, 0.001f);
        REQUIRE_NEAR(max, 0.8f, 0.001f);
    }

    SECTION("clipped Bounds constrains to the unit range")
    {
        float min = 0.0f;
        float max = 0.0f;
        Color(-1.0f, 0.5f, 2.0f).Bounds(&min, &max, true);
        REQUIRE_NEAR(min, 0.0f, 0.001f);
        REQUIRE_NEAR(max, 1.0f, 0.001f);
    }
}

TEST_CASE("Color Hue", "[Math]")
{
    REQUIRE_NEAR(Color::RED.Hue(), 0.0f, 0.001f);
    REQUIRE_NEAR(Color::GREEN.Hue(), 1.0f / 3.0f, 0.001f);
    REQUIRE_NEAR(Color::BLUE.Hue(), 2.0f / 3.0f, 0.001f);
    REQUIRE_NEAR(Color::GRAY.Hue(), 0.0f, 0.001f);
}

TEST_CASE("Color Clip and Invert", "[Math]")
{
    Color outOfRange(-0.5f, 0.5f, 1.5f, 2.0f);
    outOfRange.Clip();
    REQUIRE_NEAR(outOfRange.r_, 0.0f, 0.001f);
    REQUIRE_NEAR(outOfRange.g_, 0.5f, 0.001f);
    REQUIRE_NEAR(outOfRange.b_, 1.0f, 0.001f);
    REQUIRE_EQ_F(outOfRange.a_, 2.0f);

    Color clipAlpha(0.0f, 0.0f, 0.0f, 2.0f);
    clipAlpha.Clip(true);
    REQUIRE_NEAR(clipAlpha.a_, 1.0f, 0.001f);

    Color toInvert(0.25f, 0.5f, 0.75f, 0.4f);
    toInvert.Invert();
    REQUIRE_NEAR(toInvert.r_, 0.75f, 0.001f);
    REQUIRE_NEAR(toInvert.g_, 0.5f, 0.001f);
    REQUIRE_NEAR(toInvert.b_, 0.25f, 0.001f);
    REQUIRE_EQ_F(toInvert.a_, 0.4f);

    Color invertAlpha(0.0f, 0.0f, 0.0f, 0.4f);
    invertAlpha.Invert(true);
    REQUIRE_NEAR(invertAlpha.a_, 0.6f, 0.001f);
}

TEST_CASE("Color gamma and linear conversion", "[Math]")
{
    REQUIRE_NEAR(Color::ConvertGammaToLinear(0.0f), 0.0f, 0.001f);
    REQUIRE_NEAR(Color::ConvertGammaToLinear(1.0f), 1.0f, 0.001f);
    REQUIRE_NEAR(Color::ConvertLinearToGamma(0.0f), 0.0f, 0.001f);
    REQUIRE_NEAR(Color::ConvertLinearToGamma(1.0f), 1.0f, 0.001f);

    SECTION("the low end uses the linear segment")
    {
        REQUIRE_NEAR(Color::ConvertGammaToLinear(0.04f), 0.04f / 12.92f, 0.0001f);
        REQUIRE_NEAR(Color::ConvertLinearToGamma(0.003f), 12.92f * 0.003f, 0.0001f);
    }

    SECTION("negative input clamps to zero on the linear to gamma path")
    {
        REQUIRE_NEAR(Color::ConvertLinearToGamma(-1.0f), 0.0f, 0.001f);
    }

    SECTION("the pair round trips across the mid range")
    {
        for (float value = 0.05f; value < 1.0f; value += 0.1f)
            REQUIRE_NEAR(Color::ConvertLinearToGamma(Color::ConvertGammaToLinear(value)), value, 0.002f);
    }

    SECTION("alpha is preserved by the whole colour conversions")
    {
        const Color c(0.5f, 0.5f, 0.5f, 0.25f);
        REQUIRE_EQ_F(c.GammaToLinear().a_, 0.25f);
        REQUIRE_EQ_F(c.LinearToGamma().a_, 0.25f);
        REQUIRE(c.GammaToLinear().r_ < c.r_);
        REQUIRE(c.LinearToGamma().r_ > c.r_);
    }
}

TEST_CASE("Color conversions and string round trip", "[Math]")
{
    const Color c(0.1f, 0.2f, 0.3f, 0.4f);
    REQUIRE(c.ToVector3() == Vector3(0.1f, 0.2f, 0.3f));
    REQUIRE(c.ToVector4() == Vector4(0.1f, 0.2f, 0.3f, 0.4f));
    REQUIRE(c.Data()[0] == 0.1f);

    REQUIRE(Color::RED.ToString() == "1 0 0 1");
    REQUIRE(ToColor(Color::RED.ToString()) == Color::RED);
    REQUIRE(ToColor(c.ToString()).Equals(c));

    SECTION("a three component string leaves alpha opaque")
    {
        REQUIRE(ToColor("1 0 0") == Color::RED);
    }
}

TEST_CASE("Color HSV covers every hue sector", "[Math]")
{
    struct Case
    {
        float hue_;
        Color expected_;
    };
    const Case cases[] = {
        {0.0f / 6.0f, Color(1.0f, 0.0f, 0.0f)},
        {1.0f / 6.0f, Color(1.0f, 1.0f, 0.0f)},
        {2.0f / 6.0f, Color(0.0f, 1.0f, 0.0f)},
        {3.0f / 6.0f, Color(0.0f, 1.0f, 1.0f)},
        {4.0f / 6.0f, Color(0.0f, 0.0f, 1.0f)},
        {5.0f / 6.0f, Color(1.0f, 0.0f, 1.0f)},
    };

    for (const Case& item : cases)
    {
        Color colour;
        colour.FromHSV(item.hue_, 1.0f, 1.0f);
        REQUIRE_NEAR(colour.r_, item.expected_.r_, 0.01f);
        REQUIRE_NEAR(colour.g_, item.expected_.g_, 0.01f);
        REQUIRE_NEAR(colour.b_, item.expected_.b_, 0.01f);
    }

    SECTION("hues outside the unit range wrap around")
    {
        Color below;
        below.FromHSV(-0.5f, 1.0f, 1.0f);
        Color equivalent;
        equivalent.FromHSV(0.5f, 1.0f, 1.0f);
        REQUIRE_NEAR(below.r_, equivalent.r_, 0.01f);
        REQUIRE_NEAR(below.g_, equivalent.g_, 0.01f);
        REQUIRE_NEAR(below.b_, equivalent.b_, 0.01f);

        Color above;
        above.FromHSV(1.25f, 1.0f, 1.0f);
        Color wrapped;
        wrapped.FromHSV(0.25f, 1.0f, 1.0f);
        REQUIRE_NEAR(above.r_, wrapped.r_, 0.01f);
        REQUIRE_NEAR(above.g_, wrapped.g_, 0.01f);
    }
}

TEST_CASE("Color HSL covers the light and dark halves", "[Math]")
{
    Color dark;
    dark.FromHSL(0.0f, 1.0f, 0.25f);
    REQUIRE_NEAR(dark.r_, 0.5f, 0.01f);
    REQUIRE_NEAR(dark.g_, 0.0f, 0.01f);

    Color light;
    light.FromHSL(0.0f, 1.0f, 0.75f);
    REQUIRE_NEAR(light.r_, 1.0f, 0.01f);
    REQUIRE_NEAR(light.g_, 0.5f, 0.01f);

    SECTION("both halves round trip back through ToHSL")
    {
        const Vector3 darkHsl = dark.ToHSL();
        REQUIRE_NEAR(darkHsl.z_, 0.25f, 0.01f);

        const Vector3 lightHsl = light.ToHSL();
        REQUIRE_NEAR(lightHsl.z_, 0.75f, 0.01f);
    }
}

TEST_CASE("Color MinRGB and hue pick the right channel ordering", "[Math]")
{
    REQUIRE_NEAR(Color(0.1f, 0.5f, 0.9f).MinRGB(), 0.1f, 0.001f);
    REQUIRE_NEAR(Color(0.9f, 0.5f, 0.1f).MinRGB(), 0.1f, 0.001f);
    REQUIRE_NEAR(Color(0.5f, 0.1f, 0.9f).MinRGB(), 0.1f, 0.001f);
    REQUIRE_NEAR(Color(0.5f, 0.9f, 0.1f).MinRGB(), 0.1f, 0.001f);
    REQUIRE_NEAR(Color(0.9f, 0.1f, 0.5f).MinRGB(), 0.1f, 0.001f);
    REQUIRE_NEAR(Color(0.1f, 0.9f, 0.5f).MinRGB(), 0.1f, 0.001f);

    REQUIRE_NEAR(Color(0.1f, 0.5f, 0.9f).MaxRGB(), 0.9f, 0.001f);
    REQUIRE_NEAR(Color(0.9f, 0.5f, 0.1f).MaxRGB(), 0.9f, 0.001f);
    REQUIRE_NEAR(Color(0.5f, 0.9f, 0.1f).MaxRGB(), 0.9f, 0.001f);

    SECTION("hue is reported for every sector of the wheel")
    {
        for (int i = 0; i < 6; ++i)
        {
            Color colour;
            colour.FromHSV(static_cast<float>(i) / 6.0f, 1.0f, 1.0f);
            REQUIRE_NEAR(colour.Hue(), static_cast<float>(i) / 6.0f, 0.01f);
        }
    }

    SECTION("a fully saturated dark colour has a defined HSL saturation")
    {
        Color dark;
        dark.FromHSL(0.5f, 1.0f, 0.1f);
        REQUIRE_NEAR(dark.SaturationHSL(), 1.0f, 0.01f);
    }
}

TEST_CASE("Color HSL saturation is defined at the extremes", "[Math]")
{
    REQUIRE_NEAR(Color::BLACK.SaturationHSL(), 0.0f, 0.001f);
    REQUIRE_NEAR(Color::WHITE.SaturationHSL(), 0.0f, 0.001f);
    REQUIRE_NEAR(Color::BLACK.ToHSL().y_, 0.0f, 0.001f);
    REQUIRE_NEAR(Color::WHITE.ToHSL().y_, 0.0f, 0.001f);
}

#include "TestUtils.h"

#include <Urho3D/Math/MathDefs.h>
#include <Urho3D/Math/Random.h>

#include <vector>

using namespace Urho3D;

TEST_CASE("MathDefs comparison helpers", "[Math]")
{
    REQUIRE(Equals(1.0f, 1.0f));
    REQUIRE(Equals(0.0f, 0.0f));
    REQUIRE_FALSE(Equals(1.0f, 1.001f));
    REQUIRE(Equals(1.0, 1.0));

    REQUIRE(Min(2, 5) == 2);
    REQUIRE(Min(5, 2) == 2);
    REQUIRE(Max(2, 5) == 5);
    REQUIRE(Max(5, 2) == 5);
    REQUIRE_EQ_F(Min(-1.5f, 1.5f), -1.5f);
    REQUIRE_EQ_F(Max(-1.5f, 1.5f), 1.5f);

    REQUIRE(Abs(-5) == 5);
    REQUIRE(Abs(5) == 5);
    REQUIRE_EQ_F(Abs(-2.5f), 2.5f);

    REQUIRE_EQ_F(Sign(-3.0f), -1.0f);
    REQUIRE_EQ_F(Sign(3.0f), 1.0f);
    REQUIRE_EQ_F(Sign(0.0f), 0.0f);

    REQUIRE(Clamp(5, 0, 10) == 5);
    REQUIRE(Clamp(-5, 0, 10) == 0);
    REQUIRE(Clamp(15, 0, 10) == 10);
    REQUIRE_EQ_F(Clamp(0.5f, 0.0f, 1.0f), 0.5f);
}

TEST_CASE("MathDefs interpolation", "[Math]")
{
    REQUIRE_EQ_F(Lerp(0.0f, 10.0f, 0.0f), 0.0f);
    REQUIRE_EQ_F(Lerp(0.0f, 10.0f, 1.0f), 10.0f);
    REQUIRE_EQ_F(Lerp(0.0f, 10.0f, 0.25f), 2.5f);
    REQUIRE_EQ_F(Lerp(-10.0f, 10.0f, 0.5f), 0.0f);

    REQUIRE_EQ_F(InverseLerp(0.0f, 10.0f, 2.5f), 0.25f);
    REQUIRE_EQ_F(InverseLerp(-10.0f, 10.0f, 0.0f), 0.5f);

    SECTION("SmoothStep saturates outside the edges and is symmetric at the midpoint")
    {
        REQUIRE_EQ_F(SmoothStep(0.0f, 1.0f, -1.0f), 0.0f);
        REQUIRE_EQ_F(SmoothStep(0.0f, 1.0f, 2.0f), 1.0f);
        REQUIRE_EQ_F(SmoothStep(0.0f, 1.0f, 0.5f), 0.5f);
        REQUIRE(SmoothStep(0.0f, 1.0f, 0.25f) < 0.25f);
        REQUIRE(SmoothStep(0.0f, 1.0f, 0.75f) > 0.75f);
    }
}

TEST_CASE("MathDefs trigonometry in degrees", "[Math]")
{
    REQUIRE_NEAR(Sin(0.0f), 0.0f, M_EPSILON);
    REQUIRE_NEAR(Sin(90.0f), 1.0f, M_EPSILON);
    REQUIRE_NEAR(Cos(0.0f), 1.0f, M_EPSILON);
    REQUIRE_NEAR(Cos(180.0f), -1.0f, M_EPSILON);
    REQUIRE_NEAR(Tan(45.0f), 1.0f, M_EPSILON);

    REQUIRE_NEAR(Asin(1.0f), 90.0f, 0.001f);
    REQUIRE_NEAR(Acos(1.0f), 0.0f, 0.001f);
    REQUIRE_NEAR(Acos(-1.0f), 180.0f, 0.001f);
    REQUIRE_NEAR(Atan(1.0f), 45.0f, 0.001f);
    REQUIRE_NEAR(Atan2(1.0f, 1.0f), 45.0f, 0.001f);
    REQUIRE_NEAR(Atan2(1.0f, -1.0f), 135.0f, 0.001f);

    SECTION("Asin and Acos clamp their argument instead of returning NaN")
    {
        REQUIRE_FALSE(IsNaN(Asin(2.0f)));
        REQUIRE_FALSE(IsNaN(Acos(-2.0f)));
        REQUIRE_NEAR(Asin(5.0f), 90.0f, 0.001f);
        REQUIRE_NEAR(Acos(-5.0f), 180.0f, 0.001f);
    }

    SECTION("SinCos matches the individual functions")
    {
        float s = 0.0f;
        float c = 0.0f;
        SinCos(30.0f, s, c);
        REQUIRE_NEAR(s, Sin(30.0f), 0.0001f);
        REQUIRE_NEAR(c, Cos(30.0f), 0.0001f);
    }

    REQUIRE_NEAR(ToRadians(180.0f), M_PI, M_EPSILON);
    REQUIRE_NEAR(ToDegrees(M_PI), 180.0f, 0.001f);
}

TEST_CASE("MathDefs power and logarithm", "[Math]")
{
    REQUIRE_NEAR(Pow(2.0f, 10.0f), 1024.0f, 0.001f);
    REQUIRE_NEAR(Sqrt(16.0f), 4.0f, M_EPSILON);
    REQUIRE_NEAR(Ln(1.0f), 0.0f, M_EPSILON);
    REQUIRE(Ln(0.0f) < 0.0f);
}

TEST_CASE("MathDefs rounding", "[Math]")
{
    REQUIRE_EQ_F(Floor(1.7f), 1.0f);
    REQUIRE_EQ_F(Floor(-1.2f), -2.0f);
    REQUIRE(FloorToInt(1.7f) == 1);
    REQUIRE(FloorToInt(-1.2f) == -2);

    REQUIRE_EQ_F(Ceil(1.2f), 2.0f);
    REQUIRE_EQ_F(Ceil(-1.7f), -1.0f);
    REQUIRE(CeilToInt(1.2f) == 2);
    REQUIRE(CeilToInt(-1.7f) == -1);

    REQUIRE_EQ_F(Round(1.4f), 1.0f);
    REQUIRE_EQ_F(Round(1.6f), 2.0f);
    REQUIRE(RoundToInt(1.4f) == 1);
    REQUIRE(RoundToInt(1.6f) == 2);
    REQUIRE(RoundToInt(-1.6f) == -2);

    REQUIRE_EQ_F(Fract(1.25f), 0.25f);
    REQUIRE_EQ_F(Fract(-1.25f), 0.75f);

    SECTION("RoundToNearestMultiple rounds away at the halfway point and keeps the sign")
    {
        REQUIRE_EQ_F(RoundToNearestMultiple(7.0f, 5.0f), 5.0f);
        REQUIRE_EQ_F(RoundToNearestMultiple(8.0f, 5.0f), 10.0f);
        REQUIRE_EQ_F(RoundToNearestMultiple(7.5f, 5.0f), 10.0f);
        REQUIRE_EQ_F(RoundToNearestMultiple(-8.0f, 5.0f), -10.0f);
        REQUIRE_EQ_F(RoundToNearestMultiple(-7.0f, 5.0f), -5.0f);
    }
}

TEST_CASE("MathDefs modulo", "[Math]")
{
    REQUIRE_EQ_F(Mod(7.5f, 2.0f), 1.5f);
    REQUIRE(Mod(7, 3) == 1);
    REQUIRE(Mod(-7, 3) == -1);

    REQUIRE(AbsMod(-7, 3) == 2);
    REQUIRE(AbsMod(7, 3) == 1);
    REQUIRE_EQ_F(AbsMod(-1.5f, 4.0f), 2.5f);
}

TEST_CASE("MathDefs bit helpers", "[Math]")
{
    REQUIRE(IsPowerOfTwo(1));
    REQUIRE(IsPowerOfTwo(2));
    REQUIRE(IsPowerOfTwo(1024));
    REQUIRE_FALSE(IsPowerOfTwo(0));
    REQUIRE_FALSE(IsPowerOfTwo(3));
    REQUIRE_FALSE(IsPowerOfTwo(1000));

    REQUIRE(NextPowerOfTwo(1) == 1);
    REQUIRE(NextPowerOfTwo(3) == 4);
    REQUIRE(NextPowerOfTwo(16) == 16);
    REQUIRE(NextPowerOfTwo(17) == 32);

    REQUIRE(ClosestPowerOfTwo(17) == 16);
    REQUIRE(ClosestPowerOfTwo(25) == 32);
    REQUIRE(ClosestPowerOfTwo(16) == 16);

    REQUIRE(LogBaseTwo(1) == 0);
    REQUIRE(LogBaseTwo(8) == 3);
    REQUIRE(LogBaseTwo(1024) == 10);
    REQUIRE(LogBaseTwo(1023) == 9);

    REQUIRE(CountSetBits(0) == 0);
    REQUIRE(CountSetBits(1) == 1);
    REQUIRE(CountSetBits(0xffu) == 8);
    REQUIRE(CountSetBits(0xffffffffu) == 32);

    SECTION("SDBMHash accumulates and is order sensitive")
    {
        const unsigned ab = SDBMHash(SDBMHash(0, 'a'), 'b');
        const unsigned ba = SDBMHash(SDBMHash(0, 'b'), 'a');
        REQUIRE(ab != ba);
        REQUIRE(SDBMHash(0, 'a') != 0);
    }
}

TEST_CASE("MathDefs half float conversion", "[Math]")
{
    REQUIRE_EQ_F(HalfToFloat(FloatToHalf(0.0f)), 0.0f);
    REQUIRE_NEAR(HalfToFloat(FloatToHalf(1.0f)), 1.0f, 0.001f);
    REQUIRE_NEAR(HalfToFloat(FloatToHalf(-2.5f)), -2.5f, 0.001f);
    REQUIRE_NEAR(HalfToFloat(FloatToHalf(0.25f)), 0.25f, 0.001f);

    SECTION("sign is preserved")
    {
        REQUIRE(HalfToFloat(FloatToHalf(-1.0f)) < 0.0f);
        REQUIRE(HalfToFloat(FloatToHalf(1.0f)) > 0.0f);
    }

    SECTION("values beyond half range clamp instead of becoming infinity")
    {
        const float large = HalfToFloat(FloatToHalf(1.0e30f));
        REQUIRE_FALSE(IsInf(large));
        REQUIRE(large > 60000.0f);
    }
}

TEST_CASE("MathDefs NaN and infinity detection", "[Math]")
{
    REQUIRE_FALSE(IsNaN(1.0f));
    REQUIRE_FALSE(IsInf(1.0f));
    REQUIRE(IsInf(M_INFINITY));
    REQUIRE(IsInf(-M_INFINITY));
    REQUIRE(IsNaN(M_INFINITY - M_INFINITY));

    REQUIRE(FloatToRawIntBits(0.0f) == 0u);
    REQUIRE(FloatToRawIntBits(1.0f) == 0x3f800000u);
}

TEST_CASE("MathDefs Average", "[Math]")
{
    const std::vector<float> values{1.0f, 2.0f, 3.0f, 4.0f};
    REQUIRE_EQ_F(Average(values.begin(), values.end()), 2.5f);

    const std::vector<float> empty;
    REQUIRE_EQ_F(Average(empty.begin(), empty.end()), 0.0f);
}

TEST_CASE("Random is deterministic for a given seed", "[Math]")
{
    SetRandomSeed(12345);
    REQUIRE(GetRandomSeed() == 12345);

    std::vector<int> first;
    for (int i = 0; i < 16; ++i)
        first.push_back(Rand());

    SetRandomSeed(12345);
    for (int i = 0; i < 16; ++i)
        REQUIRE(Rand() == first[i]);

    SetRandomSeed(999);
    std::vector<int> second;
    for (int i = 0; i < 16; ++i)
        second.push_back(Rand());
    REQUIRE(first != second);
}

TEST_CASE("Random stays inside its documented ranges", "[Math]")
{
    SetRandomSeed(7);

    for (int i = 0; i < 512; ++i)
    {
        const int raw = Rand();
        REQUIRE(raw >= 0);
        REQUIRE(raw <= 32767);

        const float unit = Random();
        REQUIRE(unit >= 0.0f);
        REQUIRE(unit < 1.0f);

        const float ranged = Random(5.0f);
        REQUIRE(ranged >= 0.0f);
        REQUIRE(ranged <= 5.0f);

        const float between = Random(-3.0f, 3.0f);
        REQUIRE(between >= -3.0f);
        REQUIRE(between <= 3.0f);

        const int intRange = Random(10);
        REQUIRE(intRange >= 0);
        REQUIRE(intRange < 10);

        const int intBetween = Random(5, 15);
        REQUIRE(intBetween >= 5);
        REQUIRE(intBetween < 15);
    }
}

TEST_CASE("RandomNormal is centred on the requested mean", "[Math]")
{
    SetRandomSeed(4242);

    double sum = 0.0;
    const int samples = 20000;
    for (int i = 0; i < samples; ++i)
        sum += RandomNormal(10.0f, 1.0f);

    REQUIRE_NEAR(static_cast<float>(sum / samples), 10.0f, 0.1f);
}

#include "TestUtils.h"

#include <Urho3D/Core/StringUtils.h>
#include <Urho3D/Math/Vector2.h>

using namespace Urho3D;

TEST_CASE("IntVector2 construction and constants", "[Math]")
{
    REQUIRE(IntVector2::ZERO == IntVector2(0, 0));
    REQUIRE(IntVector2::ONE == IntVector2(1, 1));
    REQUIRE(IntVector2::LEFT == IntVector2(-1, 0));
    REQUIRE(IntVector2::RIGHT == IntVector2(1, 0));
    REQUIRE(IntVector2::UP == IntVector2(0, 1));
    REQUIRE(IntVector2::DOWN == IntVector2(0, -1));

    REQUIRE(IntVector2() == IntVector2::ZERO);

    const int intData[] = {3, 4};
    REQUIRE(IntVector2(intData) == IntVector2(3, 4));

    const float floatData[] = {3.9f, -4.9f};
    REQUIRE(IntVector2(floatData) == IntVector2(3, -4));

    IntVector2 copy(IntVector2(7, 8));
    REQUIRE(copy == IntVector2(7, 8));
    REQUIRE(copy != IntVector2(7, 9));
}

TEST_CASE("IntVector2 arithmetic", "[Math]")
{
    const IntVector2 a(6, 8);
    const IntVector2 b(2, 4);

    REQUIRE(a + b == IntVector2(8, 12));
    REQUIRE(a - b == IntVector2(4, 4));
    REQUIRE(-a == IntVector2(-6, -8));
    REQUIRE(a * 2 == IntVector2(12, 16));
    REQUIRE(2 * a == IntVector2(12, 16));
    REQUIRE(a * b == IntVector2(12, 32));
    REQUIRE(a / 2 == IntVector2(3, 4));
    REQUIRE(a / b == IntVector2(3, 2));

    IntVector2 acc(1, 1);
    acc += IntVector2(2, 3);
    REQUIRE(acc == IntVector2(3, 4));
    acc -= IntVector2(1, 1);
    REQUIRE(acc == IntVector2(2, 3));
    acc *= 3;
    REQUIRE(acc == IntVector2(6, 9));
    acc /= 3;
    REQUIRE(acc == IntVector2(2, 3));
    acc *= IntVector2(2, 2);
    REQUIRE(acc == IntVector2(4, 6));
    acc /= IntVector2(2, 2);
    REQUIRE(acc == IntVector2(2, 3));
}

TEST_CASE("IntVector2 accessors", "[Math]")
{
    const IntVector2 v(3, 4);
    REQUIRE_EQ_F(v.Length(), 5.0f);
    REQUIRE(v.Data()[0] == 3);
    REQUIRE(v.Data()[1] == 4);
    REQUIRE(v.ToString() == "3 4");
    REQUIRE(v.ToHash() != IntVector2(4, 3).ToHash());
}

TEST_CASE("Vector2 construction and constants", "[Math]")
{
    REQUIRE(Vector2::ZERO == Vector2(0.0f, 0.0f));
    REQUIRE(Vector2::ONE == Vector2(1.0f, 1.0f));
    REQUIRE(Vector2::LEFT == Vector2(-1.0f, 0.0f));
    REQUIRE(Vector2::RIGHT == Vector2(1.0f, 0.0f));
    REQUIRE(Vector2::UP == Vector2(0.0f, 1.0f));
    REQUIRE(Vector2::DOWN == Vector2(0.0f, -1.0f));

    REQUIRE(Vector2() == Vector2::ZERO);
    REQUIRE(Vector2(IntVector2(2, 3)) == Vector2(2.0f, 3.0f));

    const float data[] = {5.0f, 6.0f};
    REQUIRE(Vector2(data) == Vector2(5.0f, 6.0f));

    Vector2 assigned;
    assigned = Vector2(1.0f, 2.0f);
    REQUIRE(assigned == Vector2(1.0f, 2.0f));
    REQUIRE(assigned != Vector2(1.0f, 2.5f));
}

TEST_CASE("Vector2 arithmetic", "[Math]")
{
    const Vector2 a(1.0f, 2.0f);
    const Vector2 b(4.0f, 8.0f);

    REQUIRE(a + b == Vector2(5.0f, 10.0f));
    REQUIRE(b - a == Vector2(3.0f, 6.0f));
    REQUIRE(-a == Vector2(-1.0f, -2.0f));
    REQUIRE(a * 3.0f == Vector2(3.0f, 6.0f));
    REQUIRE(3.0f * a == Vector2(3.0f, 6.0f));
    REQUIRE(a * b == Vector2(4.0f, 16.0f));
    REQUIRE(b / a == Vector2(4.0f, 4.0f));
    REQUIRE(b / 2.0f == Vector2(2.0f, 4.0f));

    Vector2 acc(1.0f, 2.0f);
    acc += Vector2(1.0f, 1.0f);
    REQUIRE(acc == Vector2(2.0f, 3.0f));
    acc -= Vector2(1.0f, 1.0f);
    REQUIRE(acc == Vector2(1.0f, 2.0f));
    acc *= 4.0f;
    REQUIRE(acc == Vector2(4.0f, 8.0f));
    acc /= 4.0f;
    REQUIRE(acc.Equals(Vector2(1.0f, 2.0f)));
    acc *= Vector2(2.0f, 2.0f);
    REQUIRE(acc.Equals(Vector2(2.0f, 4.0f)));
    acc /= Vector2(2.0f, 2.0f);
    REQUIRE(acc.Equals(Vector2(1.0f, 2.0f)));
}

TEST_CASE("Vector2 length and normalisation", "[Math]")
{
    const Vector2 v(3.0f, 4.0f);
    REQUIRE_EQ_F(v.Length(), 5.0f);
    REQUIRE_EQ_F(v.LengthSquared(), 25.0f);

    REQUIRE(v.Normalized().Equals(Vector2(0.6f, 0.8f)));

    Vector2 inPlace(0.0f, 9.0f);
    inPlace.Normalize();
    REQUIRE(inPlace.Equals(Vector2::UP));

    SECTION("already normalised vectors are left untouched")
    {
        Vector2 unit = Vector2::RIGHT;
        unit.Normalize();
        REQUIRE(unit == Vector2::RIGHT);
    }

    SECTION("zero length degenerates instead of producing NaN")
    {
        Vector2 zero = Vector2::ZERO;
        zero.Normalize();
        REQUIRE(zero == Vector2::ZERO);
        REQUIRE(Vector2::ZERO.Normalized() == Vector2::ZERO);
    }

    SECTION("NormalizedOrDefault falls back below the epsilon")
    {
        REQUIRE(Vector2(0.0f, 5.0f).NormalizedOrDefault().Equals(Vector2::UP));
        REQUIRE(Vector2::ZERO.NormalizedOrDefault(Vector2::RIGHT) == Vector2::RIGHT);
        REQUIRE(Vector2(1.0e-9f, 0.0f).NormalizedOrDefault(Vector2::DOWN) == Vector2::DOWN);
    }

    SECTION("ReNormalized clamps the length into the requested range")
    {
        REQUIRE(Vector2(10.0f, 0.0f).ReNormalized(1.0f, 2.0f).Equals(Vector2(2.0f, 0.0f)));
        REQUIRE(Vector2(0.5f, 0.0f).ReNormalized(1.0f, 2.0f).Equals(Vector2(1.0f, 0.0f)));
        REQUIRE(Vector2(1.5f, 0.0f).ReNormalized(1.0f, 2.0f).Equals(Vector2(1.5f, 0.0f)));
        REQUIRE(Vector2::ZERO.ReNormalized(1.0f, 2.0f, Vector2::UP) == Vector2::UP);
    }
}

TEST_CASE("Vector2 products and interpolation", "[Math]")
{
    REQUIRE_EQ_F(Vector2(1.0f, 2.0f).DotProduct(Vector2(3.0f, 4.0f)), 11.0f);
    REQUIRE_EQ_F(Vector2(-1.0f, 2.0f).AbsDotProduct(Vector2(3.0f, 4.0f)), 11.0f);
    REQUIRE_EQ_F(Vector2::RIGHT.DotProduct(Vector2::UP), 0.0f);

    REQUIRE_EQ_F(Vector2(3.0f, 4.0f).ProjectOntoAxis(Vector2::UP), 4.0f);
    REQUIRE_NEAR(Vector2::RIGHT.Angle(Vector2::UP), 90.0f, 0.001f);
    REQUIRE_NEAR(Vector2::RIGHT.Angle(Vector2::RIGHT), 0.0f, 0.001f);

    REQUIRE(Vector2(-1.0f, -2.0f).Abs() == Vector2(1.0f, 2.0f));
    REQUIRE(Vector2(0.0f, 0.0f).Lerp(Vector2(10.0f, 20.0f), 0.5f) == Vector2(5.0f, 10.0f));

    REQUIRE(Vector2(1.0f, 2.0f).Equals(Vector2(1.0f, 2.0f)));
    REQUIRE_FALSE(Vector2(1.0f, 2.0f).Equals(Vector2(1.0f, 2.1f)));
    REQUIRE_FALSE(Vector2(1.0f, 2.0f).IsNaN());
    REQUIRE_FALSE(Vector2(1.0f, 2.0f).IsInf());
    REQUIRE(Vector2(M_INFINITY, 0.0f).IsInf());
}

TEST_CASE("Vector2 string round trip", "[Math]")
{
    const Vector2 v(1.5f, -2.5f);
    REQUIRE(v.ToString() == "1.5 -2.5");
    REQUIRE(ToVector2(v.ToString()) == v);
    REQUIRE(v.Data()[0] == 1.5f);
}

TEST_CASE("Vector2 free functions", "[Math]")
{
    const Vector2 a(1.0f, 8.0f);
    const Vector2 b(5.0f, 2.0f);

    REQUIRE(VectorMin(a, b) == Vector2(1.0f, 2.0f));
    REQUIRE(VectorMax(a, b) == Vector2(5.0f, 8.0f));
    REQUIRE(VectorLerp(Vector2::ZERO, Vector2(10.0f, 20.0f), Vector2(0.5f, 0.25f)) == Vector2(5.0f, 5.0f));

    REQUIRE(VectorFloor(Vector2(1.7f, -1.2f)) == Vector2(1.0f, -2.0f));
    REQUIRE(VectorCeil(Vector2(1.2f, -1.7f)) == Vector2(2.0f, -1.0f));
    REQUIRE(VectorRound(Vector2(1.6f, -1.6f)) == Vector2(2.0f, -2.0f));
    REQUIRE(VectorAbs(Vector2(-1.5f, 2.5f)) == Vector2(1.5f, 2.5f));

    REQUIRE(VectorFloorToInt(Vector2(1.7f, -1.2f)) == IntVector2(1, -2));
    REQUIRE(VectorCeilToInt(Vector2(1.2f, -1.7f)) == IntVector2(2, -1));
    REQUIRE(VectorRoundToInt(Vector2(1.6f, -1.6f)) == IntVector2(2, -2));

    REQUIRE(VectorMin(IntVector2(1, 8), IntVector2(5, 2)) == IntVector2(1, 2));
    REQUIRE(VectorMax(IntVector2(1, 8), IntVector2(5, 2)) == IntVector2(5, 8));
    REQUIRE(VectorAbs(IntVector2(-3, 4)) == IntVector2(3, 4));
}

TEST_CASE("StableRandom is deterministic and bounded", "[Math]")
{
    const float a = StableRandom(Vector2(1.0f, 2.0f));
    REQUIRE(a == StableRandom(Vector2(1.0f, 2.0f)));
    REQUIRE(a >= 0.0f);
    REQUIRE(a < 1.0f);

    REQUIRE(a != StableRandom(Vector2(2.0f, 1.0f)));

    const float scalar = StableRandom(3.0f);
    REQUIRE(scalar == StableRandom(Vector2(3.0f, 3.0f)));
    REQUIRE(scalar >= 0.0f);
    REQUIRE(scalar < 1.0f);
}

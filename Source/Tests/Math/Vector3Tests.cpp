#include "TestUtils.h"

#include <Urho3D/Math/Vector3.h>

using namespace Urho3D;

TEST_CASE("Vector3 construction and equality", "[Math]")
{
    REQUIRE(Vector3::ZERO == Vector3(0.0f, 0.0f, 0.0f));
    REQUIRE(Vector3::ONE == Vector3(1.0f, 1.0f, 1.0f));
    REQUIRE(Vector3::UP == Vector3(0.0f, 1.0f, 0.0f));
    REQUIRE(Vector3::FORWARD == Vector3(0.0f, 0.0f, 1.0f));
    REQUIRE(Vector3::RIGHT == Vector3(1.0f, 0.0f, 0.0f));

    Vector3 v(1.0f, 2.0f, 3.0f);
    REQUIRE(v.x_ == 1.0f);
    REQUIRE(v.y_ == 2.0f);
    REQUIRE(v.z_ == 3.0f);

    REQUIRE(Vector3(v) == v);
    REQUIRE(v != Vector3(1.0f, 2.0f, 3.5f));

    Vector3 fromVector2(Vector2(4.0f, 5.0f), 6.0f);
    REQUIRE(fromVector2 == Vector3(4.0f, 5.0f, 6.0f));

    const float data[] = {7.0f, 8.0f, 9.0f};
    REQUIRE(Vector3(data) == Vector3(7.0f, 8.0f, 9.0f));
}

TEST_CASE("Vector3 arithmetic", "[Math]")
{
    const Vector3 a(1.0f, 2.0f, 3.0f);
    const Vector3 b(4.0f, 5.0f, 6.0f);

    REQUIRE(a + b == Vector3(5.0f, 7.0f, 9.0f));
    REQUIRE(b - a == Vector3(3.0f, 3.0f, 3.0f));
    REQUIRE(-a == Vector3(-1.0f, -2.0f, -3.0f));
    REQUIRE(a * 2.0f == Vector3(2.0f, 4.0f, 6.0f));
    REQUIRE(2.0f * a == Vector3(2.0f, 4.0f, 6.0f));
    REQUIRE(a * b == Vector3(4.0f, 10.0f, 18.0f));
    REQUIRE(b / a == Vector3(4.0f, 2.5f, 2.0f));
    REQUIRE(b / 2.0f == Vector3(2.0f, 2.5f, 3.0f));

    Vector3 acc(1.0f, 1.0f, 1.0f);
    acc += Vector3(1.0f, 2.0f, 3.0f);
    REQUIRE(acc == Vector3(2.0f, 3.0f, 4.0f));
    acc -= Vector3(1.0f, 1.0f, 1.0f);
    REQUIRE(acc == Vector3(1.0f, 2.0f, 3.0f));
    acc *= 2.0f;
    REQUIRE(acc == Vector3(2.0f, 4.0f, 6.0f));
    acc /= 2.0f;
    REQUIRE(acc == Vector3(1.0f, 2.0f, 3.0f));
    acc *= Vector3(2.0f, 2.0f, 2.0f);
    REQUIRE(acc == Vector3(2.0f, 4.0f, 6.0f));
    acc /= Vector3(2.0f, 2.0f, 2.0f);
    REQUIRE(acc == Vector3(1.0f, 2.0f, 3.0f));
}

TEST_CASE("Vector3 length and normalisation", "[Math]")
{
    const Vector3 v(3.0f, 4.0f, 0.0f);
    REQUIRE_EQ_F(v.Length(), 5.0f);
    REQUIRE_EQ_F(v.LengthSquared(), 25.0f);

    const Vector3 n = v.Normalized();
    REQUIRE_EQ_F(n.Length(), 1.0f);
    REQUIRE_EQ_F(n.x_, 0.6f);
    REQUIRE_EQ_F(n.y_, 0.8f);

    Vector3 inPlace(0.0f, 0.0f, 7.0f);
    inPlace.Normalize();
    REQUIRE(inPlace == Vector3(0.0f, 0.0f, 1.0f));

    SECTION("zero vector normalises to itself rather than NaN")
    {
        Vector3 zero = Vector3::ZERO;
        zero.Normalize();
        REQUIRE(zero == Vector3::ZERO);
        REQUIRE(Vector3::ZERO.Normalized() == Vector3::ZERO);
    }
}

TEST_CASE("Vector3 products and projections", "[Math]")
{
    REQUIRE_EQ_F(Vector3::RIGHT.DotProduct(Vector3::UP), 0.0f);
    REQUIRE_EQ_F(Vector3(1.0f, 2.0f, 3.0f).DotProduct(Vector3(4.0f, 5.0f, 6.0f)), 32.0f);
    REQUIRE_EQ_F(Vector3(1.0f, -2.0f, 3.0f).AbsDotProduct(Vector3(4.0f, 5.0f, 6.0f)), 32.0f);

    REQUIRE(Vector3::RIGHT.CrossProduct(Vector3::UP) == Vector3::FORWARD);
    REQUIRE(Vector3::UP.CrossProduct(Vector3::RIGHT) == -Vector3::FORWARD);

    REQUIRE(Vector3(-1.0f, 2.0f, -3.0f).Abs() == Vector3(1.0f, 2.0f, 3.0f));
    REQUIRE(Vector3(1.0f, 2.0f, 3.0f).Lerp(Vector3(3.0f, 4.0f, 5.0f), 0.5f) == Vector3(2.0f, 3.0f, 4.0f));

    REQUIRE_EQ_F(Vector3::RIGHT.Angle(Vector3::UP), 90.0f);
    REQUIRE_EQ_F(Vector3::RIGHT.Angle(Vector3::RIGHT), 0.0f);

    REQUIRE_EQ_F(Vector3(2.0f, 3.0f, 4.0f).ProjectOntoAxis(Vector3::UP), 3.0f);
    REQUIRE(Vector3(1.0f, 5.0f, 1.0f).ProjectOntoPlane(Vector3::ZERO, Vector3::UP) == Vector3(1.0f, 0.0f, 1.0f));

    REQUIRE_EQ_F(Vector3(0.0f, 0.0f, 0.0f).DistanceToPoint(Vector3(0.0f, 3.0f, 4.0f)), 5.0f);

    SECTION("Orthogonalize returns a unit vector perpendicular to the axis")
    {
        const Vector3 axis = Vector3::UP;
        const Vector3 orthogonal = Vector3(2.0f, 5.0f, 0.0f).Orthogonalize(axis);
        REQUIRE_EQ_F(orthogonal.Length(), 1.0f);
        REQUIRE_EQ_F(orthogonal.DotProduct(axis), 0.0f);
        REQUIRE(orthogonal == Vector3::RIGHT);
    }

    SECTION("Orthogonalize of a vector parallel to the axis degenerates to zero")
    {
        REQUIRE(Vector3(3.0f, 0.0f, 0.0f).Orthogonalize(Vector3::RIGHT) == Vector3::ZERO);
    }
}

TEST_CASE("Vector3 normalisation fallbacks", "[Math]")
{
    SECTION("NormalizedOrDefault falls back below the epsilon")
    {
        REQUIRE(Vector3(0.0f, 5.0f, 0.0f).NormalizedOrDefault().Equals(Vector3::UP));
        REQUIRE(Vector3::ZERO.NormalizedOrDefault(Vector3::RIGHT) == Vector3::RIGHT);
        REQUIRE(Vector3(1.0e-9f, 0.0f, 0.0f).NormalizedOrDefault(Vector3::FORWARD) == Vector3::FORWARD);
    }

    SECTION("ReNormalized clamps the length into the requested range")
    {
        REQUIRE(Vector3(10.0f, 0.0f, 0.0f).ReNormalized(1.0f, 2.0f).Equals(Vector3(2.0f, 0.0f, 0.0f)));
        REQUIRE(Vector3(0.5f, 0.0f, 0.0f).ReNormalized(1.0f, 2.0f).Equals(Vector3(1.0f, 0.0f, 0.0f)));
        REQUIRE(Vector3(1.5f, 0.0f, 0.0f).ReNormalized(1.0f, 2.0f).Equals(Vector3(1.5f, 0.0f, 0.0f)));
        REQUIRE(Vector3::ZERO.ReNormalized(1.0f, 2.0f, Vector3::UP) == Vector3::UP);
    }

    SECTION("an already normalised vector is left untouched")
    {
        Vector3 unit = Vector3::RIGHT;
        unit.Normalize();
        REQUIRE(unit == Vector3::RIGHT);
    }
}

TEST_CASE("Vector3 projection helpers", "[Math]")
{
    SECTION("DistanceToPlane is signed")
    {
        REQUIRE_NEAR(Vector3(0.0f, 5.0f, 0.0f).DistanceToPlane(Vector3::ZERO, Vector3::UP), 5.0f, 0.001f);
        REQUIRE_NEAR(Vector3(0.0f, -5.0f, 0.0f).DistanceToPlane(Vector3::ZERO, Vector3::UP), -5.0f, 0.001f);
        REQUIRE_NEAR(Vector3(3.0f, 0.0f, 4.0f).DistanceToPlane(Vector3::ZERO, Vector3::UP), 0.0f, 0.001f);
    }

    SECTION("ProjectOntoLine drops a point onto the infinite line")
    {
        const Vector3 from = Vector3::ZERO;
        const Vector3 to(10.0f, 0.0f, 0.0f);
        REQUIRE(Vector3(5.0f, 5.0f, 0.0f).ProjectOntoLine(from, to).Equals(Vector3(5.0f, 0.0f, 0.0f)));
    }

    SECTION("the clamped form keeps the result inside the segment")
    {
        const Vector3 from = Vector3::ZERO;
        const Vector3 to(10.0f, 0.0f, 0.0f);

        REQUIRE(Vector3(-5.0f, 1.0f, 0.0f).ProjectOntoLine(from, to, false).x_ < 0.0f);
        REQUIRE(Vector3(-5.0f, 1.0f, 0.0f).ProjectOntoLine(from, to, true).Equals(from));
        REQUIRE(Vector3(50.0f, 1.0f, 0.0f).ProjectOntoLine(from, to, true).Equals(to));
    }
}

TEST_CASE("IntVector3", "[Math]")
{
    REQUIRE(IntVector3::ZERO == IntVector3(0, 0, 0));
    REQUIRE(IntVector3::ONE == IntVector3(1, 1, 1));
    REQUIRE(IntVector3::LEFT == IntVector3(-1, 0, 0));
    REQUIRE(IntVector3::RIGHT == IntVector3(1, 0, 0));
    REQUIRE(IntVector3::UP == IntVector3(0, 1, 0));
    REQUIRE(IntVector3::DOWN == IntVector3(0, -1, 0));
    REQUIRE(IntVector3::FORWARD == IntVector3(0, 0, 1));
    REQUIRE(IntVector3::BACK == IntVector3(0, 0, -1));

    REQUIRE(IntVector3() == IntVector3::ZERO);

    const IntVector3 a(6, 8, 10);
    const IntVector3 b(2, 4, 5);

    REQUIRE(a + b == IntVector3(8, 12, 15));
    REQUIRE(a - b == IntVector3(4, 4, 5));
    REQUIRE(-a == IntVector3(-6, -8, -10));
    REQUIRE(a * 2 == IntVector3(12, 16, 20));
    REQUIRE(a / 2 == IntVector3(3, 4, 5));
    REQUIRE(a != b);

    IntVector3 acc(1, 1, 1);
    acc += IntVector3(1, 2, 3);
    REQUIRE(acc == IntVector3(2, 3, 4));
    acc -= IntVector3(1, 1, 1);
    REQUIRE(acc == IntVector3(1, 2, 3));
    acc *= 2;
    REQUIRE(acc == IntVector3(2, 4, 6));
    acc /= 2;
    REQUIRE(acc == IntVector3(1, 2, 3));

    REQUIRE_NEAR(IntVector3(2, 3, 6).Length(), 7.0f, 0.001f);
    REQUIRE(a.Data()[0] == 6);
    REQUIRE(IntVector3(1, 2, 3).ToString() == "1 2 3");
    REQUIRE(IntVector3(1, 2, 3).ToHash() != IntVector3(3, 2, 1).ToHash());

    REQUIRE(Vector3(IntVector3(1, 2, 3)) == Vector3(1.0f, 2.0f, 3.0f));
}

TEST_CASE("Vector3 free functions", "[Math]")
{
    const Vector3 a(1.0f, 8.0f, 3.0f);
    const Vector3 b(5.0f, 2.0f, 7.0f);

    REQUIRE(VectorMin(a, b) == Vector3(1.0f, 2.0f, 3.0f));
    REQUIRE(VectorMax(a, b) == Vector3(5.0f, 8.0f, 7.0f));
    REQUIRE(VectorLerp(Vector3::ZERO, Vector3(10.0f, 10.0f, 10.0f), Vector3(0.5f, 0.25f, 0.0f))
        == Vector3(5.0f, 2.5f, 0.0f));

    REQUIRE(VectorFloor(Vector3(1.7f, -1.2f, 0.5f)) == Vector3(1.0f, -2.0f, 0.0f));
    REQUIRE(VectorCeil(Vector3(1.2f, -1.7f, 0.5f)) == Vector3(2.0f, -1.0f, 1.0f));
    REQUIRE(VectorRound(Vector3(1.6f, -1.6f, 1.4f)) == Vector3(2.0f, -2.0f, 1.0f));
    REQUIRE(VectorAbs(Vector3(-1.5f, 2.5f, -3.5f)) == Vector3(1.5f, 2.5f, 3.5f));

    REQUIRE(VectorFloorToInt(Vector3(1.7f, -1.2f, 0.5f)) == IntVector3(1, -2, 0));
    REQUIRE(VectorCeilToInt(Vector3(1.2f, -1.7f, 0.5f)) == IntVector3(2, -1, 1));
    REQUIRE(VectorRoundToInt(Vector3(1.6f, -1.6f, 1.4f)) == IntVector3(2, -2, 1));

    REQUIRE(VectorMin(IntVector3(1, 8, 3), IntVector3(5, 2, 7)) == IntVector3(1, 2, 3));
    REQUIRE(VectorMax(IntVector3(1, 8, 3), IntVector3(5, 2, 7)) == IntVector3(5, 8, 7));
    REQUIRE(VectorAbs(IntVector3(-3, 4, -5)) == IntVector3(3, 4, 5));
}

TEST_CASE("Vector3 validity and string round trip", "[Math]")
{
    REQUIRE(Vector3(1.0f, 2.0f, 3.0f).IsNaN() == false);
    REQUIRE(Vector3(1.0f, 2.0f, 3.0f).IsInf() == false);

    const Vector3 v(1.5f, -2.25f, 3.0f);
    REQUIRE(v.ToString() == "1.5 -2.25 3");
    REQUIRE(ToVector3(v.ToString()) == v);

    REQUIRE(Vector3(1.0f, 2.0f, 3.0f).Data()[1] == 2.0f);
    REQUIRE(Vector3(1.0f, 2.0f, 3.0f).ToHash() != Vector3(3.0f, 2.0f, 1.0f).ToHash());
}

TEST_CASE("Vector3 two dimensional constructors", "[Math]")
{
    REQUIRE(Vector3(Vector2(1.0f, 2.0f)) == Vector3(1.0f, 2.0f, 0.0f));
    REQUIRE(Vector3(3.0f, 4.0f) == Vector3(3.0f, 4.0f, 0.0f));
}

TEST_CASE("IntVector3 componentwise multiply and divide", "[Math]")
{
    REQUIRE(IntVector3(2, 3, 4) * IntVector3(5, 6, 7) == IntVector3(10, 18, 28));
    REQUIRE(IntVector3(10, 18, 28) / IntVector3(5, 6, 7) == IntVector3(2, 3, 4));
}

TEST_CASE("StableRandom over a Vector3 seed", "[Math]")
{
    const float value = StableRandom(Vector3(1.0f, 2.0f, 3.0f));
    REQUIRE(value == StableRandom(Vector3(1.0f, 2.0f, 3.0f)));
    REQUIRE(value >= 0.0f);
    REQUIRE(value < 1.0f);
    REQUIRE(value != StableRandom(Vector3(3.0f, 2.0f, 1.0f)));
}

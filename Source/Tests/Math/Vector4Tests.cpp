#include "TestUtils.h"

#include <Urho3D/Core/StringUtils.h>
#include <Urho3D/Math/Vector4.h>

using namespace Urho3D;

TEST_CASE("Vector4 construction and constants", "[Math]")
{
    REQUIRE(Vector4::ZERO == Vector4(0.0f, 0.0f, 0.0f, 0.0f));
    REQUIRE(Vector4::ONE == Vector4(1.0f, 1.0f, 1.0f, 1.0f));
    REQUIRE(Vector4() == Vector4::ZERO);

    REQUIRE(Vector4(Vector3(1.0f, 2.0f, 3.0f), 4.0f) == Vector4(1.0f, 2.0f, 3.0f, 4.0f));

    const float data[] = {5.0f, 6.0f, 7.0f, 8.0f};
    REQUIRE(Vector4(data) == Vector4(5.0f, 6.0f, 7.0f, 8.0f));

    Vector4 assigned;
    assigned = Vector4(1.0f, 2.0f, 3.0f, 4.0f);
    REQUIRE(assigned == Vector4(1.0f, 2.0f, 3.0f, 4.0f));
    REQUIRE(assigned != Vector4(1.0f, 2.0f, 3.0f, 5.0f));
}

TEST_CASE("Vector4 arithmetic", "[Math]")
{
    const Vector4 a(1.0f, 2.0f, 3.0f, 4.0f);
    const Vector4 b(4.0f, 8.0f, 12.0f, 16.0f);

    REQUIRE(a + b == Vector4(5.0f, 10.0f, 15.0f, 20.0f));
    REQUIRE(b - a == Vector4(3.0f, 6.0f, 9.0f, 12.0f));
    REQUIRE(-a == Vector4(-1.0f, -2.0f, -3.0f, -4.0f));
    REQUIRE(a * 2.0f == Vector4(2.0f, 4.0f, 6.0f, 8.0f));
    REQUIRE(2.0f * a == Vector4(2.0f, 4.0f, 6.0f, 8.0f));
    REQUIRE(a * b == Vector4(4.0f, 16.0f, 36.0f, 64.0f));
    REQUIRE(b / a == Vector4(4.0f, 4.0f, 4.0f, 4.0f));
    REQUIRE(b / 2.0f == Vector4(2.0f, 4.0f, 6.0f, 8.0f));

    Vector4 acc(1.0f, 2.0f, 3.0f, 4.0f);
    acc += Vector4(1.0f, 1.0f, 1.0f, 1.0f);
    REQUIRE(acc == Vector4(2.0f, 3.0f, 4.0f, 5.0f));
    acc -= Vector4(1.0f, 1.0f, 1.0f, 1.0f);
    REQUIRE(acc == Vector4(1.0f, 2.0f, 3.0f, 4.0f));
    acc *= 2.0f;
    REQUIRE(acc == Vector4(2.0f, 4.0f, 6.0f, 8.0f));
    acc /= 2.0f;
    REQUIRE(acc.Equals(Vector4(1.0f, 2.0f, 3.0f, 4.0f)));
    acc *= Vector4(2.0f, 2.0f, 2.0f, 2.0f);
    REQUIRE(acc.Equals(Vector4(2.0f, 4.0f, 6.0f, 8.0f)));
    acc /= Vector4(2.0f, 2.0f, 2.0f, 2.0f);
    REQUIRE(acc.Equals(Vector4(1.0f, 2.0f, 3.0f, 4.0f)));
}

TEST_CASE("Vector4 indexing", "[Math]")
{
    Vector4 v(1.0f, 2.0f, 3.0f, 4.0f);
    REQUIRE(v[0] == 1.0f);
    REQUIRE(v[1] == 2.0f);
    REQUIRE(v[2] == 3.0f);
    REQUIRE(v[3] == 4.0f);

    v[2] = 30.0f;
    REQUIRE(v.z_ == 30.0f);

    const Vector4 constVector(9.0f, 8.0f, 7.0f, 6.0f);
    REQUIRE(constVector[0] == 9.0f);
    REQUIRE(constVector[3] == 6.0f);
}

TEST_CASE("Vector4 products and interpolation", "[Math]")
{
    REQUIRE_EQ_F(Vector4(1.0f, 2.0f, 3.0f, 4.0f).DotProduct(Vector4(1.0f, 1.0f, 1.0f, 1.0f)), 10.0f);
    REQUIRE_EQ_F(Vector4(-1.0f, 2.0f, -3.0f, 4.0f).AbsDotProduct(Vector4(1.0f, 1.0f, 1.0f, 1.0f)), 10.0f);

    SECTION("ProjectOntoAxis ignores the w component")
    {
        REQUIRE_EQ_F(Vector4(2.0f, 3.0f, 4.0f, 99.0f).ProjectOntoAxis(Vector3::UP), 3.0f);
    }

    REQUIRE(Vector4(-1.0f, 2.0f, -3.0f, 4.0f).Abs() == Vector4(1.0f, 2.0f, 3.0f, 4.0f));
    REQUIRE(Vector4::ZERO.Lerp(Vector4(10.0f, 20.0f, 30.0f, 40.0f), 0.5f) == Vector4(5.0f, 10.0f, 15.0f, 20.0f));

    REQUIRE(Vector4(1.0f, 2.0f, 3.0f, 4.0f).Equals(Vector4(1.0f, 2.0f, 3.0f, 4.0f)));
    REQUIRE_FALSE(Vector4(1.0f, 2.0f, 3.0f, 4.0f).Equals(Vector4(1.0f, 2.0f, 3.0f, 4.5f)));

    REQUIRE_FALSE(Vector4(1.0f, 2.0f, 3.0f, 4.0f).IsNaN());
    REQUIRE_FALSE(Vector4(1.0f, 2.0f, 3.0f, 4.0f).IsInf());
    REQUIRE(Vector4(0.0f, 0.0f, 0.0f, M_INFINITY).IsInf());
}

TEST_CASE("Vector4 conversions", "[Math]")
{
    const Vector4 v(1.0f, 2.0f, 3.0f, 4.0f);
    REQUIRE(static_cast<Vector2>(v) == Vector2(1.0f, 2.0f));
    REQUIRE(static_cast<Vector3>(v) == Vector3(1.0f, 2.0f, 3.0f));

    REQUIRE(v.ToString() == "1 2 3 4");
    REQUIRE(ToVector4(v.ToString()) == v);
    REQUIRE(v.Data()[3] == 4.0f);
    REQUIRE(v.ToHash() != Vector4(4.0f, 3.0f, 2.0f, 1.0f).ToHash());
}

TEST_CASE("Vector4 free functions", "[Math]")
{
    const Vector4 a(1.0f, 8.0f, 3.0f, 6.0f);
    const Vector4 b(5.0f, 2.0f, 7.0f, 4.0f);

    REQUIRE(VectorMin(a, b) == Vector4(1.0f, 2.0f, 3.0f, 4.0f));
    REQUIRE(VectorMax(a, b) == Vector4(5.0f, 8.0f, 7.0f, 6.0f));
    REQUIRE(VectorLerp(Vector4::ZERO, Vector4(10.0f, 10.0f, 10.0f, 10.0f), Vector4(0.5f, 0.25f, 0.0f, 1.0f))
        == Vector4(5.0f, 2.5f, 0.0f, 10.0f));

    REQUIRE(VectorFloor(Vector4(1.7f, -1.2f, 0.5f, -0.5f)) == Vector4(1.0f, -2.0f, 0.0f, -1.0f));
    REQUIRE(VectorCeil(Vector4(1.2f, -1.7f, 0.5f, -0.5f)) == Vector4(2.0f, -1.0f, 1.0f, -0.0f));
    REQUIRE(VectorRound(Vector4(1.6f, -1.6f, 1.4f, -1.4f)) == Vector4(2.0f, -2.0f, 1.0f, -1.0f));
}

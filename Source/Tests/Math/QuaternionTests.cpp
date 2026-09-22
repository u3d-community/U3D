#include "TestUtils.h"

#include <Urho3D/Core/StringUtils.h>
#include <Urho3D/Math/Matrix3.h>
#include <Urho3D/Math/Quaternion.h>

using namespace Urho3D;

namespace
{

const float kTolerance = 0.001f;

bool NearlyEqual(const Quaternion& lhs, const Quaternion& rhs, float eps = kTolerance)
{
    return Abs(lhs.w_ - rhs.w_) <= eps && Abs(lhs.x_ - rhs.x_) <= eps
        && Abs(lhs.y_ - rhs.y_) <= eps && Abs(lhs.z_ - rhs.z_) <= eps;
}

bool NearlyEqual(const Vector3& lhs, const Vector3& rhs, float eps = kTolerance)
{
    return Abs(lhs.x_ - rhs.x_) <= eps && Abs(lhs.y_ - rhs.y_) <= eps && Abs(lhs.z_ - rhs.z_) <= eps;
}

bool SameRotation(const Quaternion& lhs, const Quaternion& rhs, float eps = kTolerance)
{
    return NearlyEqual(lhs, rhs, eps) || NearlyEqual(lhs, -rhs, eps);
}

}

TEST_CASE("Quaternion default construction is identity", "[Math]")
{
    const Quaternion q;
    REQUIRE(q == Quaternion::IDENTITY);
    REQUIRE(q.w_ == 1.0f);
    REQUIRE(q.x_ == 0.0f);
    REQUIRE(q.y_ == 0.0f);
    REQUIRE(q.z_ == 0.0f);

    REQUIRE(q * Vector3(1.0f, 2.0f, 3.0f) == Vector3(1.0f, 2.0f, 3.0f));
}

TEST_CASE("Quaternion component construction", "[Math]")
{
    const Quaternion q(1.0f, 2.0f, 3.0f, 4.0f);
    REQUIRE(q.w_ == 1.0f);
    REQUIRE(q.x_ == 2.0f);
    REQUIRE(q.y_ == 3.0f);
    REQUIRE(q.z_ == 4.0f);

    const Quaternion copy(q);
    REQUIRE(copy == q);

    Quaternion assigned;
    assigned = q;
    REQUIRE(assigned == q);
    REQUIRE(assigned != Quaternion::IDENTITY);

    const float data[] = {1.0f, 2.0f, 3.0f, 4.0f};
    REQUIRE(Quaternion(data) == q);
    REQUIRE(q.Data()[0] == 1.0f);
    REQUIRE(q.Data()[3] == 4.0f);
}

TEST_CASE("Quaternion from angle and axis", "[Math]")
{
    const Quaternion q(90.0f, Vector3::UP);

    REQUIRE_NEAR(q.Angle(), 90.0f, kTolerance);
    REQUIRE(NearlyEqual(q.Axis().Normalized(), Vector3::UP));

    SECTION("rotates a vector around that axis")
    {
        REQUIRE(NearlyEqual(q * Vector3::FORWARD, Vector3::RIGHT));
        REQUIRE(NearlyEqual(q * Vector3::RIGHT, -Vector3::FORWARD));
        REQUIRE(NearlyEqual(q * Vector3::UP, Vector3::UP));
    }

    SECTION("a zero angle is the identity")
    {
        REQUIRE(SameRotation(Quaternion(0.0f, Vector3::UP), Quaternion::IDENTITY));
    }

    SECTION("360 degrees returns to the same orientation")
    {
        REQUIRE(NearlyEqual(Quaternion(360.0f, Vector3::UP) * Vector3::FORWARD, Vector3::FORWARD));
    }

    SECTION("the axis is normalised internally")
    {
        REQUIRE(SameRotation(Quaternion(90.0f, Vector3(0.0f, 5.0f, 0.0f)), q));
    }

    SECTION("single argument constructor rotates around forward")
    {
        REQUIRE(SameRotation(Quaternion(45.0f), Quaternion(45.0f, Vector3::FORWARD)));
    }
}

TEST_CASE("Quaternion Euler angle round trip", "[Math]")
{
    const Quaternion q(10.0f, 20.0f, 30.0f);
    const Vector3 euler = q.EulerAngles();

    REQUIRE_NEAR(euler.x_, 10.0f, kTolerance);
    REQUIRE_NEAR(euler.y_, 20.0f, kTolerance);
    REQUIRE_NEAR(euler.z_, 30.0f, kTolerance);

    REQUIRE_NEAR(q.PitchAngle(), 10.0f, kTolerance);
    REQUIRE_NEAR(q.YawAngle(), 20.0f, kTolerance);
    REQUIRE_NEAR(q.RollAngle(), 30.0f, kTolerance);

    REQUIRE(SameRotation(Quaternion(euler), q));

    SECTION("pitch beyond the gimbal limit is clamped")
    {
        const Quaternion straightUp(90.0f, 0.0f, 0.0f);
        REQUIRE_NEAR(straightUp.PitchAngle(), 90.0f, 0.01f);
    }
}

TEST_CASE("Quaternion FromRotationTo", "[Math]")
{
    const Quaternion q(Vector3::FORWARD, Vector3::RIGHT);
    REQUIRE(NearlyEqual(q * Vector3::FORWARD, Vector3::RIGHT));

    SECTION("identical directions produce the identity")
    {
        REQUIRE(NearlyEqual(Quaternion(Vector3::UP, Vector3::UP) * Vector3::UP, Vector3::UP));
    }

    SECTION("opposite directions still produce a valid 180 degree rotation")
    {
        const Quaternion flip(Vector3::FORWARD, -Vector3::FORWARD);
        REQUIRE_FALSE(flip.IsNaN());
        REQUIRE(NearlyEqual(flip * Vector3::FORWARD, -Vector3::FORWARD, 0.01f));
    }

    SECTION("magnitudes are ignored")
    {
        REQUIRE(SameRotation(Quaternion(Vector3::FORWARD * 5.0f, Vector3::RIGHT * 3.0f), q));
    }
}

TEST_CASE("Quaternion FromAxes and rotation matrix round trip", "[Math]")
{
    const Quaternion source(30.0f, 45.0f, 60.0f);
    const Matrix3 matrix = source.RotationMatrix();

    REQUIRE(SameRotation(Quaternion(matrix), source));

    const Quaternion fromAxes(
        matrix * Vector3::RIGHT,
        matrix * Vector3::UP,
        matrix * Vector3::FORWARD);
    REQUIRE(SameRotation(fromAxes, source));
}

TEST_CASE("Quaternion FromLookRotation", "[Math]")
{
    Quaternion q;
    REQUIRE(q.FromLookRotation(Vector3::RIGHT, Vector3::UP));
    REQUIRE(NearlyEqual(q * Vector3::FORWARD, Vector3::RIGHT));

    SECTION("a zero direction falls back to FromRotationTo and still succeeds")
    {
        Quaternion target(45.0f, Vector3::UP);
        REQUIRE(target.FromLookRotation(Vector3::ZERO, Vector3::UP));
        REQUIRE_FALSE(target.IsNaN());
    }

    SECTION("a direction parallel to up still yields a finite rotation")
    {
        Quaternion straight;
        REQUIRE(straight.FromLookRotation(Vector3::UP, Vector3::UP));
        REQUIRE_FALSE(straight.IsNaN());
        REQUIRE(NearlyEqual(straight * Vector3::FORWARD, Vector3::UP, 0.01f));
    }
}

TEST_CASE("Quaternion arithmetic", "[Math]")
{
    const Quaternion a(1.0f, 2.0f, 3.0f, 4.0f);
    const Quaternion b(5.0f, 6.0f, 7.0f, 8.0f);

    REQUIRE(a + b == Quaternion(6.0f, 8.0f, 10.0f, 12.0f));
    REQUIRE(b - a == Quaternion(4.0f, 4.0f, 4.0f, 4.0f));
    REQUIRE(-a == Quaternion(-1.0f, -2.0f, -3.0f, -4.0f));
    REQUIRE(a * 2.0f == Quaternion(2.0f, 4.0f, 6.0f, 8.0f));

    Quaternion acc(1.0f, 2.0f, 3.0f, 4.0f);
    acc += Quaternion(1.0f, 1.0f, 1.0f, 1.0f);
    REQUIRE(acc == Quaternion(2.0f, 3.0f, 4.0f, 5.0f));
    acc *= 2.0f;
    REQUIRE(acc == Quaternion(4.0f, 6.0f, 8.0f, 10.0f));

    REQUIRE_EQ_F(a.LengthSquared(), 30.0f);
    REQUIRE_EQ_F(a.DotProduct(b), 70.0f);
    REQUIRE(a.Conjugate() == Quaternion(1.0f, -2.0f, -3.0f, -4.0f));
}

TEST_CASE("Quaternion composition", "[Math]")
{
    const Quaternion yaw90(90.0f, Vector3::UP);

    SECTION("multiplying by the identity changes nothing")
    {
        REQUIRE(SameRotation(yaw90 * Quaternion::IDENTITY, yaw90));
        REQUIRE(SameRotation(Quaternion::IDENTITY * yaw90, yaw90));
    }

    SECTION("two 90 degree yaws equal one 180 degree yaw")
    {
        REQUIRE(SameRotation(yaw90 * yaw90, Quaternion(180.0f, Vector3::UP)));
    }

    SECTION("composition applies to vectors in the same order")
    {
        const Quaternion pitch90(90.0f, Vector3::RIGHT);
        const Quaternion combined = yaw90 * pitch90;
        REQUIRE(NearlyEqual(combined * Vector3::FORWARD, yaw90 * (pitch90 * Vector3::FORWARD)));
    }

    SECTION("composing with the inverse yields the identity")
    {
        REQUIRE(SameRotation(yaw90 * yaw90.Inverse(), Quaternion::IDENTITY));
    }
}

TEST_CASE("Quaternion normalisation and inverse", "[Math]")
{
    Quaternion q(1.0f, 2.0f, 3.0f, 4.0f);
    q.Normalize();
    REQUIRE_NEAR(q.LengthSquared(), 1.0f, kTolerance);

    const Quaternion normalized = Quaternion(2.0f, 0.0f, 0.0f, 0.0f).Normalized();
    REQUIRE_NEAR(normalized.LengthSquared(), 1.0f, kTolerance);

    SECTION("the inverse of a unit quaternion is its conjugate")
    {
        const Quaternion unit(60.0f, Vector3::UP);
        REQUIRE(NearlyEqual(unit.Inverse(), unit.Conjugate()));
    }

    SECTION("the inverse of a non unit quaternion scales by the inverse square length")
    {
        const Quaternion scaled(0.0f, 2.0f, 0.0f, 0.0f);
        const Quaternion inverse = scaled.Inverse();
        REQUIRE(NearlyEqual(scaled * inverse, Quaternion::IDENTITY));
    }

    SECTION("a degenerate quaternion inverts differently on the SSE and scalar paths")
    {
        const Quaternion degenerate(0.0f, 0.0f, 0.0f, 0.0f);
#ifdef URHO3D_SSE
        REQUIRE(degenerate.Inverse().IsNaN());
#else
        REQUIRE(degenerate.Inverse() == Quaternion::IDENTITY);
#endif
    }
}

TEST_CASE("Quaternion Slerp", "[Math]")
{
    const Quaternion from = Quaternion::IDENTITY;
    const Quaternion to(90.0f, Vector3::UP);

    REQUIRE(SameRotation(from.Slerp(to, 0.0f), from));
    REQUIRE(SameRotation(from.Slerp(to, 1.0f), to));

    const Quaternion half = from.Slerp(to, 0.5f);
    REQUIRE_NEAR(half.Angle(), 45.0f, 0.01f);
    REQUIRE_NEAR(half.LengthSquared(), 1.0f, kTolerance);

    SECTION("Slerp takes the shortest path across the antipode")
    {
        const Quaternion negated = -to;
        REQUIRE_NEAR(from.Slerp(negated, 0.5f).Angle(), 45.0f, 0.01f);
    }

    SECTION("Slerp between identical rotations is stable")
    {
        REQUIRE(SameRotation(to.Slerp(to, 0.5f), to));
    }
}

TEST_CASE("Quaternion Nlerp", "[Math]")
{
    const Quaternion from = Quaternion::IDENTITY;
    const Quaternion to(90.0f, Vector3::UP);

    REQUIRE(SameRotation(from.Nlerp(to, 0.0f), from));
    REQUIRE(SameRotation(from.Nlerp(to, 1.0f), to));
    REQUIRE_NEAR(from.Nlerp(to, 0.5f).LengthSquared(), 1.0f, kTolerance);

    SECTION("the shortest path flag flips the sign when the dot product is negative")
    {
        const Quaternion negated = -to;
        const Quaternion direct = from.Nlerp(negated, 0.5f, false);
        const Quaternion shortest = from.Nlerp(negated, 0.5f, true);
        REQUIRE_FALSE(NearlyEqual(direct, shortest));
        REQUIRE(SameRotation(shortest, from.Nlerp(to, 0.5f)));
    }
}

TEST_CASE("Quaternion validity and string round trip", "[Math]")
{
    REQUIRE_FALSE(Quaternion::IDENTITY.IsNaN());
    REQUIRE_FALSE(Quaternion::IDENTITY.IsInf());
    REQUIRE(Quaternion(M_INFINITY, 0.0f, 0.0f, 0.0f).IsInf());

    REQUIRE(Quaternion::IDENTITY.Equals(Quaternion(1.0f, 0.0f, 0.0f, 0.0f)));
    REQUIRE_FALSE(Quaternion::IDENTITY.Equals(Quaternion(0.9f, 0.0f, 0.0f, 0.0f)));

    const Quaternion q(1.0f, 2.0f, 3.0f, 4.0f);
    REQUIRE(q.ToString() == "1 2 3 4");
    REQUIRE(ToQuaternion(q.ToString()).Equals(q));

    SECTION("a three component string is parsed as Euler angles")
    {
        REQUIRE(SameRotation(ToQuaternion("10 20 30"), Quaternion(10.0f, 20.0f, 30.0f)));
    }
}

TEST_CASE("Quaternion FromRotationMatrix covers every diagonal branch", "[Math]")
{
    const Quaternion aboutX(180.0f, Vector3::RIGHT);
    const Quaternion aboutY(180.0f, Vector3::UP);
    const Quaternion aboutZ(180.0f, Vector3::FORWARD);
    const Quaternion small(10.0f, Vector3::UP);

    REQUIRE(SameRotation(Quaternion(aboutX.RotationMatrix()), aboutX, 0.01f));
    REQUIRE(SameRotation(Quaternion(aboutY.RotationMatrix()), aboutY, 0.01f));
    REQUIRE(SameRotation(Quaternion(aboutZ.RotationMatrix()), aboutZ, 0.01f));
    REQUIRE(SameRotation(Quaternion(small.RotationMatrix()), small, 0.01f));

    const Quaternion xDominant(170.0f, Vector3(1.0f, 0.2f, 0.1f).Normalized());
    REQUIRE(SameRotation(Quaternion(xDominant.RotationMatrix()), xDominant, 0.001f));

    SECTION("each reconstruction rotates vectors the same way as the original")
    {
        const Vector3 probe(0.3f, 0.5f, 0.8f);
        REQUIRE(NearlyEqual(Quaternion(aboutX.RotationMatrix()) * probe, aboutX * probe, 0.01f));
        REQUIRE(NearlyEqual(Quaternion(aboutY.RotationMatrix()) * probe, aboutY * probe, 0.01f));
        REQUIRE(NearlyEqual(Quaternion(aboutZ.RotationMatrix()) * probe, aboutZ * probe, 0.01f));
    }
}

TEST_CASE("Quaternion FromRotationTo picks a fallback axis when antiparallel", "[Math]")
{
    const Quaternion flipX(Vector3::RIGHT, -Vector3::RIGHT);
    REQUIRE_FALSE(flipX.IsNaN());
    REQUIRE(NearlyEqual(flipX * Vector3::RIGHT, -Vector3::RIGHT, 0.01f));

    const Quaternion flipUp(Vector3::UP, -Vector3::UP);
    REQUIRE_FALSE(flipUp.IsNaN());
    REQUIRE(NearlyEqual(flipUp * Vector3::UP, -Vector3::UP, 0.01f));

    const Quaternion flipForward(Vector3::FORWARD, -Vector3::FORWARD);
    REQUIRE_FALSE(flipForward.IsNaN());
    REQUIRE(NearlyEqual(flipForward * Vector3::FORWARD, -Vector3::FORWARD, 0.01f));
}

TEST_CASE("Quaternion EulerAngles handles both gimbal poles", "[Math]")
{
    const Quaternion pitchUp(90.0f, 0.0f, 0.0f);
    REQUIRE_NEAR(pitchUp.EulerAngles().x_, 90.0f, 0.01f);
    REQUIRE_NEAR(pitchUp.EulerAngles().y_, 0.0f, 0.01f);

    const Quaternion pitchDown(-90.0f, 0.0f, 0.0f);
    REQUIRE_NEAR(pitchDown.EulerAngles().x_, -90.0f, 0.01f);
    REQUIRE_NEAR(pitchDown.EulerAngles().y_, 0.0f, 0.01f);
    REQUIRE(SameRotation(Quaternion(pitchDown.EulerAngles()), pitchDown, 0.001f));
    REQUIRE(SameRotation(Quaternion(pitchUp.EulerAngles()), pitchUp, 0.001f));
    const Quaternion tiltedPole(90.0f, 20.0f, 30.0f);
    REQUIRE(SameRotation(Quaternion(tiltedPole.EulerAngles()), tiltedPole, 0.001f));
    const Quaternion tiltedNegativePole(-90.0f, 20.0f, 30.0f);
    REQUIRE(SameRotation(Quaternion(tiltedNegativePole.EulerAngles()), tiltedNegativePole, 0.001f));

    SECTION("the pole values are reported through the individual accessors too")
    {
        REQUIRE_NEAR(pitchDown.PitchAngle(), -90.0f, 0.01f);
        REQUIRE_NEAR(pitchUp.PitchAngle(), 90.0f, 0.01f);
    }
}

TEST_CASE("Quaternion FromLookRotation absorbs degenerate directions", "[Math]")
{
    const float nan = M_INFINITY - M_INFINITY;

    Quaternion target(45.0f, Vector3::UP);
    REQUIRE(target.FromLookRotation(Vector3(nan, nan, nan), Vector3::UP));
    REQUIRE_FALSE(target.IsNaN());

    Quaternion infinite(45.0f, Vector3::UP);
    REQUIRE(infinite.FromLookRotation(Vector3(M_INFINITY, 0.0f, 0.0f), Vector3::UP));
    REQUIRE_FALSE(infinite.IsNaN());
}

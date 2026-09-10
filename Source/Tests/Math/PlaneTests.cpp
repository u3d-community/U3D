#include "TestUtils.h"

#include <Urho3D/Math/Matrix3.h>
#include <Urho3D/Math/Matrix3x4.h>
#include <Urho3D/Math/Matrix4.h>
#include <Urho3D/Math/Plane.h>

using namespace Urho3D;

TEST_CASE("Plane construction", "[Math]")
{
    SECTION("from a normal and a point")
    {
        const Plane plane(Vector3::UP, Vector3(0.0f, 5.0f, 0.0f));
        REQUIRE(plane.normal_.Equals(Vector3::UP));
        REQUIRE_NEAR(plane.d_, -5.0f, 0.001f);
    }

    SECTION("the normal is normalised and the absolute normal cached")
    {
        const Plane plane(Vector3(0.0f, 10.0f, 0.0f), Vector3::ZERO);
        REQUIRE(plane.normal_.Equals(Vector3::UP));
        REQUIRE(plane.absNormal_.Equals(Vector3::UP));

        const Plane negative(Vector3(0.0f, -1.0f, 0.0f), Vector3::ZERO);
        REQUIRE(negative.absNormal_.Equals(Vector3::UP));
    }

    SECTION("from three points the winding sets the normal direction")
    {
        const Plane plane(Vector3::ZERO, Vector3::FORWARD, Vector3::RIGHT);
        REQUIRE(plane.normal_.Equals(Vector3::UP));
    }

    SECTION("from a Vector4 the components are taken verbatim")
    {
        const Plane plane(Vector4(0.0f, 1.0f, 0.0f, -3.0f));
        REQUIRE(plane.normal_ == Vector3::UP);
        REQUIRE(plane.d_ == -3.0f);
        REQUIRE(plane.ToVector4() == Vector4(0.0f, 1.0f, 0.0f, -3.0f));
    }

    SECTION("the default plane is the origin plane and UP is the y plane")
    {
        REQUIRE(Plane().d_ == 0.0f);
        REQUIRE(Plane::UP.normal_.Equals(Vector3::UP));
        REQUIRE_NEAR(Plane::UP.d_, 0.0f, 0.001f);
    }

    SECTION("copy and assignment preserve every field")
    {
        const Plane source(Vector3::RIGHT, Vector3(2.0f, 0.0f, 0.0f));
        const Plane copy(source);
        REQUIRE(copy.normal_ == source.normal_);
        REQUIRE(copy.d_ == source.d_);

        Plane assigned;
        assigned = source;
        REQUIRE(assigned.normal_ == source.normal_);
        REQUIRE(assigned.d_ == source.d_);
    }
}

TEST_CASE("Plane Distance is signed", "[Math]")
{
    const Plane plane(Vector3::UP, Vector3::ZERO);

    REQUIRE_NEAR(plane.Distance(Vector3(0.0f, 5.0f, 0.0f)), 5.0f, 0.001f);
    REQUIRE_NEAR(plane.Distance(Vector3(0.0f, -5.0f, 0.0f)), -5.0f, 0.001f);
    REQUIRE_NEAR(plane.Distance(Vector3(10.0f, 0.0f, 10.0f)), 0.0f, 0.001f);
}

TEST_CASE("Plane Project drops the point onto the plane", "[Math]")
{
    const Plane plane(Vector3::UP, Vector3(0.0f, 2.0f, 0.0f));

    const Vector3 projected = plane.Project(Vector3(3.0f, 10.0f, 4.0f));
    REQUIRE(projected.Equals(Vector3(3.0f, 2.0f, 4.0f)));
    REQUIRE_NEAR(plane.Distance(projected), 0.0f, 0.001f);

    SECTION("a point already on the plane is unchanged")
    {
        const Vector3 onPlane(1.0f, 2.0f, 3.0f);
        REQUIRE(plane.Project(onPlane).Equals(onPlane));
    }
}

TEST_CASE("Plane Reflect mirrors a direction", "[Math]")
{
    const Plane plane(Vector3::UP, Vector3::ZERO);

    REQUIRE(plane.Reflect(Vector3(0.0f, -1.0f, 0.0f)).Equals(Vector3(0.0f, 1.0f, 0.0f)));
    REQUIRE(plane.Reflect(Vector3(1.0f, -1.0f, 0.0f)).Equals(Vector3(1.0f, 1.0f, 0.0f)));

    SECTION("a direction parallel to the plane is unchanged")
    {
        REQUIRE(plane.Reflect(Vector3::RIGHT).Equals(Vector3::RIGHT));
    }

    SECTION("reflecting twice returns the original direction")
    {
        const Vector3 direction(0.3f, -0.7f, 0.2f);
        REQUIRE(plane.Reflect(plane.Reflect(direction)).Equals(direction));
    }
}

TEST_CASE("Plane ReflectionMatrix mirrors points", "[Math]")
{
    const Plane plane(Vector3::UP, Vector3::ZERO);
    const Matrix3x4 reflection = plane.ReflectionMatrix();

    REQUIRE((reflection * Vector3(1.0f, 5.0f, 2.0f)).Equals(Vector3(1.0f, -5.0f, 2.0f)));
    REQUIRE((reflection * Vector3(1.0f, 0.0f, 2.0f)).Equals(Vector3(1.0f, 0.0f, 2.0f)));

    SECTION("an offset plane reflects about its own height")
    {
        const Matrix3x4 offset = Plane(Vector3::UP, Vector3(0.0f, 2.0f, 0.0f)).ReflectionMatrix();
        REQUIRE((offset * Vector3(0.0f, 5.0f, 0.0f)).Equals(Vector3(0.0f, -1.0f, 0.0f)));
    }
}

TEST_CASE("Plane Transform", "[Math]")
{
    const Plane plane(Vector3::UP, Vector3::ZERO);

    SECTION("translation shifts the plane constant")
    {
        const Matrix3x4 translation(Vector3(0.0f, 3.0f, 0.0f), Quaternion::IDENTITY, 1.0f);
        const Plane moved = plane.Transformed(translation);
        REQUIRE(moved.normal_.Equals(Vector3::UP));
        REQUIRE_NEAR(moved.Distance(Vector3(0.0f, 3.0f, 0.0f)), 0.0f, 0.001f);
    }

    SECTION("rotation turns the normal the same way it turns a vector")
    {
        const Quaternion quaternion(90.0f, Vector3::FORWARD);
        const Matrix3 rotation = quaternion.RotationMatrix();
        const Plane rotated = plane.Transformed(rotation);
        REQUIRE(rotated.normal_.Equals(quaternion * Vector3::UP));
        REQUIRE_NEAR(rotated.normal_.Abs().x_, 1.0f, 0.001f);
    }

    SECTION("in place Transform matches Transformed")
    {
        const Matrix3x4 transform(Vector3(1.0f, 2.0f, 3.0f), Quaternion(30.0f, Vector3::RIGHT), 1.0f);
        Plane inPlace = plane;
        inPlace.Transform(transform);
        const Plane expected = plane.Transformed(transform);
        REQUIRE(inPlace.normal_.Equals(expected.normal_));
        REQUIRE_NEAR(inPlace.d_, expected.d_, 0.001f);
    }

    SECTION("a Matrix4 transform agrees with the equivalent Matrix3x4")
    {
        const Matrix3x4 affine(Vector3(0.0f, 4.0f, 0.0f), Quaternion::IDENTITY, 1.0f);
        const Matrix4 full = affine.ToMatrix4();
        const Plane viaAffine = plane.Transformed(affine);
        const Plane viaFull = plane.Transformed(full);
        REQUIRE(viaFull.normal_.Equals(viaAffine.normal_));
        REQUIRE_NEAR(viaFull.d_, viaAffine.d_, 0.001f);
    }
}

TEST_CASE("Plane Transform by a Matrix3 and a Matrix4", "[Math]")
{
    const Plane plane(Vector3::UP, Vector3(0.0f, 2.0f, 0.0f));

    SECTION("a Matrix3 rotation turns the normal and keeps the offset magnitude")
    {
        const Quaternion rotation(90.0f, Vector3::FORWARD);
        Plane inPlace = plane;
        inPlace.Transform(rotation.RotationMatrix());

        REQUIRE(inPlace.normal_.Equals(rotation * Vector3::UP));
        REQUIRE_NEAR(Abs(inPlace.d_), 2.0f, 0.001f);
    }

    SECTION("a Matrix4 transform matches the equivalent Matrix3x4")
    {
        const Matrix3x4 affine(Vector3(0.0f, 3.0f, 0.0f), Quaternion(20.0f, Vector3::RIGHT), 1.0f);

        Plane viaMatrix4 = plane;
        viaMatrix4.Transform(affine.ToMatrix4());

        const Plane viaAffine = plane.Transformed(affine);
        REQUIRE(viaMatrix4.normal_.Equals(viaAffine.normal_));
        REQUIRE_NEAR(viaMatrix4.d_, viaAffine.d_, 0.001f);
    }
}

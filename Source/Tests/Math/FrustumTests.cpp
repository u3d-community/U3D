#include "TestUtils.h"

#include <Urho3D/Math/BoundingBox.h>
#include <Urho3D/Math/Frustum.h>
#include <Urho3D/Math/Matrix4.h>
#include <Urho3D/Math/Polyhedron.h>
#include <Urho3D/Math/Sphere.h>

using namespace Urho3D;

namespace
{

Frustum MakePerspective()
{
    Frustum frustum;
    frustum.Define(90.0f, 1.0f, 1.0f, 1.0f, 100.0f);
    return frustum;
}

}

TEST_CASE("Frustum Define from field of view", "[Math]")
{
    const Frustum frustum = MakePerspective();

    float minZ = M_INFINITY;
    float maxZ = -M_INFINITY;
    for (const Vector3& vertex : frustum.vertices_)
    {
        minZ = Min(minZ, vertex.z_);
        maxZ = Max(maxZ, vertex.z_);
    }
    REQUIRE_NEAR(minZ, 1.0f, 0.01f);
    REQUIRE_NEAR(maxZ, 100.0f, 0.01f);

    SECTION("a 90 degree horizontal field of view opens at 45 degrees per side")
    {
        float maxAbsX = 0.0f;
        for (const Vector3& vertex : frustum.vertices_)
        {
            if (Equals(vertex.z_, 1.0f))
                maxAbsX = Max(maxAbsX, Abs(vertex.x_));
        }
        REQUIRE_NEAR(maxAbsX, 1.0f, 0.01f);
    }

    SECTION("copy and assignment preserve the planes")
    {
        const Frustum copy(frustum);
        REQUIRE(copy.IsInside(Vector3(0.0f, 0.0f, 10.0f)) == INSIDE);

        Frustum assigned;
        assigned = frustum;
        REQUIRE(assigned.IsInside(Vector3(0.0f, 0.0f, 10.0f)) == INSIDE);
        REQUIRE(assigned.IsInside(Vector3(0.0f, 0.0f, -10.0f)) == OUTSIDE);
    }
}

TEST_CASE("Frustum point containment", "[Math]")
{
    const Frustum frustum = MakePerspective();

    REQUIRE(frustum.IsInside(Vector3(0.0f, 0.0f, 10.0f)) == INSIDE);
    REQUIRE(frustum.IsInside(Vector3(0.0f, 0.0f, -10.0f)) == OUTSIDE);
    REQUIRE(frustum.IsInside(Vector3(0.0f, 0.0f, 200.0f)) == OUTSIDE);
    REQUIRE(frustum.IsInside(Vector3(0.0f, 0.0f, 0.5f)) == OUTSIDE);
    REQUIRE(frustum.IsInside(Vector3(100.0f, 0.0f, 10.0f)) == OUTSIDE);

    SECTION("Distance is zero inside and positive outside")
    {
        REQUIRE_EQ_F(frustum.Distance(Vector3(0.0f, 0.0f, 10.0f)), 0.0f);
        REQUIRE(frustum.Distance(Vector3(0.0f, 0.0f, -10.0f)) > 0.0f);
        REQUIRE_NEAR(frustum.Distance(Vector3(0.0f, 0.0f, 0.0f)), 1.0f, 0.01f);
    }
}

TEST_CASE("Frustum sphere containment", "[Math]")
{
    const Frustum frustum = MakePerspective();

    REQUIRE(frustum.IsInside(Sphere(Vector3(0.0f, 0.0f, 10.0f), 1.0f)) == INSIDE);
    REQUIRE(frustum.IsInside(Sphere(Vector3(0.0f, 0.0f, -50.0f), 1.0f)) == OUTSIDE);
    REQUIRE(frustum.IsInside(Sphere(Vector3(0.0f, 0.0f, 1.0f), 2.0f)) == INTERSECTS);

    REQUIRE(frustum.IsInsideFast(Sphere(Vector3(0.0f, 0.0f, 10.0f), 1.0f)) == INSIDE);
    REQUIRE(frustum.IsInsideFast(Sphere(Vector3(0.0f, 0.0f, -50.0f), 1.0f)) == OUTSIDE);

    SECTION("a sphere straddling the far plane intersects")
    {
        REQUIRE(frustum.IsInside(Sphere(Vector3(0.0f, 0.0f, 100.0f), 5.0f)) == INTERSECTS);
    }
}

TEST_CASE("Frustum box containment", "[Math]")
{
    const Frustum frustum = MakePerspective();

    REQUIRE(frustum.IsInside(BoundingBox(Vector3(-1.0f, -1.0f, 9.0f), Vector3(1.0f, 1.0f, 11.0f))) == INSIDE);
    REQUIRE(frustum.IsInside(BoundingBox(Vector3(-1.0f, -1.0f, -20.0f), Vector3(1.0f, 1.0f, -10.0f))) == OUTSIDE);
    REQUIRE(frustum.IsInside(BoundingBox(Vector3(-1.0f, -1.0f, -5.0f), Vector3(1.0f, 1.0f, 5.0f))) == INTERSECTS);

    REQUIRE(frustum.IsInsideFast(BoundingBox(Vector3(-1.0f, -1.0f, 9.0f), Vector3(1.0f, 1.0f, 11.0f))) == INSIDE);
    REQUIRE(frustum.IsInsideFast(BoundingBox(Vector3(-1.0f, -1.0f, -20.0f), Vector3(1.0f, 1.0f, -10.0f))) == OUTSIDE);

    SECTION("a huge box containing the whole frustum still registers")
    {
        REQUIRE(frustum.IsInside(BoundingBox(-1000.0f, 1000.0f)) != OUTSIDE);
    }
}

TEST_CASE("Frustum Define from a bounding box", "[Math]")
{
    Frustum frustum;
    frustum.Define(BoundingBox(Vector3(-1.0f, -1.0f, 1.0f), Vector3(1.0f, 1.0f, 10.0f)));

    REQUIRE(frustum.IsInside(Vector3(0.0f, 0.0f, 5.0f)) == INSIDE);
    REQUIRE(frustum.IsInside(Vector3(0.0f, 0.0f, 20.0f)) == OUTSIDE);
}

TEST_CASE("Frustum Define from near and far corners", "[Math]")
{
    Frustum frustum;
    frustum.Define(Vector3(1.0f, 1.0f, 1.0f), Vector3(1.0f, 1.0f, 10.0f));

    REQUIRE(frustum.IsInside(Vector3(0.0f, 0.0f, 5.0f)) == INSIDE);
    REQUIRE(frustum.IsInside(Vector3(0.0f, 0.0f, -5.0f)) == OUTSIDE);
    REQUIRE(frustum.IsInside(Vector3(0.0f, 0.0f, 20.0f)) == OUTSIDE);
    REQUIRE(frustum.IsInside(Vector3(5.0f, 0.0f, 5.0f)) == OUTSIDE);
}

TEST_CASE("Frustum DefineOrtho keeps parallel sides", "[Math]")
{
    Frustum frustum;
    frustum.DefineOrtho(4.0f, 1.0f, 1.0f, 1.0f, 100.0f);

    REQUIRE(frustum.IsInside(Vector3(1.0f, 0.0f, 2.0f)) == INSIDE);
    REQUIRE(frustum.IsInside(Vector3(1.0f, 0.0f, 90.0f)) == INSIDE);
    REQUIRE(frustum.IsInside(Vector3(10.0f, 0.0f, 50.0f)) == OUTSIDE);
}

TEST_CASE("Frustum Define from a projection matrix", "[Math]")
{
    const Frustum source = MakePerspective();

    Matrix4 projection = Matrix4::IDENTITY;
    projection.SetScale(Vector3(0.5f, 0.5f, 0.01f));

    Frustum fromProjection;
    fromProjection.Define(projection);

    REQUIRE(fromProjection.IsInside(Vector3::ZERO) != OUTSIDE);

    SECTION("DefineSplit narrows the depth range of an existing projection")
    {
        Frustum split;
        split.DefineSplit(projection, 10.0f, 20.0f);
        REQUIRE(split.IsInside(Vector3(0.0f, 0.0f, 1000.0f)) == OUTSIDE);
    }

    REQUIRE(source.IsInside(Vector3(0.0f, 0.0f, 10.0f)) == INSIDE);
}

TEST_CASE("Frustum Transform", "[Math]")
{
    const Frustum frustum = MakePerspective();

    SECTION("translation moves the volume")
    {
        const Matrix3x4 translation(Vector3(100.0f, 0.0f, 0.0f), Quaternion::IDENTITY, 1.0f);
        const Frustum moved = frustum.Transformed(translation);

        REQUIRE(moved.IsInside(Vector3(100.0f, 0.0f, 10.0f)) == INSIDE);
        REQUIRE(moved.IsInside(Vector3(0.0f, 0.0f, 10.0f)) == OUTSIDE);
    }

    SECTION("rotation turns the volume")
    {
        const Matrix3 rotation = Quaternion(90.0f, Vector3::UP).RotationMatrix();
        const Frustum rotated = frustum.Transformed(rotation);

        REQUIRE(rotated.IsInside(Vector3(10.0f, 0.0f, 0.0f)) == INSIDE);
        REQUIRE(rotated.IsInside(Vector3(0.0f, 0.0f, 10.0f)) == OUTSIDE);
    }

    SECTION("in place Transform matches Transformed")
    {
        const Matrix3x4 transform(Vector3(5.0f, 0.0f, 0.0f), Quaternion(30.0f, Vector3::UP), 1.0f);
        Frustum inPlace = frustum;
        inPlace.Transform(transform);
        const Frustum expected = frustum.Transformed(transform);

        for (unsigned i = 0; i < NUM_FRUSTUM_VERTICES; ++i)
            REQUIRE(inPlace.vertices_[i].Equals(expected.vertices_[i]));
    }

    SECTION("a Matrix3 rotation matches the equivalent Matrix3x4")
    {
        const Quaternion rotation(45.0f, Vector3::RIGHT);
        Frustum viaMatrix3 = frustum;
        viaMatrix3.Transform(rotation.RotationMatrix());

        Frustum viaMatrix3x4 = frustum;
        viaMatrix3x4.Transform(Matrix3x4(Vector3::ZERO, rotation, 1.0f));

        for (unsigned i = 0; i < NUM_FRUSTUM_VERTICES; ++i)
        {
            const Vector3& expected = viaMatrix3.vertices_[i];
            const Vector3& actual = viaMatrix3x4.vertices_[i];
            REQUIRE_NEAR((expected - actual).Length(), 0.0f, expected.Length() * 1e-5f);
        }
    }
}

TEST_CASE("Frustum Projected produces a screen space rect", "[Math]")
{
    const Frustum frustum = MakePerspective();

    Matrix4 projection = Matrix4::IDENTITY;
    projection.SetScale(Vector3(1.0f, 1.0f, 0.01f));

    const Rect projected = frustum.Projected(projection);
    REQUIRE(projected.Defined());
    REQUIRE(projected.Size().x_ > 0.0f);
}

TEST_CASE("Frustum UpdatePlanes rebuilds from the vertices", "[Math]")
{
    Frustum frustum = MakePerspective();

    for (auto& vertex : frustum.vertices_)
        vertex += Vector3(50.0f, 0.0f, 0.0f);
    frustum.UpdatePlanes();

    REQUIRE(frustum.IsInside(Vector3(50.0f, 0.0f, 10.0f)) == INSIDE);
    REQUIRE(frustum.IsInside(Vector3(0.0f, 0.0f, 10.0f)) == OUTSIDE);
}

TEST_CASE("Frustum Projected clips against the near plane", "[Math]")
{
    Frustum straddling;
    straddling.Define(Vector3(1.0f, 1.0f, -5.0f), Vector3(2.0f, 2.0f, 20.0f));

    Matrix4 projection = Matrix4::IDENTITY;
    projection.SetScale(Vector3(1.0f, 1.0f, 0.1f));

    const Rect projected = straddling.Projected(projection);
    REQUIRE(projected.Defined());
    REQUIRE_FALSE(projected.Min().IsNaN());
    REQUIRE_FALSE(projected.Max().IsNaN());

    SECTION("a frustum entirely behind the near plane still yields a defined rect")
    {
        Frustum behind;
        behind.Define(Vector3(1.0f, 1.0f, -20.0f), Vector3(2.0f, 2.0f, -5.0f));
        REQUIRE_FALSE(behind.Projected(projection).Min().IsNaN());
    }
}

TEST_CASE("Frustum flips inverted planes under a mirrored transform", "[Math]")
{
    const Frustum frustum = MakePerspective();

    Matrix3 mirror = Matrix3::IDENTITY;
    mirror.SetScale(Vector3(-1.0f, 1.0f, 1.0f));

    const Frustum mirrored = frustum.Transformed(mirror);

    REQUIRE(mirrored.IsInside(Vector3(0.0f, 0.0f, 10.0f)) == INSIDE);
    REQUIRE(mirrored.IsInside(Vector3(0.0f, 0.0f, -10.0f)) == OUTSIDE);
    REQUIRE(mirrored.IsInside(Vector3(0.0f, 0.0f, 200.0f)) == OUTSIDE);
}

TEST_CASE("Frustum Projected clips edges whose far vertex is behind the near plane", "[Math]")
{
    Frustum inverted;
    inverted.Define(Vector3(1.0f, 1.0f, 5.0f), Vector3(2.0f, 2.0f, -20.0f));

    Matrix4 projection = Matrix4::IDENTITY;
    projection.SetScale(Vector3(1.0f, 1.0f, 0.1f));

    const Rect projected = inverted.Projected(projection);
    REQUIRE_FALSE(projected.Min().IsNaN());
    REQUIRE_FALSE(projected.Max().IsNaN());
}

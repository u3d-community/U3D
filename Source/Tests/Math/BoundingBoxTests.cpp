#include "TestUtils.h"

#include <Urho3D/Core/StringUtils.h>
#include <Urho3D/Math/BoundingBox.h>
#include <Urho3D/Math/Frustum.h>
#include <Urho3D/Math/Matrix3x4.h>
#include <Urho3D/Math/Polyhedron.h>
#include <Urho3D/Math/Sphere.h>

using namespace Urho3D;

TEST_CASE("BoundingBox default construction is undefined", "[Math]")
{
    const BoundingBox box;
    REQUIRE_FALSE(box.Defined());

    REQUIRE(BoundingBox(Vector3::ZERO, Vector3::ONE).Defined());
    REQUIRE(BoundingBox(-1.0f, 1.0f).Defined());
}

TEST_CASE("BoundingBox construction variants agree", "[Math]")
{
    const BoundingBox reference(Vector3(-1.0f, -1.0f, -1.0f), Vector3(1.0f, 1.0f, 1.0f));

    REQUIRE(BoundingBox(-1.0f, 1.0f) == reference);
    REQUIRE(BoundingBox(reference) == reference);

    BoundingBox assigned;
    assigned = reference;
    REQUIRE(assigned == reference);
    REQUIRE(assigned != BoundingBox(0.0f, 1.0f));

    SECTION("a rect becomes a flat box on the z plane")
    {
        const BoundingBox fromRect(Rect(1.0f, 2.0f, 3.0f, 4.0f));
        REQUIRE(fromRect.min_ == Vector3(1.0f, 2.0f, 0.0f));
        REQUIRE(fromRect.max_ == Vector3(3.0f, 4.0f, 0.0f));

        BoundingBox assignedRect;
        assignedRect = Rect(1.0f, 2.0f, 3.0f, 4.0f);
        REQUIRE(assignedRect == fromRect);
    }

    SECTION("from a vertex array")
    {
        const Vector3 vertices[] = {
            Vector3(-1.0f, 0.0f, 0.0f),
            Vector3(2.0f, 3.0f, 0.0f),
            Vector3(0.0f, -5.0f, 4.0f),
        };
        const BoundingBox fromVertices(vertices, 3);
        REQUIRE(fromVertices.min_ == Vector3(-1.0f, -5.0f, 0.0f));
        REQUIRE(fromVertices.max_ == Vector3(2.0f, 3.0f, 4.0f));
    }

    SECTION("an empty vertex array leaves the box undefined")
    {
        const Vector3 vertices[] = {Vector3::ZERO};
        const BoundingBox fromNone(vertices, 0);
        REQUIRE_FALSE(fromNone.Defined());
    }

    SECTION("from a sphere the box is the axis aligned bound")
    {
        const BoundingBox fromSphere(Sphere(Vector3(1.0f, 2.0f, 3.0f), 2.0f));
        REQUIRE(fromSphere.min_ == Vector3(-1.0f, 0.0f, 1.0f));
        REQUIRE(fromSphere.max_ == Vector3(3.0f, 4.0f, 5.0f));
    }
}

TEST_CASE("BoundingBox accessors", "[Math]")
{
    const BoundingBox box(Vector3(-2.0f, -4.0f, -6.0f), Vector3(2.0f, 4.0f, 6.0f));

    REQUIRE(box.Center() == Vector3::ZERO);
    REQUIRE(box.Size() == Vector3(4.0f, 8.0f, 12.0f));
    REQUIRE(box.HalfSize() == Vector3(2.0f, 4.0f, 6.0f));

    REQUIRE(box.ToString() == "-2 -4 -6 - 2 4 6");
}

TEST_CASE("BoundingBox Define, Merge and Clear", "[Math]")
{
    BoundingBox box;
    box.Define(Vector3(1.0f, 1.0f, 1.0f));
    REQUIRE(box.Defined());
    REQUIRE(box.min_ == Vector3(1.0f, 1.0f, 1.0f));
    REQUIRE(box.max_ == Vector3(1.0f, 1.0f, 1.0f));

    box.Merge(Vector3(3.0f, 5.0f, 7.0f));
    REQUIRE(box.max_ == Vector3(3.0f, 5.0f, 7.0f));

    box.Merge(Vector3(-3.0f, -5.0f, -7.0f));
    REQUIRE(box.min_ == Vector3(-3.0f, -5.0f, -7.0f));

    SECTION("merging an interior point changes nothing")
    {
        const BoundingBox before = box;
        box.Merge(Vector3::ZERO);
        REQUIRE(box == before);
    }

    SECTION("merging another box takes the union")
    {
        box.Merge(BoundingBox(Vector3(-10.0f, 0.0f, 0.0f), Vector3(0.0f, 10.0f, 0.0f)));
        REQUIRE(box.min_.x_ == -10.0f);
        REQUIRE(box.max_.y_ == 10.0f);
    }

    SECTION("merging a vertex array")
    {
        const Vector3 vertices[] = {Vector3(100.0f, 0.0f, 0.0f), Vector3(-100.0f, 0.0f, 0.0f)};
        box.Merge(vertices, 2);
        REQUIRE(box.min_.x_ == -100.0f);
        REQUIRE(box.max_.x_ == 100.0f);
    }

    SECTION("merging a sphere expands to contain it")
    {
        box.Merge(Sphere(Vector3(0.0f, 20.0f, 0.0f), 1.0f));
        REQUIRE(box.max_.y_ >= 21.0f);
    }

    SECTION("Clear returns to the undefined state")
    {
        box.Clear();
        REQUIRE_FALSE(box.Defined());
    }

    SECTION("Define from min and max floats builds a cube")
    {
        BoundingBox cube;
        cube.Define(-3.0f, 3.0f);
        REQUIRE(cube.min_ == Vector3(-3.0f, -3.0f, -3.0f));
        REQUIRE(cube.max_ == Vector3(3.0f, 3.0f, 3.0f));
    }
}

TEST_CASE("BoundingBox containment", "[Math]")
{
    const BoundingBox box(-1.0f, 1.0f);

    REQUIRE(box.IsInside(Vector3::ZERO) == INSIDE);
    REQUIRE(box.IsInside(Vector3(2.0f, 0.0f, 0.0f)) == OUTSIDE);
    REQUIRE(box.IsInside(Vector3(0.0f, -2.0f, 0.0f)) == OUTSIDE);
    REQUIRE(box.IsInside(Vector3(0.0f, 0.0f, 2.0f)) == OUTSIDE);

    REQUIRE(box.IsInside(BoundingBox(-0.5f, 0.5f)) == INSIDE);
    REQUIRE(box.IsInside(BoundingBox(0.5f, 2.0f)) == INTERSECTS);
    REQUIRE(box.IsInside(BoundingBox(5.0f, 6.0f)) == OUTSIDE);

    REQUIRE(box.IsInsideFast(BoundingBox(-0.5f, 0.5f)) == INSIDE);
    REQUIRE(box.IsInsideFast(BoundingBox(5.0f, 6.0f)) == OUTSIDE);

    SECTION("against spheres")
    {
        REQUIRE(box.IsInside(Sphere(Vector3::ZERO, 0.5f)) == INSIDE);
        REQUIRE(box.IsInside(Sphere(Vector3(10.0f, 0.0f, 0.0f), 1.0f)) == OUTSIDE);
        REQUIRE(box.IsInsideFast(Sphere(Vector3(10.0f, 0.0f, 0.0f), 1.0f)) == OUTSIDE);
    }
}

TEST_CASE("BoundingBox DistanceToPoint", "[Math]")
{
    const BoundingBox box(-1.0f, 1.0f);

    REQUIRE_EQ_F(box.DistanceToPoint(Vector3::ZERO), 0.0f);
    REQUIRE_EQ_F(box.DistanceToPoint(Vector3(1.0f, 0.0f, 0.0f)), 0.0f);
    REQUIRE_NEAR(box.DistanceToPoint(Vector3(4.0f, 0.0f, 0.0f)), 3.0f, 0.001f);
    REQUIRE_NEAR(box.DistanceToPoint(Vector3(-4.0f, 0.0f, 0.0f)), 3.0f, 0.001f);
    REQUIRE_NEAR(box.DistanceToPoint(Vector3(4.0f, 5.0f, 1.0f)), Vector3(3.0f, 4.0f, 0.0f).Length(), 0.001f);
}

TEST_CASE("BoundingBox Clip", "[Math]")
{
    BoundingBox box(-10.0f, 10.0f);
    box.Clip(BoundingBox(0.0f, 20.0f));
    REQUIRE(box.min_ == Vector3(0.0f, 0.0f, 0.0f));
    REQUIRE(box.max_ == Vector3(10.0f, 10.0f, 10.0f));

    SECTION("clipping against a containing box is a no-op")
    {
        BoundingBox inner(-1.0f, 1.0f);
        inner.Clip(BoundingBox(-10.0f, 10.0f));
        REQUIRE(inner == BoundingBox(-1.0f, 1.0f));
    }
}

TEST_CASE("BoundingBox Transform", "[Math]")
{
    const BoundingBox box(-1.0f, 1.0f);

    SECTION("translation moves the box without resizing it")
    {
        const Matrix3x4 translation(Vector3(10.0f, 0.0f, 0.0f), Quaternion::IDENTITY, 1.0f);
        const BoundingBox moved = box.Transformed(translation);
        REQUIRE(moved.Center().Equals(Vector3(10.0f, 0.0f, 0.0f)));
        REQUIRE(moved.Size().Equals(box.Size()));
    }

    SECTION("uniform scale scales the size")
    {
        const Matrix3x4 scale(Vector3::ZERO, Quaternion::IDENTITY, 2.0f);
        REQUIRE(box.Transformed(scale).Size().Equals(Vector3(4.0f, 4.0f, 4.0f)));
    }

    SECTION("rotation grows the axis aligned bound of a cube diagonally")
    {
        const Matrix3x4 rotation(Vector3::ZERO, Quaternion(45.0f, Vector3::UP), 1.0f);
        const BoundingBox rotated = box.Transformed(rotation);
        REQUIRE(rotated.Size().x_ > box.Size().x_);
        REQUIRE_NEAR(rotated.Size().y_, box.Size().y_, 0.001f);
    }

    SECTION("in place transform matches Transformed")
    {
        const Matrix3x4 transform(Vector3(1.0f, 2.0f, 3.0f), Quaternion(30.0f, Vector3::UP), 1.5f);
        BoundingBox inPlace = box;
        inPlace.Transform(transform);
        REQUIRE(inPlace.min_.Equals(box.Transformed(transform).min_));
        REQUIRE(inPlace.max_.Equals(box.Transformed(transform).max_));
    }
}

TEST_CASE("BoundingBox interoperates with frustum and polyhedron", "[Math]")
{
    Frustum frustum;
    frustum.Define(45.0f, 1.0f, 1.0f, 1.0f, 10.0f);

    const BoundingBox fromFrustum(frustum);
    REQUIRE(fromFrustum.Defined());
    REQUIRE(fromFrustum.max_.z_ >= 10.0f - M_LARGE_EPSILON);

    const Polyhedron poly(BoundingBox(-2.0f, 2.0f));
    const BoundingBox fromPoly(poly);
    REQUIRE(fromPoly.Defined());
    REQUIRE(fromPoly.min_.Equals(Vector3(-2.0f, -2.0f, -2.0f)));
    REQUIRE(fromPoly.max_.Equals(Vector3(2.0f, 2.0f, 2.0f)));

    SECTION("merging a frustum expands the box")
    {
        BoundingBox box(Vector3::ZERO, Vector3::ZERO);
        box.Merge(frustum);
        REQUIRE(box.max_.z_ >= 10.0f - M_LARGE_EPSILON);
    }

    SECTION("merging a polyhedron expands the box")
    {
        BoundingBox box(Vector3::ZERO, Vector3::ZERO);
        box.Merge(poly);
        REQUIRE(box.min_.x_ <= -2.0f + M_LARGE_EPSILON);
    }
}

TEST_CASE("BoundingBox Define overloads", "[Math]")
{
    BoundingBox fromBox;
    fromBox.Define(BoundingBox(Vector3(1.0f, 2.0f, 3.0f), Vector3(4.0f, 5.0f, 6.0f)));
    REQUIRE(fromBox.min_ == Vector3(1.0f, 2.0f, 3.0f));
    REQUIRE(fromBox.max_ == Vector3(4.0f, 5.0f, 6.0f));

    BoundingBox fromRect;
    fromRect.Define(Rect(1.0f, 2.0f, 3.0f, 4.0f));
    REQUIRE(fromRect.min_ == Vector3(1.0f, 2.0f, 0.0f));
    REQUIRE(fromRect.max_ == Vector3(3.0f, 4.0f, 0.0f));

    BoundingBox fromCorners;
    fromCorners.Define(Vector3(-1.0f, -2.0f, -3.0f), Vector3(1.0f, 2.0f, 3.0f));
    REQUIRE(fromCorners.min_ == Vector3(-1.0f, -2.0f, -3.0f));
    REQUIRE(fromCorners.max_ == Vector3(1.0f, 2.0f, 3.0f));
}

TEST_CASE("BoundingBox Merge grows on each axis independently", "[Math]")
{
    BoundingBox box(-1.0f, 1.0f);

    box.Merge(BoundingBox(Vector3(5.0f, 0.0f, 0.0f), Vector3(6.0f, 0.5f, 0.5f)));
    REQUIRE_NEAR(box.max_.x_, 6.0f, 0.001f);

    box.Merge(BoundingBox(Vector3(0.0f, 5.0f, 0.0f), Vector3(0.5f, 6.0f, 0.5f)));
    REQUIRE_NEAR(box.max_.y_, 6.0f, 0.001f);

    box.Merge(BoundingBox(Vector3(0.0f, 0.0f, 5.0f), Vector3(0.5f, 0.5f, 6.0f)));
    REQUIRE_NEAR(box.max_.z_, 6.0f, 0.001f);

    box.Merge(BoundingBox(Vector3(-6.0f, -6.0f, -6.0f), Vector3(-5.0f, -5.0f, -5.0f)));
    REQUIRE(box.min_.Equals(Vector3(-6.0f, -6.0f, -6.0f)));

    SECTION("merging a fully contained box changes nothing")
    {
        const BoundingBox before = box;
        box.Merge(BoundingBox(-0.5f, 0.5f));
        REQUIRE(box == before);
    }

    SECTION("Clear resets both corners to the undefined sentinels")
    {
        box.Clear();
        REQUIRE_FALSE(box.Defined());
        REQUIRE(box.min_.x_ == M_INFINITY);
        REQUIRE(box.max_.x_ == -M_INFINITY);
    }
}

TEST_CASE("BoundingBox Transform by a Matrix3", "[Math]")
{
    const BoundingBox box(-1.0f, 1.0f);
    const Matrix3 rotation = Quaternion(45.0f, Vector3::UP).RotationMatrix();

    const BoundingBox rotated = box.Transformed(rotation);
    REQUIRE(rotated.Size().x_ > box.Size().x_);
    REQUIRE_NEAR(rotated.Size().y_, box.Size().y_, 0.001f);

    BoundingBox inPlace = box;
    inPlace.Transform(rotation);
    REQUIRE(inPlace.min_.Equals(rotated.min_));
    REQUIRE(inPlace.max_.Equals(rotated.max_));

    SECTION("it matches the equivalent Matrix3x4")
    {
        const BoundingBox viaAffine = box.Transformed(Matrix3x4(rotation));
        REQUIRE(rotated.min_.Equals(viaAffine.min_));
        REQUIRE(rotated.max_.Equals(viaAffine.max_));
    }
}

TEST_CASE("BoundingBox Projected", "[Math]")
{
    const BoundingBox box(Vector3(-1.0f, -1.0f, 5.0f), Vector3(1.0f, 1.0f, 10.0f));

    Matrix4 projection = Matrix4::IDENTITY;
    projection.SetScale(Vector3(1.0f, 1.0f, 0.1f));

    const Rect projected = box.Projected(projection);
    REQUIRE(projected.Defined());
    REQUIRE(projected.Size().x_ > 0.0f);
    REQUIRE(projected.Size().y_ > 0.0f);

    SECTION("a box straddling the near clip is pushed forward instead of inverting")
    {
        const BoundingBox straddling(Vector3(-1.0f, -1.0f, -5.0f), Vector3(1.0f, 1.0f, 5.0f));
        const Rect clipped = straddling.Projected(projection);
        REQUIRE(clipped.Defined());
        REQUIRE_FALSE(clipped.Min().IsNaN());
        REQUIRE_FALSE(clipped.Max().IsNaN());
    }

    SECTION("a box entirely behind the near clip still yields a defined rect")
    {
        const BoundingBox behind(Vector3(-1.0f, -1.0f, -10.0f), Vector3(1.0f, 1.0f, -5.0f));
        REQUIRE(behind.Projected(projection).Defined());
    }
}

TEST_CASE("BoundingBox sphere containment reaches every axis on both sides", "[Math]")
{
    const BoundingBox box(-1.0f, 1.0f);

    REQUIRE(box.IsInside(Sphere(Vector3(-5.0f, 0.0f, 0.0f), 0.5f)) == OUTSIDE);
    REQUIRE(box.IsInside(Sphere(Vector3(5.0f, 0.0f, 0.0f), 0.5f)) == OUTSIDE);
    REQUIRE(box.IsInside(Sphere(Vector3(0.0f, -5.0f, 0.0f), 0.5f)) == OUTSIDE);
    REQUIRE(box.IsInside(Sphere(Vector3(0.0f, 5.0f, 0.0f), 0.5f)) == OUTSIDE);
    REQUIRE(box.IsInside(Sphere(Vector3(0.0f, 0.0f, -5.0f), 0.5f)) == OUTSIDE);
    REQUIRE(box.IsInside(Sphere(Vector3(0.0f, 0.0f, 5.0f), 0.5f)) == OUTSIDE);

    REQUIRE(box.IsInsideFast(Sphere(Vector3(-5.0f, 0.0f, 0.0f), 0.5f)) == OUTSIDE);
    REQUIRE(box.IsInsideFast(Sphere(Vector3(5.0f, 0.0f, 0.0f), 0.5f)) == OUTSIDE);
    REQUIRE(box.IsInsideFast(Sphere(Vector3(0.0f, -5.0f, 0.0f), 0.5f)) == OUTSIDE);
    REQUIRE(box.IsInsideFast(Sphere(Vector3(0.0f, 5.0f, 0.0f), 0.5f)) == OUTSIDE);
    REQUIRE(box.IsInsideFast(Sphere(Vector3(0.0f, 0.0f, -5.0f), 0.5f)) == OUTSIDE);
    REQUIRE(box.IsInsideFast(Sphere(Vector3(0.0f, 0.0f, 5.0f), 0.5f)) == OUTSIDE);

    SECTION("a sphere reaching past a face intersects rather than being contained")
    {
        REQUIRE(box.IsInside(Sphere(Vector3(0.9f, 0.0f, 0.0f), 0.5f)) == INTERSECTS);
        REQUIRE(box.IsInside(Sphere(Vector3(-0.9f, 0.0f, 0.0f), 0.5f)) == INTERSECTS);
        REQUIRE(box.IsInside(Sphere(Vector3(0.0f, 0.9f, 0.0f), 0.5f)) == INTERSECTS);
        REQUIRE(box.IsInside(Sphere(Vector3(0.0f, -0.9f, 0.0f), 0.5f)) == INTERSECTS);
        REQUIRE(box.IsInside(Sphere(Vector3(0.0f, 0.0f, 0.9f), 0.5f)) == INTERSECTS);
        REQUIRE(box.IsInside(Sphere(Vector3(0.0f, 0.0f, -0.9f), 0.5f)) == INTERSECTS);
    }

    SECTION("a fully enclosed sphere is inside")
    {
        REQUIRE(box.IsInside(Sphere(Vector3::ZERO, 0.25f)) == INSIDE);
        REQUIRE(box.IsInsideFast(Sphere(Vector3::ZERO, 0.25f)) == INSIDE);
    }
}

TEST_CASE("BoundingBox fast sphere test uses squared non-unit distances", "[Math]")
{
    const BoundingBox box(Vector3::ZERO, Vector3::ONE);
    REQUIRE(box.IsInsideFast(Sphere(Vector3(4.0f, 1.0f, 1.0f), 2.5f)) == OUTSIDE);
    REQUIRE(box.IsInsideFast(Sphere(Vector3(1.0f, -3.0f, 1.0f), 2.5f)) == OUTSIDE);
    REQUIRE(box.IsInsideFast(Sphere(Vector3(4.0f, 5.0f, 1.0f), 5.1f)) != OUTSIDE);
}

TEST_CASE("BoundingBox Clip trims each face independently", "[Math]")
{
    BoundingBox maxX(-10.0f, 10.0f);
    maxX.Clip(BoundingBox(Vector3(-20.0f, -20.0f, -20.0f), Vector3(5.0f, 20.0f, 20.0f)));
    REQUIRE_NEAR(maxX.max_.x_, 5.0f, 0.001f);

    BoundingBox maxY(-10.0f, 10.0f);
    maxY.Clip(BoundingBox(Vector3(-20.0f, -20.0f, -20.0f), Vector3(20.0f, 5.0f, 20.0f)));
    REQUIRE_NEAR(maxY.max_.y_, 5.0f, 0.001f);

    BoundingBox maxZ(-10.0f, 10.0f);
    maxZ.Clip(BoundingBox(Vector3(-20.0f, -20.0f, -20.0f), Vector3(20.0f, 20.0f, 5.0f)));
    REQUIRE_NEAR(maxZ.max_.z_, 5.0f, 0.001f);

    SECTION("clipping against a disjoint box collapses to the undefined state")
    {
        BoundingBox disjoint(-1.0f, 1.0f);
        disjoint.Clip(BoundingBox(10.0f, 20.0f));
        REQUIRE_FALSE(disjoint.Defined());
    }
}

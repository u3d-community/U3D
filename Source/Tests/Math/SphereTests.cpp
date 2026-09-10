#include "TestUtils.h"

#include <Urho3D/Math/BoundingBox.h>
#include <Urho3D/Math/Frustum.h>
#include <Urho3D/Math/Polyhedron.h>
#include <Urho3D/Math/Sphere.h>

using namespace Urho3D;

TEST_CASE("Sphere default construction is undefined", "[Math]")
{
    const Sphere sphere;
    REQUIRE_FALSE(sphere.Defined());
    REQUIRE(sphere.center_ == Vector3::ZERO);

    REQUIRE(Sphere(Vector3::ZERO, 1.0f).Defined());
}

TEST_CASE("Sphere construction and assignment", "[Math]")
{
    const Sphere sphere(Vector3(1.0f, 2.0f, 3.0f), 4.0f);
    REQUIRE(sphere.center_ == Vector3(1.0f, 2.0f, 3.0f));
    REQUIRE(sphere.radius_ == 4.0f);

    const Sphere copy(sphere);
    REQUIRE(copy == sphere);

    Sphere assigned;
    assigned = sphere;
    REQUIRE(assigned == sphere);
    REQUIRE(assigned != Sphere(Vector3::ZERO, 1.0f));

    SECTION("from a vertex array the sphere contains every vertex")
    {
        const Vector3 vertices[] = {
            Vector3(-1.0f, 0.0f, 0.0f),
            Vector3(1.0f, 0.0f, 0.0f),
            Vector3(0.0f, 2.0f, 0.0f),
        };
        const Sphere fromVertices(vertices, 3);
        REQUIRE(fromVertices.Defined());
        for (const Vector3& vertex : vertices)
            REQUIRE((vertex - fromVertices.center_).Length() <= fromVertices.radius_ + M_LARGE_EPSILON);
    }

    SECTION("from a box the sphere contains every corner")
    {
        const BoundingBox box(-1.0f, 1.0f);
        const Sphere fromBox(box);
        REQUIRE(fromBox.Defined());

        for (int corner = 0; corner < 8; ++corner)
        {
            const Vector3 point(
                (corner & 1) ? box.max_.x_ : box.min_.x_,
                (corner & 2) ? box.max_.y_ : box.min_.y_,
                (corner & 4) ? box.max_.z_ : box.min_.z_);
            REQUIRE((point - fromBox.center_).Length() <= fromBox.radius_ + M_LARGE_EPSILON);
        }
    }
}

TEST_CASE("Sphere Define, Merge and Clear", "[Math]")
{
    Sphere sphere;
    sphere.Define(Vector3::ZERO, 1.0f);
    REQUIRE(sphere.Defined());

    SECTION("merging a contained point changes nothing")
    {
        const Sphere before = sphere;
        sphere.Merge(Vector3(0.5f, 0.0f, 0.0f));
        REQUIRE(sphere == before);
    }

    SECTION("merging an outside point grows the sphere to contain it")
    {
        const Vector3 point(5.0f, 0.0f, 0.0f);
        sphere.Merge(point);
        REQUIRE((point - sphere.center_).Length() <= sphere.radius_ + M_LARGE_EPSILON);
        REQUIRE((Vector3(-1.0f, 0.0f, 0.0f) - sphere.center_).Length() <= sphere.radius_ + M_LARGE_EPSILON);
    }

    SECTION("merging into an undefined sphere adopts the point")
    {
        Sphere empty;
        empty.Merge(Vector3(1.0f, 2.0f, 3.0f));
        REQUIRE(empty.Defined());
        REQUIRE(empty.center_ == Vector3(1.0f, 2.0f, 3.0f));
    }

    SECTION("merging a box contains its corners")
    {
        sphere.Merge(BoundingBox(Vector3(4.0f, 4.0f, 4.0f), Vector3(6.0f, 6.0f, 6.0f)));
        REQUIRE((Vector3(6.0f, 6.0f, 6.0f) - sphere.center_).Length() <= sphere.radius_ + M_LARGE_EPSILON);
    }

    SECTION("merging another sphere contains it")
    {
        sphere.Merge(Sphere(Vector3(10.0f, 0.0f, 0.0f), 2.0f));
        REQUIRE((Vector3(12.0f, 0.0f, 0.0f) - sphere.center_).Length() <= sphere.radius_ + M_LARGE_EPSILON);
    }

    SECTION("merging a fully contained sphere is a no-op")
    {
        Sphere big(Vector3::ZERO, 10.0f);
        const Sphere before = big;
        big.Merge(Sphere(Vector3(1.0f, 0.0f, 0.0f), 2.0f));
        REQUIRE(big == before);
    }

    SECTION("merging a sphere that swallows this one adopts it wholesale")
    {
        Sphere small(Vector3(1.0f, 0.0f, 0.0f), 1.0f);
        const Sphere big(Vector3::ZERO, 10.0f);
        small.Merge(big);
        REQUIRE(small == big);
    }

    SECTION("merging into an undefined sphere adopts the other sphere")
    {
        Sphere empty;
        const Sphere other(Vector3(1.0f, 2.0f, 3.0f), 4.0f);
        empty.Merge(other);
        REQUIRE(empty == other);
    }

    SECTION("two partially overlapping spheres grow to cover both")
    {
        Sphere a(Vector3(-2.0f, 0.0f, 0.0f), 2.0f);
        a.Merge(Sphere(Vector3(2.0f, 0.0f, 0.0f), 2.0f));
        REQUIRE((Vector3(-4.0f, 0.0f, 0.0f) - a.center_).Length() <= a.radius_ + M_LARGE_EPSILON);
        REQUIRE((Vector3(4.0f, 0.0f, 0.0f) - a.center_).Length() <= a.radius_ + M_LARGE_EPSILON);
    }

    SECTION("Clear returns to the undefined state")
    {
        sphere.Clear();
        REQUIRE_FALSE(sphere.Defined());
    }

    SECTION("Define from another sphere copies it")
    {
        Sphere target;
        target.Define(Sphere(Vector3::ONE, 3.0f));
        REQUIRE(target == Sphere(Vector3::ONE, 3.0f));
    }
}

TEST_CASE("Sphere containment", "[Math]")
{
    const Sphere sphere(Vector3::ZERO, 2.0f);

    REQUIRE(sphere.IsInside(Vector3::ZERO) == INSIDE);
    REQUIRE(sphere.IsInside(Vector3(1.0f, 0.0f, 0.0f)) == INSIDE);
    REQUIRE(sphere.IsInside(Vector3(5.0f, 0.0f, 0.0f)) == OUTSIDE);

    SECTION("against other spheres")
    {
        REQUIRE(sphere.IsInside(Sphere(Vector3::ZERO, 0.5f)) == INSIDE);
        REQUIRE(sphere.IsInside(Sphere(Vector3(2.0f, 0.0f, 0.0f), 1.0f)) == INTERSECTS);
        REQUIRE(sphere.IsInside(Sphere(Vector3(10.0f, 0.0f, 0.0f), 1.0f)) == OUTSIDE);

        REQUIRE(sphere.IsInsideFast(Sphere(Vector3::ZERO, 0.5f)) == INSIDE);
        REQUIRE(sphere.IsInsideFast(Sphere(Vector3(10.0f, 0.0f, 0.0f), 1.0f)) == OUTSIDE);
    }

    SECTION("against boxes")
    {
        REQUIRE(sphere.IsInside(BoundingBox(-0.5f, 0.5f)) == INSIDE);
        REQUIRE(sphere.IsInside(BoundingBox(10.0f, 11.0f)) == OUTSIDE);
        REQUIRE(sphere.IsInsideFast(BoundingBox(10.0f, 11.0f)) == OUTSIDE);
        REQUIRE(sphere.IsInsideFast(BoundingBox(-0.5f, 0.5f)) != OUTSIDE);
    }

    SECTION("a box just inside the radius is distinguished from one just outside")
    {
        const BoundingBox near(Vector3(1.5f, -0.1f, -0.1f), Vector3(3.0f, 0.1f, 0.1f));
        const BoundingBox far(Vector3(2.5f, -0.1f, -0.1f), Vector3(3.0f, 0.1f, 0.1f));

        REQUIRE(sphere.IsInsideFast(near) == INSIDE);
        REQUIRE(sphere.IsInsideFast(far) == OUTSIDE);
        REQUIRE(sphere.IsInside(near) == INTERSECTS);
        REQUIRE(sphere.IsInside(far) == OUTSIDE);
    }

    SECTION("a box straddling the sphere centre is reported as intersecting")
    {
        REQUIRE(sphere.IsInside(BoundingBox(Vector3(-5.0f, -0.1f, -0.1f), Vector3(5.0f, 0.1f, 0.1f))) == INTERSECTS);
        REQUIRE(sphere.IsInsideFast(BoundingBox(Vector3(-5.0f, -0.1f, -0.1f), Vector3(5.0f, 0.1f, 0.1f))) == INSIDE);
    }
}

TEST_CASE("Sphere Distance", "[Math]")
{
    const Sphere sphere(Vector3::ZERO, 2.0f);

    REQUIRE_EQ_F(sphere.Distance(Vector3::ZERO), 0.0f);
    REQUIRE_EQ_F(sphere.Distance(Vector3(1.0f, 0.0f, 0.0f)), 0.0f);
    REQUIRE_NEAR(sphere.Distance(Vector3(5.0f, 0.0f, 0.0f)), 3.0f, 0.001f);
}

TEST_CASE("Sphere surface points", "[Math]")
{
    const Sphere sphere(Vector3(1.0f, 2.0f, 3.0f), 5.0f);

    for (float theta = 0.0f; theta <= 180.0f; theta += 45.0f)
    {
        for (float phi = 0.0f; phi < 360.0f; phi += 45.0f)
        {
            const Vector3 point = sphere.GetPoint(theta, phi);
            REQUIRE_NEAR((point - sphere.center_).Length(), sphere.radius_, 0.001f);
            REQUIRE(sphere.GetLocalPoint(theta, phi).Equals(point - sphere.center_));
        }
    }
}

TEST_CASE("Sphere from frustum and polyhedron", "[Math]")
{
    Frustum frustum;
    frustum.Define(45.0f, 1.0f, 1.0f, 1.0f, 10.0f);

    const Sphere fromFrustum(frustum);
    REQUIRE(fromFrustum.Defined());
    for (const Vector3& vertex : frustum.vertices_)
        REQUIRE((vertex - fromFrustum.center_).Length() <= fromFrustum.radius_ + M_LARGE_EPSILON);

    const Polyhedron poly(BoundingBox(-2.0f, 2.0f));
    const Sphere fromPoly(poly);
    REQUIRE(fromPoly.Defined());

    SECTION("merging a frustum contains all of its vertices")
    {
        Sphere sphere(Vector3::ZERO, 0.1f);
        sphere.Merge(frustum);
        for (const Vector3& vertex : frustum.vertices_)
            REQUIRE((vertex - sphere.center_).Length() <= sphere.radius_ + M_LARGE_EPSILON);
    }

    SECTION("merging a polyhedron grows the sphere")
    {
        Sphere sphere(Vector3::ZERO, 0.1f);
        sphere.Merge(poly);
        REQUIRE(sphere.radius_ > 0.1f);
    }
}

TEST_CASE("Sphere Define from an empty vertex array leaves it undefined", "[Math]")
{
    const Vector3 vertices[] = {Vector3::ZERO};

    Sphere sphere;
    sphere.Define(vertices, 0);
    REQUIRE_FALSE(sphere.Defined());

    Sphere existing(Vector3::ONE, 5.0f);
    existing.Define(vertices, 0);
    REQUIRE(existing == Sphere(Vector3::ONE, 5.0f));

    Sphere merged(Vector3::ZERO, 1.0f);
    merged.Merge(vertices, 0);
    REQUIRE(merged == Sphere(Vector3::ZERO, 1.0f));
}

TEST_CASE("Sphere box containment reaches every axis on both sides", "[Math]")
{
    const Sphere sphere(Vector3::ZERO, 2.0f);

    const BoundingBox unit(-0.5f, 0.5f);

    REQUIRE(Sphere(Vector3(-5.0f, 0.0f, 0.0f), 1.0f).IsInside(unit) == OUTSIDE);
    REQUIRE(Sphere(Vector3(5.0f, 0.0f, 0.0f), 1.0f).IsInside(unit) == OUTSIDE);
    REQUIRE(Sphere(Vector3(0.0f, -5.0f, 0.0f), 1.0f).IsInside(unit) == OUTSIDE);
    REQUIRE(Sphere(Vector3(0.0f, 5.0f, 0.0f), 1.0f).IsInside(unit) == OUTSIDE);
    REQUIRE(Sphere(Vector3(0.0f, 0.0f, -5.0f), 1.0f).IsInside(unit) == OUTSIDE);
    REQUIRE(Sphere(Vector3(0.0f, 0.0f, 5.0f), 1.0f).IsInside(unit) == OUTSIDE);

    REQUIRE(Sphere(Vector3(-5.0f, 0.0f, 0.0f), 1.0f).IsInsideFast(unit) == OUTSIDE);
    REQUIRE(Sphere(Vector3(5.0f, 0.0f, 0.0f), 1.0f).IsInsideFast(unit) == OUTSIDE);
    REQUIRE(Sphere(Vector3(0.0f, -5.0f, 0.0f), 1.0f).IsInsideFast(unit) == OUTSIDE);
    REQUIRE(Sphere(Vector3(0.0f, 5.0f, 0.0f), 1.0f).IsInsideFast(unit) == OUTSIDE);
    REQUIRE(Sphere(Vector3(0.0f, 0.0f, -5.0f), 1.0f).IsInsideFast(unit) == OUTSIDE);
    REQUIRE(Sphere(Vector3(0.0f, 0.0f, 5.0f), 1.0f).IsInsideFast(unit) == OUTSIDE);

    SECTION("a box corner poking outside on each axis reports INTERSECTS")
    {
        REQUIRE(sphere.IsInside(BoundingBox(Vector3(-10.0f, -0.1f, -0.1f), Vector3(0.1f, 0.1f, 0.1f))) == INTERSECTS);
        REQUIRE(sphere.IsInside(BoundingBox(Vector3(-0.1f, -0.1f, -0.1f), Vector3(10.0f, 0.1f, 0.1f))) == INTERSECTS);
        REQUIRE(sphere.IsInside(BoundingBox(Vector3(-0.1f, -10.0f, -0.1f), Vector3(0.1f, 0.1f, 0.1f))) == INTERSECTS);
        REQUIRE(sphere.IsInside(BoundingBox(Vector3(-0.1f, -0.1f, -0.1f), Vector3(0.1f, 10.0f, 0.1f))) == INTERSECTS);
        REQUIRE(sphere.IsInside(BoundingBox(Vector3(-0.1f, -0.1f, -10.0f), Vector3(0.1f, 0.1f, 0.1f))) == INTERSECTS);
        REQUIRE(sphere.IsInside(BoundingBox(Vector3(-0.1f, -0.1f, -0.1f), Vector3(0.1f, 0.1f, 10.0f))) == INTERSECTS);
    }
}

TEST_CASE("Sphere box containment walks every corner", "[Math]")
{
    const Sphere sphere(Vector3::ZERO, Sqrt(14.0f));

    SECTION("the fourth corner")
    {
        const BoundingBox box(Vector3(-3.0f, -1.0f, -0.1f), Vector3(1.0f, 3.0f, 0.1f));
        REQUIRE(sphere.IsInside(box) == INTERSECTS);
    }

    const Sphere wider(Vector3::ZERO, Sqrt(23.0f));

    SECTION("the sixth corner")
    {
        const BoundingBox box(Vector3(-3.0f, -3.0f, -1.0f), Vector3(1.0f, 1.0f, 3.0f));
        REQUIRE(wider.IsInside(box) == INTERSECTS);
    }

    SECTION("the seventh corner")
    {
        const BoundingBox box(Vector3(-1.0f, -3.0f, -1.0f), Vector3(3.0f, 1.0f, 3.0f));
        REQUIRE(wider.IsInside(box) == INTERSECTS);
    }

    SECTION("the eighth corner")
    {
        const BoundingBox box(Vector3(-1.0f, -1.0f, -1.0f), Vector3(3.0f, 3.0f, 3.0f));
        REQUIRE(wider.IsInside(box) == INTERSECTS);
    }

    SECTION("a box whose every corner is within the radius is fully inside")
    {
        REQUIRE(wider.IsInside(BoundingBox(-1.0f, 1.0f)) == INSIDE);
    }
}

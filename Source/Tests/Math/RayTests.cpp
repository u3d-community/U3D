#include "TestUtils.h"

#include <Urho3D/Math/BoundingBox.h>
#include <Urho3D/Math/Frustum.h>
#include <Urho3D/Math/Matrix3x4.h>
#include <Urho3D/Math/Plane.h>
#include <Urho3D/Math/Ray.h>
#include <Urho3D/Math/Sphere.h>

using namespace Urho3D;

namespace
{

const Vector3 kQuad[] = {
    Vector3(-1.0f, -1.0f, 5.0f), Vector3(1.0f, 1.0f, 5.0f), Vector3(1.0f, -1.0f, 5.0f),
    Vector3(-1.0f, -1.0f, 5.0f), Vector3(-1.0f, 1.0f, 5.0f), Vector3(1.0f, 1.0f, 5.0f),
};

const Vector3 kQuadProbeOrigin(0.4f, -0.4f, 0.0f);

PODVector<Vector3> MakeCube()
{
    const Vector3 corners[8] = {
        Vector3(-1.0f, -1.0f, -1.0f), Vector3(1.0f, -1.0f, -1.0f),
        Vector3(1.0f, 1.0f, -1.0f), Vector3(-1.0f, 1.0f, -1.0f),
        Vector3(-1.0f, -1.0f, 1.0f), Vector3(1.0f, -1.0f, 1.0f),
        Vector3(1.0f, 1.0f, 1.0f), Vector3(-1.0f, 1.0f, 1.0f),
    };
    const int quads[6][4] = {
        {0, 3, 2, 1}, {4, 5, 6, 7}, {0, 4, 7, 3}, {1, 2, 6, 5}, {0, 1, 5, 4}, {3, 7, 6, 2},
    };

    PODVector<Vector3> vertices;
    for (const auto& quad : quads)
    {
        vertices.Push(corners[quad[0]]);
        vertices.Push(corners[quad[1]]);
        vertices.Push(corners[quad[2]]);
        vertices.Push(corners[quad[0]]);
        vertices.Push(corners[quad[2]]);
        vertices.Push(corners[quad[3]]);
    }
    return vertices;
}

}

TEST_CASE("Ray construction normalises the direction", "[Math]")
{
    const Ray ray(Vector3::ZERO, Vector3(0.0f, 0.0f, 10.0f));
    REQUIRE(ray.origin_ == Vector3::ZERO);
    REQUIRE(ray.direction_.Equals(Vector3::FORWARD));
    REQUIRE_NEAR(ray.direction_.Length(), 1.0f, 0.001f);

    const Ray copy(ray);
    REQUIRE(copy == ray);

    Ray assigned;
    assigned = ray;
    REQUIRE(assigned == ray);
    REQUIRE(assigned != Ray(Vector3::ONE, Vector3::UP));

    Ray defined;
    defined.Define(Vector3(1.0f, 2.0f, 3.0f), Vector3::RIGHT);
    REQUIRE(defined.origin_ == Vector3(1.0f, 2.0f, 3.0f));
    REQUIRE(defined.direction_.Equals(Vector3::RIGHT));
}

TEST_CASE("Ray Project and Distance", "[Math]")
{
    const Ray ray(Vector3::ZERO, Vector3::FORWARD);

    REQUIRE(ray.Project(Vector3(0.0f, 0.0f, 5.0f)).Equals(Vector3(0.0f, 0.0f, 5.0f)));
    REQUIRE(ray.Project(Vector3(3.0f, 4.0f, 5.0f)).Equals(Vector3(0.0f, 0.0f, 5.0f)));

    REQUIRE_NEAR(ray.Distance(Vector3(0.0f, 0.0f, 5.0f)), 0.0f, 0.001f);
    REQUIRE_NEAR(ray.Distance(Vector3(3.0f, 4.0f, 5.0f)), 5.0f, 0.001f);

    SECTION("points behind the origin project to a negative parameter")
    {
        REQUIRE(ray.Project(Vector3(0.0f, 0.0f, -5.0f)).z_ < 0.0f);
    }
}

TEST_CASE("Ray ClosestPoint between two rays", "[Math]")
{
    const Ray a(Vector3::ZERO, Vector3::FORWARD);
    const Ray b(Vector3(0.0f, 1.0f, 5.0f), Vector3::RIGHT);

    REQUIRE(a.ClosestPoint(b).Equals(Vector3(0.0f, 0.0f, 5.0f)));

    SECTION("parallel rays return the origin")
    {
        const Ray parallel(Vector3(1.0f, 0.0f, 0.0f), Vector3::FORWARD);
        REQUIRE(a.ClosestPoint(parallel).Equals(a.origin_));
    }
}

TEST_CASE("Ray against a plane", "[Math]")
{
    const Ray ray(Vector3::ZERO, Vector3::FORWARD);
    const Plane plane(-Vector3::FORWARD, Vector3(0.0f, 0.0f, 5.0f));

    REQUIRE_NEAR(ray.HitDistance(plane), 5.0f, 0.001f);

    SECTION("a plane behind the ray does not register a hit")
    {
        const Plane behind(Vector3::FORWARD, Vector3(0.0f, 0.0f, -5.0f));
        REQUIRE(ray.HitDistance(behind) == M_INFINITY);
    }

    SECTION("a parallel plane does not register a hit")
    {
        const Plane parallel(Vector3::UP, Vector3(0.0f, 5.0f, 0.0f));
        REQUIRE(ray.HitDistance(parallel) == M_INFINITY);
    }
}

TEST_CASE("Ray against a bounding box", "[Math]")
{
    const Ray ray(Vector3::ZERO, Vector3::FORWARD);

    REQUIRE_NEAR(ray.HitDistance(BoundingBox(Vector3(-1.0f, -1.0f, 4.0f), Vector3(1.0f, 1.0f, 6.0f))), 4.0f, 0.001f);

    SECTION("a ray starting inside the box hits at zero distance")
    {
        REQUIRE_NEAR(ray.HitDistance(BoundingBox(-1.0f, 1.0f)), 0.0f, 0.001f);
    }

    SECTION("a box behind the ray is missed")
    {
        REQUIRE(ray.HitDistance(BoundingBox(Vector3(-1.0f, -1.0f, -6.0f), Vector3(1.0f, 1.0f, -4.0f))) == M_INFINITY);
    }

    SECTION("a box off to the side is missed")
    {
        REQUIRE(ray.HitDistance(BoundingBox(Vector3(10.0f, 10.0f, 4.0f), Vector3(11.0f, 11.0f, 6.0f))) == M_INFINITY);
    }

    SECTION("every face slab is exercised from both directions")
    {
        const BoundingBox target(-1.0f, 1.0f);

        REQUIRE_NEAR(Ray(Vector3(-5.0f, 0.0f, 0.0f), Vector3::RIGHT).HitDistance(target), 4.0f, 0.001f);
        REQUIRE_NEAR(Ray(Vector3(5.0f, 0.0f, 0.0f), -Vector3::RIGHT).HitDistance(target), 4.0f, 0.001f);
        REQUIRE_NEAR(Ray(Vector3(0.0f, -5.0f, 0.0f), Vector3::UP).HitDistance(target), 4.0f, 0.001f);
        REQUIRE_NEAR(Ray(Vector3(0.0f, 5.0f, 0.0f), -Vector3::UP).HitDistance(target), 4.0f, 0.001f);
        REQUIRE_NEAR(Ray(Vector3(0.0f, 0.0f, -5.0f), Vector3::FORWARD).HitDistance(target), 4.0f, 0.001f);
        REQUIRE_NEAR(Ray(Vector3(0.0f, 0.0f, 5.0f), -Vector3::FORWARD).HitDistance(target), 4.0f, 0.001f);
    }

    SECTION("an oblique approach still resolves the entry face correctly")
    {
        const BoundingBox target(-1.0f, 1.0f);

        const Ray throughPlusX(Vector3(5.0f, -1.5f, 0.0f), Vector3(-1.0f, 0.5f, 0.0f));
        REQUIRE_NEAR(throughPlusX.HitDistance(target), 4.472f, 0.01f);

        const Ray throughPlusZ(Vector3(0.0f, -1.5f, 5.0f), Vector3(0.0f, 0.5f, -1.0f));
        REQUIRE_NEAR(throughPlusZ.HitDistance(target), 4.472f, 0.01f);

        const Ray throughMinusX(Vector3(-5.0f, -1.5f, 0.0f), Vector3(1.0f, 0.5f, 0.0f));
        REQUIRE_NEAR(throughMinusX.HitDistance(target), 4.472f, 0.01f);

        const Ray throughMinusZ(Vector3(0.0f, -1.5f, -5.0f), Vector3(0.0f, 0.5f, 1.0f));
        REQUIRE_NEAR(throughMinusZ.HitDistance(target), 4.472f, 0.01f);
    }

    SECTION("a ray aimed at a face plane but outside the face bounds misses")
    {
        const BoundingBox target(-1.0f, 1.0f);
        REQUIRE(Ray(Vector3(5.0f, 10.0f, 0.0f), -Vector3::RIGHT).HitDistance(target) == M_INFINITY);
        REQUIRE(Ray(Vector3(5.0f, 0.0f, 10.0f), -Vector3::RIGHT).HitDistance(target) == M_INFINITY);
        REQUIRE(Ray(Vector3(0.0f, 0.0f, 5.0f), -Vector3::FORWARD).HitDistance(
            BoundingBox(Vector3(-1.0f, 10.0f, -1.0f), Vector3(1.0f, 11.0f, 1.0f))) == M_INFINITY);
    }
}

TEST_CASE("Ray against a sphere", "[Math]")
{
    const Ray ray(Vector3::ZERO, Vector3::FORWARD);

    REQUIRE_NEAR(ray.HitDistance(Sphere(Vector3(0.0f, 0.0f, 5.0f), 1.0f)), 4.0f, 0.001f);

    SECTION("a ray starting inside the sphere hits at zero distance")
    {
        REQUIRE_NEAR(ray.HitDistance(Sphere(Vector3::ZERO, 1.0f)), 0.0f, 0.001f);
    }

    SECTION("a sphere behind the ray yields a negative distance rather than infinity")
    {
        REQUIRE_NEAR(ray.HitDistance(Sphere(Vector3(0.0f, 0.0f, -5.0f), 1.0f)), -4.0f, 0.001f);
    }

    SECTION("a sphere the ray line misses entirely returns infinity")
    {
        REQUIRE(ray.HitDistance(Sphere(Vector3(10.0f, 0.0f, -5.0f), 1.0f)) == M_INFINITY);
    }

    SECTION("a sphere off to the side is missed")
    {
        REQUIRE(ray.HitDistance(Sphere(Vector3(10.0f, 0.0f, 5.0f), 1.0f)) == M_INFINITY);
    }

    SECTION("a grazing hit lands on the tangent point")
    {
        const Ray tangent(Vector3(1.0f, 0.0f, 0.0f), Vector3::FORWARD);
        REQUIRE_NEAR(tangent.HitDistance(Sphere(Vector3(0.0f, 0.0f, 5.0f), 1.0f)), 5.0f, 0.01f);
    }
}

TEST_CASE("Ray against a triangle", "[Math]")
{
    const Ray ray(Vector3::ZERO, Vector3::FORWARD);
    const Vector3 v0(-1.0f, -1.0f, 5.0f);
    const Vector3 v1(0.0f, 1.0f, 5.0f);
    const Vector3 v2(1.0f, -1.0f, 5.0f);

    REQUIRE_NEAR(ray.HitDistance(v0, v1, v2), 5.0f, 0.001f);

    SECTION("the test is single sided, so the reverse winding is culled")
    {
        REQUIRE(ray.HitDistance(v2, v1, v0) == M_INFINITY);
    }

    SECTION("the surface normal and barycentric coordinates are reported")
    {
        Vector3 normal;
        Vector3 bary;
        const float distance = ray.HitDistance(v0, v1, v2, &normal, &bary);
        REQUIRE_NEAR(distance, 5.0f, 0.001f);
        REQUIRE_NEAR(normal.Normalized().Abs().z_, 1.0f, 0.001f);
        REQUIRE_NEAR(bary.x_ + bary.y_ + bary.z_, 1.0f, 0.001f);
        REQUIRE(bary.x_ >= 0.0f);
        REQUIRE(bary.y_ >= 0.0f);
        REQUIRE(bary.z_ >= 0.0f);
    }

    SECTION("a ray missing the triangle returns infinity")
    {
        const Ray offset(Vector3(10.0f, 0.0f, 0.0f), Vector3::FORWARD);
        REQUIRE(offset.HitDistance(v0, v1, v2) == M_INFINITY);
    }

    SECTION("a triangle behind the ray is missed")
    {
        REQUIRE(ray.HitDistance(
            Vector3(-1.0f, -1.0f, -5.0f), Vector3(1.0f, -1.0f, -5.0f), Vector3(0.0f, 1.0f, -5.0f)) == M_INFINITY);
    }
}

TEST_CASE("Ray against raw vertex data", "[Math]")
{
    const Ray ray(kQuadProbeOrigin, Vector3::FORWARD);

    const float distance = ray.HitDistance(kQuad, sizeof(Vector3), 0, 6);
    REQUIRE_NEAR(distance, 5.0f, 0.001f);

    SECTION("the normal is reported")
    {
        Vector3 normal;
        ray.HitDistance(kQuad, sizeof(Vector3), 0, 6, &normal);
        REQUIRE_NEAR(normal.Normalized().Abs().z_, 1.0f, 0.001f);
    }

    SECTION("an indexed draw finds the same hit")
    {
        const unsigned short indices[] = {0, 1, 2, 3, 4, 5};
        REQUIRE_NEAR(ray.HitDistance(kQuad, sizeof(Vector3), indices, sizeof(unsigned short), 0, 6), 5.0f, 0.001f);
    }

    SECTION("32 bit indices work too")
    {
        const unsigned indices[] = {0, 1, 2, 3, 4, 5};
        REQUIRE_NEAR(ray.HitDistance(kQuad, sizeof(Vector3), indices, sizeof(unsigned), 0, 6), 5.0f, 0.001f);
    }

    SECTION("a ray to the side misses every triangle")
    {
        const Ray offset(Vector3(10.0f, 0.0f, 0.0f), Vector3::FORWARD);
        REQUIRE(offset.HitDistance(kQuad, sizeof(Vector3), 0, 6) == M_INFINITY);
    }

    SECTION("only the requested vertex range is considered")
    {
        REQUIRE(ray.HitDistance(kQuad, sizeof(Vector3), 3, 3) == M_INFINITY);
    }
}

TEST_CASE("Ray interpolates UV coordinates at the hit point", "[Math]")
{
    struct Vertex
    {
        Vector3 position_;
        Vector2 uv_;
    };

    const Vertex vertices[] = {
        {Vector3(-1.0f, -1.0f, 5.0f), Vector2(0.0f, 0.0f)},
        {Vector3(1.0f, 1.0f, 5.0f), Vector2(1.0f, 1.0f)},
        {Vector3(1.0f, -1.0f, 5.0f), Vector2(1.0f, 0.0f)},
        {Vector3(-1.0f, -1.0f, 5.0f), Vector2(0.0f, 0.0f)},
        {Vector3(-1.0f, 1.0f, 5.0f), Vector2(0.0f, 1.0f)},
        {Vector3(1.0f, 1.0f, 5.0f), Vector2(1.0f, 1.0f)},
    };
    const unsigned stride = sizeof(Vertex);
    const unsigned uvOffset = sizeof(Vector3);

    SECTION("the centre of the quad maps to the middle of the UV range")
    {
        const Ray ray(Vector3(0.1f, -0.1f, 0.0f), Vector3::FORWARD);
        Vector3 normal;
        Vector2 uv;
        const float distance = ray.HitDistance(vertices, stride, 0, 6, &normal, &uv, uvOffset);

        REQUIRE_NEAR(distance, 5.0f, 0.001f);
        REQUIRE_NEAR(uv.x_, 0.55f, 0.01f);
        REQUIRE_NEAR(uv.y_, 0.45f, 0.01f);
    }

    SECTION("a corner maps to the corner UV")
    {
        const Ray ray(Vector3(-0.98f, -0.98f, 0.0f), Vector3::FORWARD);
        Vector2 uv;
        REQUIRE_NEAR(ray.HitDistance(vertices, stride, 0, 6, nullptr, &uv, uvOffset), 5.0f, 0.001f);
        REQUIRE_NEAR(uv.x_, 0.01f, 0.01f);
        REQUIRE_NEAR(uv.y_, 0.01f, 0.01f);
    }

    SECTION("a hit on the second triangle uses that triangle's own vertices")
    {
        const Ray ray(Vector3(-0.5f, 0.5f, 0.0f), Vector3::FORWARD);
        Vector2 uv;
        REQUIRE_NEAR(ray.HitDistance(vertices, stride, 0, 6, nullptr, &uv, uvOffset), 5.0f, 0.001f);
        REQUIRE_NEAR(uv.x_, 0.25f, 0.01f);
        REQUIRE_NEAR(uv.y_, 0.75f, 0.01f);
    }

    SECTION("the indexed overload also resolves the second triangle")
    {
        const Ray ray(Vector3(-0.5f, 0.5f, 0.0f), Vector3::FORWARD);

        const unsigned short shortIndices[] = {0, 1, 2, 3, 4, 5};
        Vector2 shortUv;
        REQUIRE_NEAR(ray.HitDistance(
            vertices, stride, shortIndices, sizeof(unsigned short), 0, 6, nullptr, &shortUv, uvOffset), 5.0f, 0.001f);
        REQUIRE_NEAR(shortUv.x_, 0.25f, 0.01f);
        REQUIRE_NEAR(shortUv.y_, 0.75f, 0.01f);

        const unsigned longIndices[] = {0, 1, 2, 3, 4, 5};
        Vector2 longUv;
        REQUIRE_NEAR(ray.HitDistance(
            vertices, stride, longIndices, sizeof(unsigned), 0, 6, nullptr, &longUv, uvOffset), 5.0f, 0.001f);
        REQUIRE_NEAR(longUv.x_, 0.25f, 0.01f);
        REQUIRE_NEAR(longUv.y_, 0.75f, 0.01f);
    }

    SECTION("the 32 bit indexed overload interpolates the first triangle too")
    {
        const unsigned indices[] = {0, 1, 2, 3, 4, 5};
        const Ray ray(Vector3(0.1f, -0.1f, 0.0f), Vector3::FORWARD);
        Vector2 uv;
        REQUIRE_NEAR(ray.HitDistance(
            vertices, stride, indices, sizeof(unsigned), 0, 6, nullptr, &uv, uvOffset), 5.0f, 0.001f);
        REQUIRE_NEAR(uv.x_, 0.55f, 0.01f);
        REQUIRE_NEAR(uv.y_, 0.45f, 0.01f);
    }

    SECTION("a miss reports a zero UV")
    {
        const Ray ray(Vector3(10.0f, 10.0f, 0.0f), Vector3::FORWARD);
        Vector2 uv(9.0f, 9.0f);
        REQUIRE(ray.HitDistance(vertices, stride, 0, 6, nullptr, &uv, uvOffset) == M_INFINITY);
        REQUIRE(uv == Vector2::ZERO);
    }

    SECTION("the indexed overload interpolates the same UV")
    {
        const unsigned short indices[] = {0, 1, 2, 3, 4, 5};
        const Ray ray(Vector3(0.1f, -0.1f, 0.0f), Vector3::FORWARD);
        Vector2 uv;
        const float distance = ray.HitDistance(
            vertices, stride, indices, sizeof(unsigned short), 0, 6, nullptr, &uv, uvOffset);
        REQUIRE_NEAR(distance, 5.0f, 0.001f);
        REQUIRE_NEAR(uv.x_, 0.55f, 0.01f);
        REQUIRE_NEAR(uv.y_, 0.45f, 0.01f);
    }

    SECTION("the indexed overload reports a zero UV on a miss")
    {
        const unsigned indices[] = {0, 1, 2, 3, 4, 5};
        const Ray ray(Vector3(10.0f, 10.0f, 0.0f), Vector3::FORWARD);
        Vector2 uv(9.0f, 9.0f);
        REQUIRE(ray.HitDistance(vertices, stride, indices, sizeof(unsigned), 0, 6, nullptr, &uv, uvOffset)
            == M_INFINITY);
        REQUIRE(uv == Vector2::ZERO);
    }
}

TEST_CASE("Ray InsideGeometry", "[Math]")
{
    const PODVector<Vector3> cube = MakeCube();

    const Vector3 direction = Vector3(0.3f, 0.2f, 1.0f).Normalized();

    const Ray inside(Vector3(0.1f, -0.2f, 0.05f), direction);
    REQUIRE(inside.InsideGeometry(cube.Buffer(), sizeof(Vector3), 0, cube.Size()));

    const Ray outside(Vector3(0.1f, -0.2f, -10.0f), direction);
    REQUIRE_FALSE(outside.InsideGeometry(cube.Buffer(), sizeof(Vector3), 0, cube.Size()));

    SECTION("a ray that misses the geometry entirely is not inside it")
    {
        const Ray away(Vector3(50.0f, 50.0f, 50.0f), Vector3::UP);
        REQUIRE_FALSE(away.InsideGeometry(cube.Buffer(), sizeof(Vector3), 0, cube.Size()));
    }

    SECTION("the indexed overload agrees with the direct one")
    {
        PODVector<unsigned short> shortIndices;
        PODVector<unsigned> longIndices;
        for (unsigned i = 0; i < cube.Size(); ++i)
        {
            shortIndices.Push(static_cast<unsigned short>(i));
            longIndices.Push(i);
        }

        REQUIRE(inside.InsideGeometry(
            cube.Buffer(), sizeof(Vector3), shortIndices.Buffer(), sizeof(unsigned short), 0, cube.Size()));
        REQUIRE_FALSE(outside.InsideGeometry(
            cube.Buffer(), sizeof(Vector3), shortIndices.Buffer(), sizeof(unsigned short), 0, cube.Size()));

        REQUIRE(inside.InsideGeometry(
            cube.Buffer(), sizeof(Vector3), longIndices.Buffer(), sizeof(unsigned), 0, cube.Size()));
        REQUIRE_FALSE(outside.InsideGeometry(
            cube.Buffer(), sizeof(Vector3), longIndices.Buffer(), sizeof(unsigned), 0, cube.Size()));
    }

    SECTION("the indexed hit distance agrees with the direct one")
    {
        PODVector<unsigned> longIndices;
        for (unsigned i = 0; i < cube.Size(); ++i)
            longIndices.Push(i);

        const Ray probe(Vector3(0.1f, -0.2f, -10.0f), Vector3::FORWARD);
        REQUIRE_NEAR(
            probe.HitDistance(cube.Buffer(), sizeof(Vector3), longIndices.Buffer(), sizeof(unsigned), 0, cube.Size()),
            probe.HitDistance(cube.Buffer(), sizeof(Vector3), 0, cube.Size()), 0.001f);
    }
}

TEST_CASE("Ray against a frustum", "[Math]")
{
    Frustum frustum;
    frustum.Define(90.0f, 1.0f, 1.0f, 1.0f, 10.0f);

    const Ray fromOutside(Vector3(0.0f, 0.0f, -10.0f), Vector3::FORWARD);
    REQUIRE(fromOutside.HitDistance(frustum) < M_INFINITY);

    SECTION("a ray pointing away misses")
    {
        const Ray away(Vector3(0.0f, 0.0f, -10.0f), -Vector3::FORWARD);
        REQUIRE(away.HitDistance(frustum) == M_INFINITY);
    }

    SECTION("a ray starting inside hits at zero when the frustum counts as solid")
    {
        const Ray inside(Vector3(0.0f, 0.0f, 5.0f), Vector3::FORWARD);
        REQUIRE_NEAR(inside.HitDistance(frustum, true), 0.0f, 0.001f);
        REQUIRE(inside.HitDistance(frustum, false) > 0.0f);
    }
}

TEST_CASE("Ray Transformed", "[Math]")
{
    const Ray ray(Vector3::ZERO, Vector3::FORWARD);

    SECTION("translation moves the origin and keeps the direction")
    {
        const Matrix3x4 translation(Vector3(1.0f, 2.0f, 3.0f), Quaternion::IDENTITY, 1.0f);
        const Ray moved = ray.Transformed(translation);
        REQUIRE(moved.origin_.Equals(Vector3(1.0f, 2.0f, 3.0f)));
        REQUIRE(moved.direction_.Equals(Vector3::FORWARD));
    }

    SECTION("rotation turns the direction")
    {
        const Matrix3x4 rotation(Vector3::ZERO, Quaternion(90.0f, Vector3::UP), 1.0f);
        REQUIRE(ray.Transformed(rotation).direction_.Equals(Vector3::RIGHT));
    }

    SECTION("the direction is scaled rather than renormalised")
    {
        const Matrix3x4 scale(Vector3::ZERO, Quaternion::IDENTITY, 5.0f);
        REQUIRE_NEAR(ray.Transformed(scale).direction_.Length(), 5.0f, 0.001f);
    }
}

TEST_CASE("Ray against an undefined bounding box misses", "[Math]")
{
    const Ray ray(Vector3::ZERO, Vector3::FORWARD);
    REQUIRE(ray.HitDistance(BoundingBox()) == M_INFINITY);
}

TEST_CASE("Ray passing beside a frustum misses it", "[Math]")
{
    Frustum frustum;
    frustum.Define(45.0f, 1.0f, 1.0f, 1.0f, 10.0f);

    const Ray beside(Vector3(100.0f, 0.0f, -10.0f), Vector3::FORWARD);
    REQUIRE(beside.HitDistance(frustum) == M_INFINITY);

    const Ray away(Vector3(100.0f, 0.0f, 5.0f), Vector3::RIGHT);
    REQUIRE(away.HitDistance(frustum) == M_INFINITY);
}

TEST_CASE("Ray reports the normal through both indexed overloads", "[Math]")
{
    const Ray ray(kQuadProbeOrigin, Vector3::FORWARD);

    const unsigned short shortIndices[] = {0, 1, 2, 3, 4, 5};
    Vector3 shortNormal;
    REQUIRE_NEAR(ray.HitDistance(
        kQuad, sizeof(Vector3), shortIndices, sizeof(unsigned short), 0, 6, &shortNormal), 5.0f, 0.001f);
    REQUIRE_NEAR(shortNormal.Normalized().Abs().z_, 1.0f, 0.001f);

    const unsigned longIndices[] = {0, 1, 2, 3, 4, 5};
    Vector3 longNormal;
    REQUIRE_NEAR(ray.HitDistance(
        kQuad, sizeof(Vector3), longIndices, sizeof(unsigned), 0, 6, &longNormal), 5.0f, 0.001f);
    REQUIRE_NEAR(longNormal.Normalized().Abs().z_, 1.0f, 0.001f);
}

TEST_CASE("Ray reports a zero UV on a miss through the 16 bit indexed overload", "[Math]")
{
    struct Vertex
    {
        Vector3 position_;
        Vector2 uv_;
    };

    const Vertex vertices[] = {
        {Vector3(-1.0f, -1.0f, 5.0f), Vector2(0.0f, 0.0f)},
        {Vector3(1.0f, 1.0f, 5.0f), Vector2(1.0f, 1.0f)},
        {Vector3(1.0f, -1.0f, 5.0f), Vector2(1.0f, 0.0f)},
    };
    const unsigned short indices[] = {0, 1, 2};

    const Ray miss(Vector3(10.0f, 10.0f, 0.0f), Vector3::FORWARD);
    Vector2 uv(9.0f, 9.0f);
    REQUIRE(miss.HitDistance(
        vertices, sizeof(Vertex), indices, sizeof(unsigned short), 0, 3, nullptr, &uv, sizeof(Vector3))
        == M_INFINITY);
    REQUIRE(uv == Vector2::ZERO);
}

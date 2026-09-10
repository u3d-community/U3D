#include "TestUtils.h"

#include <Urho3D/Math/BoundingBox.h>
#include <Urho3D/Math/Frustum.h>
#include <Urho3D/Math/Matrix3x4.h>
#include <Urho3D/Math/Plane.h>
#include <Urho3D/Math/Polyhedron.h>

using namespace Urho3D;

namespace
{

unsigned CountVertices(const Polyhedron& polyhedron)
{
    unsigned total = 0;
    for (unsigned i = 0; i < polyhedron.faces_.Size(); ++i)
        total += polyhedron.faces_[i].Size();
    return total;
}

bool NearlyEqual(const Vector3& lhs, const Vector3& rhs, float eps = M_LARGE_EPSILON)
{
    return Abs(lhs.x_ - rhs.x_) <= eps && Abs(lhs.y_ - rhs.y_) <= eps && Abs(lhs.z_ - rhs.z_) <= eps;
}

BoundingBox BoundsOf(const Polyhedron& polyhedron)
{
    BoundingBox bounds;
    for (unsigned i = 0; i < polyhedron.faces_.Size(); ++i)
    {
        for (unsigned j = 0; j < polyhedron.faces_[i].Size(); ++j)
            bounds.Merge(polyhedron.faces_[i][j]);
    }
    return bounds;
}

}

TEST_CASE("Polyhedron default is empty", "[Math]")
{
    const Polyhedron polyhedron;
    REQUIRE(polyhedron.Empty());
    REQUIRE(polyhedron.faces_.Empty());
}

TEST_CASE("Polyhedron from a bounding box has six faces", "[Math]")
{
    const Polyhedron polyhedron(BoundingBox(-1.0f, 1.0f));

    REQUIRE_FALSE(polyhedron.Empty());
    REQUIRE(polyhedron.faces_.Size() == 6);
    REQUIRE(CountVertices(polyhedron) == 24);

    const BoundingBox bounds = BoundsOf(polyhedron);
    REQUIRE(bounds.min_.Equals(Vector3(-1.0f, -1.0f, -1.0f)));
    REQUIRE(bounds.max_.Equals(Vector3(1.0f, 1.0f, 1.0f)));

    SECTION("Define replaces the previous contents")
    {
        Polyhedron target(BoundingBox(-5.0f, 5.0f));
        target.Define(BoundingBox(-1.0f, 1.0f));
        REQUIRE(target.faces_.Size() == 6);
        REQUIRE(BoundsOf(target).max_.Equals(Vector3(1.0f, 1.0f, 1.0f)));
    }

    SECTION("Clear empties it")
    {
        Polyhedron target(BoundingBox(-1.0f, 1.0f));
        target.Clear();
        REQUIRE(target.Empty());
    }

    SECTION("copy and assignment preserve the faces")
    {
        const Polyhedron copy(polyhedron);
        REQUIRE(copy.faces_.Size() == 6);

        Polyhedron assigned;
        assigned = polyhedron;
        REQUIRE(assigned.faces_.Size() == 6);
    }
}

TEST_CASE("Polyhedron from a frustum has six faces", "[Math]")
{
    Frustum frustum;
    frustum.Define(90.0f, 1.0f, 1.0f, 1.0f, 10.0f);

    const Polyhedron polyhedron(frustum);
    REQUIRE(polyhedron.faces_.Size() == 6);

    const BoundingBox bounds = BoundsOf(polyhedron);
    REQUIRE_NEAR(bounds.min_.z_, 1.0f, 0.01f);
    REQUIRE_NEAR(bounds.max_.z_, 10.0f, 0.01f);

    SECTION("Define from a frustum replaces the contents")
    {
        Polyhedron target(BoundingBox(-100.0f, 100.0f));
        target.Define(frustum);
        REQUIRE(target.faces_.Size() == 6);
        REQUIRE(BoundsOf(target).max_.z_ < 20.0f);
    }
}

TEST_CASE("Polyhedron AddFace", "[Math]")
{
    Polyhedron polyhedron;

    polyhedron.AddFace(Vector3::ZERO, Vector3::RIGHT, Vector3::UP);
    REQUIRE(polyhedron.faces_.Size() == 1);
    REQUIRE(polyhedron.faces_[0].Size() == 3);

    polyhedron.AddFace(Vector3::ZERO, Vector3::RIGHT, Vector3::UP, Vector3::FORWARD);
    REQUIRE(polyhedron.faces_.Size() == 2);
    REQUIRE(polyhedron.faces_[1].Size() == 4);

    PODVector<Vector3> pentagon;
    for (int i = 0; i < 5; ++i)
        pentagon.Push(Vector3(static_cast<float>(i), 0.0f, 0.0f));
    polyhedron.AddFace(pentagon);
    REQUIRE(polyhedron.faces_.Size() == 3);
    REQUIRE(polyhedron.faces_[2].Size() == 5);

    SECTION("the stored vertices are the ones that were added")
    {
        REQUIRE(polyhedron.faces_[0][0].Equals(Vector3::ZERO));
        REQUIRE(polyhedron.faces_[0][1].Equals(Vector3::RIGHT));
        REQUIRE(polyhedron.faces_[2][4].Equals(Vector3(4.0f, 0.0f, 0.0f)));
    }
}

TEST_CASE("Polyhedron Clip against a plane", "[Math]")
{
    Polyhedron polyhedron(BoundingBox(-1.0f, 1.0f));

    polyhedron.Clip(Plane(Vector3::UP, Vector3::ZERO));

    REQUIRE_FALSE(polyhedron.Empty());
    const BoundingBox bounds = BoundsOf(polyhedron);
    REQUIRE(bounds.min_.y_ >= -M_LARGE_EPSILON);
    REQUIRE_NEAR(bounds.max_.y_, 1.0f, 0.01f);
    REQUIRE_NEAR(bounds.min_.x_, -1.0f, 0.01f);

    SECTION("clipping everything away empties the polyhedron")
    {
        Polyhedron target(BoundingBox(-1.0f, 1.0f));
        target.Clip(Plane(Vector3::UP, Vector3(0.0f, 10.0f, 0.0f)));
        REQUIRE(target.Empty());
    }

    SECTION("a plane that misses the volume leaves it intact")
    {
        Polyhedron target(BoundingBox(-1.0f, 1.0f));
        const unsigned before = CountVertices(target);
        target.Clip(Plane(Vector3::UP, Vector3(0.0f, -10.0f, 0.0f)));
        REQUIRE(CountVertices(target) == before);
    }
}

TEST_CASE("Polyhedron Clip against a box", "[Math]")
{
    Polyhedron polyhedron(BoundingBox(-2.0f, 2.0f));
    polyhedron.Clip(BoundingBox(-1.0f, 1.0f));

    REQUIRE_FALSE(polyhedron.Empty());
    const BoundingBox bounds = BoundsOf(polyhedron);
    REQUIRE(NearlyEqual(bounds.min_, Vector3(-1.0f, -1.0f, -1.0f)));
    REQUIRE(NearlyEqual(bounds.max_, Vector3(1.0f, 1.0f, 1.0f)));

    SECTION("clipping against a disjoint box empties it")
    {
        Polyhedron target(BoundingBox(-1.0f, 1.0f));
        target.Clip(BoundingBox(Vector3(10.0f, 10.0f, 10.0f), Vector3(20.0f, 20.0f, 20.0f)));
        REQUIRE(target.Empty());
    }
}

TEST_CASE("Polyhedron Clip against a frustum", "[Math]")
{
    Frustum frustum;
    frustum.Define(90.0f, 1.0f, 1.0f, 1.0f, 10.0f);

    Polyhedron polyhedron(BoundingBox(Vector3(-1.0f, -1.0f, 2.0f), Vector3(1.0f, 1.0f, 5.0f)));
    polyhedron.Clip(frustum);

    REQUIRE_FALSE(polyhedron.Empty());
    const BoundingBox bounds = BoundsOf(polyhedron);
    REQUIRE(bounds.min_.z_ >= 1.0f - M_LARGE_EPSILON);
    REQUIRE(bounds.max_.z_ <= 10.0f + M_LARGE_EPSILON);

    SECTION("a volume entirely behind the near plane is clipped away")
    {
        Polyhedron behind(BoundingBox(Vector3(-1.0f, -1.0f, -20.0f), Vector3(1.0f, 1.0f, -10.0f)));
        behind.Clip(frustum);
        REQUIRE(behind.Empty());
    }
}

TEST_CASE("Polyhedron Transform", "[Math]")
{
    const Polyhedron polyhedron(BoundingBox(-1.0f, 1.0f));

    SECTION("translation moves every vertex")
    {
        const Matrix3x4 translation(Vector3(10.0f, 0.0f, 0.0f), Quaternion::IDENTITY, 1.0f);
        const Polyhedron moved = polyhedron.Transformed(translation);

        const BoundingBox bounds = BoundsOf(moved);
        REQUIRE(bounds.min_.Equals(Vector3(9.0f, -1.0f, -1.0f)));
        REQUIRE(bounds.max_.Equals(Vector3(11.0f, 1.0f, 1.0f)));
    }

    SECTION("scale grows the volume")
    {
        const Matrix3x4 scale(Vector3::ZERO, Quaternion::IDENTITY, 3.0f);
        const BoundingBox bounds = BoundsOf(polyhedron.Transformed(scale));
        REQUIRE(bounds.max_.Equals(Vector3(3.0f, 3.0f, 3.0f)));
    }

    SECTION("in place Transform matches Transformed")
    {
        const Matrix3x4 transform(Vector3(1.0f, 2.0f, 3.0f), Quaternion(30.0f, Vector3::UP), 2.0f);
        Polyhedron inPlace = polyhedron;
        inPlace.Transform(transform);

        const BoundingBox expected = BoundsOf(polyhedron.Transformed(transform));
        const BoundingBox actual = BoundsOf(inPlace);
        REQUIRE(actual.min_.Equals(expected.min_));
        REQUIRE(actual.max_.Equals(expected.max_));
    }

    SECTION("a Matrix3 rotation matches the equivalent Matrix3x4")
    {
        const Quaternion rotation(45.0f, Vector3::UP);
        const BoundingBox viaMatrix3 = BoundsOf(polyhedron.Transformed(rotation.RotationMatrix()));
        const BoundingBox viaMatrix3x4 =
            BoundsOf(polyhedron.Transformed(Matrix3x4(Vector3::ZERO, rotation, 1.0f)));

        REQUIRE(viaMatrix3.min_.Equals(viaMatrix3x4.min_));
        REQUIRE(viaMatrix3.max_.Equals(viaMatrix3x4.max_));
    }
}

TEST_CASE("Polyhedron Transform by a Matrix3", "[Math]")
{
    const Polyhedron polyhedron(BoundingBox(-1.0f, 1.0f));
    const Quaternion rotation(45.0f, Vector3::UP);
    const Matrix3 matrix = rotation.RotationMatrix();

    Polyhedron inPlace = polyhedron;
    inPlace.Transform(matrix);

    const BoundingBox bounds = BoundsOf(inPlace);
    REQUIRE(bounds.Size().x_ > 2.0f);
    REQUIRE_NEAR(bounds.Size().y_, 2.0f, 0.001f);

    SECTION("it matches Transformed and the equivalent Matrix3x4")
    {
        const BoundingBox viaTransformed = BoundsOf(polyhedron.Transformed(matrix));
        REQUIRE(NearlyEqual(bounds.min_, viaTransformed.min_));
        REQUIRE(NearlyEqual(bounds.max_, viaTransformed.max_));

        const BoundingBox viaAffine = BoundsOf(polyhedron.Transformed(Matrix3x4(Vector3::ZERO, rotation, 1.0f)));
        REQUIRE(NearlyEqual(bounds.min_, viaAffine.min_));
    }
}

TEST_CASE("Polyhedron clipping down to a triangle", "[Math]")
{
    Polyhedron polyhedron(BoundingBox(-1.0f, 1.0f));
    polyhedron.Clip(Plane(Vector3(1.0f, 1.0f, 1.0f).Normalized(), Vector3(0.5f, 0.5f, 0.5f)));

    REQUIRE_FALSE(polyhedron.Empty());

    bool sawTriangle = false;
    for (unsigned i = 0; i < polyhedron.faces_.Size(); ++i)
    {
        if (polyhedron.faces_[i].Size() == 3)
            sawTriangle = true;
    }
    REQUIRE(sawTriangle);
}

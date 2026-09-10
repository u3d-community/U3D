#include "TestUtils.h"

#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/Drawable.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Graphics/OctreeQuery.h>
#include <Urho3D/Graphics/StaticModel.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/Scene.h>

using namespace Urho3D;
using namespace U3DTest;

namespace
{

struct OctreeFixture
{
    OctreeFixture() :
        scene_(new Scene(HeadlessContext()))
    {
        octree_ = scene_->CreateComponent<Octree>();
        model_ = HeadlessContext()->GetSubsystem<ResourceCache>()->GetResource<Model>("Models/Box.mdl");
    }

    StaticModel* AddBox(const Vector3& position)
    {
        Node* node = scene_->CreateChild();
        node->SetPosition(position);
        auto* drawable = node->CreateComponent<StaticModel>();
        drawable->SetModel(model_);
        return drawable;
    }

    void Settle()
    {
        FrameInfo frame{};
        octree_->Update(frame);
    }

    SharedPtr<Scene> scene_;
    Octree* octree_{};
    Model* model_{};
};

}

TEST_CASE("StaticModel exposes the bounds of its model", "[Graphics]")
{
    OctreeFixture fixture;
    REQUIRE(fixture.model_ != nullptr);

    StaticModel* drawable = fixture.AddBox(Vector3::ZERO);
    REQUIRE(drawable->GetModel() == fixture.model_);
    REQUIRE(drawable->GetNumGeometries() == fixture.model_->GetNumGeometries());

    const BoundingBox local = drawable->GetBoundingBox();
    REQUIRE(local.Size().x_ > 0.0f);
    REQUIRE(drawable->GetWorldBoundingBox().Center().Equals(Vector3::ZERO));

    SECTION("moving the node moves the world bounding box")
    {
        drawable->GetNode()->SetPosition(Vector3(10.0f, 0.0f, 0.0f));
        REQUIRE(drawable->GetWorldBoundingBox().Center().Equals(Vector3(10.0f, 0.0f, 0.0f)));
        REQUIRE(drawable->GetBoundingBox().Center().Equals(local.Center()));
    }

    SECTION("scaling the node scales the world bounding box only")
    {
        drawable->GetNode()->SetScale(4.0f);
        REQUIRE_NEAR(drawable->GetWorldBoundingBox().Size().x_, local.Size().x_ * 4.0f, 0.001f);
    }

    SECTION("clearing the model empties the geometry list")
    {
        drawable->SetModel(nullptr);
        REQUIRE(drawable->GetModel() == nullptr);
        REQUIRE(drawable->GetNumGeometries() == 0);
    }

    SECTION("drawable flags and masks round trip")
    {
        REQUIRE(drawable->GetDrawableFlags() == DRAWABLE_GEOMETRY);
        drawable->SetViewMask(0x3);
        drawable->SetLightMask(0x5);
        drawable->SetShadowMask(0x9);
        drawable->SetZoneMask(0x11);
        REQUIRE(drawable->GetViewMask() == 0x3);
        REQUIRE(drawable->GetLightMask() == 0x5);
        REQUIRE(drawable->GetShadowMask() == 0x9);
        REQUIRE(drawable->GetZoneMask() == 0x11);

        drawable->SetCastShadows(true);
        REQUIRE(drawable->GetCastShadows());
        drawable->SetOccluder(true);
        REQUIRE(drawable->IsOccluder());
        drawable->SetOccludee(false);
        REQUIRE_FALSE(drawable->IsOccludee());

        drawable->SetDrawDistance(50.0f);
        drawable->SetShadowDistance(25.0f);
        REQUIRE_EQ_F(drawable->GetDrawDistance(), 50.0f);
        REQUIRE_EQ_F(drawable->GetShadowDistance(), 25.0f);
    }
}

TEST_CASE("Octree finds drawables inside a volume", "[Graphics]")
{
    OctreeFixture fixture;
    StaticModel* origin = fixture.AddBox(Vector3::ZERO);
    StaticModel* far = fixture.AddBox(Vector3(100.0f, 0.0f, 0.0f));
    fixture.Settle();

    PODVector<Drawable*> result;

    BoxOctreeQuery nearBox(result, BoundingBox(Vector3(-2.0f, -2.0f, -2.0f), Vector3(2.0f, 2.0f, 2.0f)));
    fixture.octree_->GetDrawables(nearBox);
    REQUIRE(result.Size() == 1);
    REQUIRE(result[0] == origin);

    SphereOctreeQuery sphere(result, Sphere(Vector3(100.0f, 0.0f, 0.0f), 2.0f));
    fixture.octree_->GetDrawables(sphere);
    REQUIRE(result.Size() == 1);
    REQUIRE(result[0] == far);

    PointOctreeQuery point(result, Vector3::ZERO);
    fixture.octree_->GetDrawables(point);
    REQUIRE(result.Contains(origin));

    SECTION("a volume covering everything returns both drawables")
    {
        BoxOctreeQuery everything(result, BoundingBox(-1000.0f, 1000.0f));
        fixture.octree_->GetDrawables(everything);
        REQUIRE(result.Size() == 2);
    }

    SECTION("a volume covering nothing returns nothing")
    {
        SphereOctreeQuery empty(result, Sphere(Vector3(0.0f, 500.0f, 0.0f), 1.0f));
        fixture.octree_->GetDrawables(empty);
        REQUIRE(result.Empty());
    }

    SECTION("the view mask filters the result")
    {
        origin->SetViewMask(0x2);
        BoxOctreeQuery masked(result, BoundingBox(-1000.0f, 1000.0f), DRAWABLE_ANY, 0x1);
        fixture.octree_->GetDrawables(masked);
        REQUIRE_FALSE(result.Contains(origin));
        REQUIRE(result.Contains(far));
    }

    SECTION("the drawable flags filter the result")
    {
        BoxOctreeQuery lightsOnly(result, BoundingBox(-1000.0f, 1000.0f), DRAWABLE_LIGHT);
        fixture.octree_->GetDrawables(lightsOnly);
        REQUIRE(result.Empty());
    }

    SECTION("a camera frustum selects only what it can see")
    {
        Node* cameraNode = fixture.scene_->CreateChild("Camera");
        cameraNode->SetPosition(Vector3(0.0f, 0.0f, -10.0f));
        auto* camera = cameraNode->CreateComponent<Camera>();
        camera->SetAspectRatio(1.0f);
        camera->SetFarClip(50.0f);

        FrustumOctreeQuery visible(result, camera->GetFrustum());
        fixture.octree_->GetDrawables(visible);
        REQUIRE(result.Contains(origin));
        REQUIRE_FALSE(result.Contains(far));
    }

    SECTION("removing a drawable takes it out of the octree")
    {
        far->GetNode()->Remove();
        fixture.Settle();

        BoxOctreeQuery everything(result, BoundingBox(-1000.0f, 1000.0f));
        fixture.octree_->GetDrawables(everything);
        REQUIRE(result.Size() == 1);
        REQUIRE(result[0] == origin);
    }
}

TEST_CASE("Octree raycasts hit the nearest drawable", "[Graphics]")
{
    OctreeFixture fixture;
    StaticModel* near = fixture.AddBox(Vector3(0.0f, 0.0f, 10.0f));
    StaticModel* far = fixture.AddBox(Vector3(0.0f, 0.0f, 30.0f));
    fixture.Settle();

    PODVector<RayQueryResult> results;
    RayOctreeQuery query(results, Ray(Vector3::ZERO, Vector3::FORWARD), RAY_TRIANGLE, 100.0f);
    fixture.octree_->Raycast(query);

    REQUIRE(results.Size() == 2);
    REQUIRE(results[0].drawable_ == near);
    REQUIRE(results[1].drawable_ == far);
    REQUIRE(results[0].distance_ < results[1].distance_);
    REQUIRE(results[0].node_ == near->GetNode());

    SECTION("RaycastSingle stops at the first hit")
    {
        PODVector<RayQueryResult> single;
        RayOctreeQuery singleQuery(single, Ray(Vector3::ZERO, Vector3::FORWARD), RAY_TRIANGLE, 100.0f);
        fixture.octree_->RaycastSingle(singleQuery);
        REQUIRE(single.Size() == 1);
        REQUIRE(single[0].drawable_ == near);
    }

    SECTION("a ray pointing away hits nothing")
    {
        PODVector<RayQueryResult> missed;
        RayOctreeQuery away(missed, Ray(Vector3::ZERO, -Vector3::FORWARD), RAY_TRIANGLE, 100.0f);
        fixture.octree_->Raycast(away);
        REQUIRE(missed.Empty());
    }

    SECTION("a short ray only reaches the nearer box")
    {
        PODVector<RayQueryResult> shortHits;
        RayOctreeQuery shortRay(shortHits, Ray(Vector3::ZERO, Vector3::FORWARD), RAY_TRIANGLE, 15.0f);
        fixture.octree_->Raycast(shortRay);
        REQUIRE(shortHits.Size() == 1);
        REQUIRE(shortHits[0].drawable_ == near);
    }

    SECTION("the coarser query levels agree on which drawables are hit")
    {
        PODVector<RayQueryResult> boxHits;
        RayOctreeQuery boxLevel(boxHits, Ray(Vector3::ZERO, Vector3::FORWARD), RAY_AABB, 100.0f);
        fixture.octree_->Raycast(boxLevel);
        REQUIRE(boxHits.Size() == 2);

        PODVector<RayQueryResult> sphereHits;
        RayOctreeQuery sphereLevel(sphereHits, Ray(Vector3::ZERO, Vector3::FORWARD), RAY_OBB, 100.0f);
        fixture.octree_->Raycast(sphereLevel);
        REQUIRE(sphereHits.Size() == 2);
    }

    SECTION("the view mask filters raycast hits too")
    {
        near->SetViewMask(0x2);
        PODVector<RayQueryResult> masked;
        RayOctreeQuery maskedQuery(masked, Ray(Vector3::ZERO, Vector3::FORWARD), RAY_TRIANGLE, 100.0f,
            DRAWABLE_ANY, 0x1);
        fixture.octree_->Raycast(maskedQuery);
        REQUIRE(masked.Size() == 1);
        REQUIRE(masked[0].drawable_ == far);
    }
}

TEST_CASE("Octree resizes to hold what it is given", "[Graphics]")
{
    OctreeFixture fixture;

    fixture.octree_->SetSize(BoundingBox(-10.0f, 10.0f), 4);
    REQUIRE(fixture.octree_->GetNumLevels() == 4);
    REQUIRE(fixture.octree_->GetWorldBoundingBox().Size().x_ >= 20.0f);

    SECTION("the level count is clamped to a workable range")
    {
        fixture.octree_->SetSize(BoundingBox(-10.0f, 10.0f), 0);
        REQUIRE(fixture.octree_->GetNumLevels() >= 1);
    }

    SECTION("drawables outside the root box are still found")
    {
        fixture.octree_->SetSize(BoundingBox(-1.0f, 1.0f), 2);
        StaticModel* outside = fixture.AddBox(Vector3(500.0f, 0.0f, 0.0f));
        fixture.Settle();

        PODVector<Drawable*> result;
        BoxOctreeQuery everything(result, BoundingBox(-1000.0f, 1000.0f));
        fixture.octree_->GetDrawables(everything);
        REQUIRE(result.Contains(outside));
    }
}

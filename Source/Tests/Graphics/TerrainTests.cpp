#include "TestUtils.h"

#include <Urho3D/Graphics/Drawable.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/OctreeQuery.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Graphics/Terrain.h>
#include <Urho3D/Graphics/TerrainPatch.h>
#include <Urho3D/Resource/Image.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/Scene.h>

using namespace Urho3D;
using namespace U3DTest;

namespace
{

SharedPtr<Image> MakeRamp(Context* context, int size = 33)
{
    SharedPtr<Image> image(new Image(context));
    image->SetSize(size, size, 1);
    for (int y = 0; y < size; ++y)
    {
        for (int x = 0; x < size; ++x)
        {
            const float ramp = (float)x / (float)(size - 1);
            image->SetPixel(x, y, Color(ramp, ramp, ramp));
        }
    }
    return image;
}

struct TerrainFixture
{
    TerrainFixture() :
        scene_(new Scene(HeadlessContext()))
    {
        scene_->CreateComponent<Octree>();
        node_ = scene_->CreateChild("Terrain");
        terrain_ = node_->CreateComponent<Terrain>();
        terrain_->SetPatchSize(16);
        terrain_->SetSpacing(Vector3(1.0f, 0.25f, 1.0f));
        heightMap_ = MakeRamp(HeadlessContext());
        terrain_->SetHeightMap(heightMap_);
    }

    SharedPtr<Scene> scene_;
    Node* node_{};
    Terrain* terrain_{};
    SharedPtr<Image> heightMap_;
};

}

TEST_CASE("Terrain builds patches from a height map", "[Graphics]")
{
    TerrainFixture fixture;
    Terrain* terrain = fixture.terrain_;

    REQUIRE(terrain->GetHeightMap() == fixture.heightMap_);
    REQUIRE(terrain->GetPatchSize() == 16);
    REQUIRE(terrain->GetSpacing() == Vector3(1.0f, 0.25f, 1.0f));

    REQUIRE(terrain->GetNumVertices() == IntVector2(33, 33));
    REQUIRE(terrain->GetNumPatches() == IntVector2(2, 2));

    REQUIRE(terrain->GetPatch(0, 0) != nullptr);
    REQUIRE(terrain->GetPatch(1, 1) != nullptr);
    REQUIRE(terrain->GetPatch(2, 0) == nullptr);
    REQUIRE(terrain->GetPatch(0, -1) == nullptr);

    TerrainPatch* patch = terrain->GetPatch(0, 0);
    REQUIRE(patch->GetOwner() == terrain);
    REQUIRE(patch->GetCoordinates() == IntVector2(0, 0));
    REQUIRE(patch->GetWorldBoundingBox().Defined());
    REQUIRE(patch->GetGeometry() != nullptr);
    REQUIRE(patch->GetMaxLodGeometry() != nullptr);

    SECTION("the patch size is clamped to a workable power of two")
    {
        terrain->SetPatchSize(3);
        REQUIRE(terrain->GetPatchSize() >= 4);

        terrain->SetPatchSize(17);
        REQUIRE(terrain->GetPatchSize() % 2 == 0);
    }

    SECTION("the spacing is stored exactly as given, with no clamping")
    {
        const Vector3 before = terrain->GetSpacing();
        terrain->SetSpacing(Vector3::ZERO);
        REQUIRE(terrain->GetSpacing() == Vector3::ZERO);

        terrain->SetSpacing(before);
        REQUIRE(terrain->GetSpacing() == before);
    }

    SECTION("clearing the height map removes the patches")
    {
        terrain->SetHeightMap(nullptr);
        REQUIRE(terrain->GetHeightMap() == nullptr);
        REQUIRE(terrain->GetNumPatches() == IntVector2::ZERO);
        REQUIRE(terrain->GetPatch(0, 0) == nullptr);
    }

    SECTION("the lod level count is clamped")
    {
        terrain->SetMaxLodLevels(0);
        REQUIRE(terrain->GetMaxLodLevels() >= 1);
        terrain->SetMaxLodLevels(100);
        REQUIRE(terrain->GetMaxLodLevels() <= 32);
    }
}

TEST_CASE("Terrain samples its height and normal in world space", "[Graphics]")
{
    TerrainFixture fixture;
    Terrain* terrain = fixture.terrain_;

    const float low = terrain->GetHeight(Vector3(-16.0f, 0.0f, 0.0f));
    const float high = terrain->GetHeight(Vector3(16.0f, 0.0f, 0.0f));
    REQUIRE(high > low);
    REQUIRE_NEAR(high - low, 255.0f * 0.25f, 1.0f);

    SECTION("the height varies smoothly between samples")
    {
        float previous = terrain->GetHeight(Vector3(-16.0f, 0.0f, 0.0f));
        for (float x = -15.0f; x <= 16.0f; x += 1.0f)
        {
            const float current = terrain->GetHeight(Vector3(x, 0.0f, 0.0f));
            REQUIRE(current >= previous - 0.001f);
            previous = current;
        }
    }

    SECTION("the height does not change along z")
    {
        const float first = terrain->GetHeight(Vector3(0.0f, 0.0f, -8.0f));
        const float second = terrain->GetHeight(Vector3(0.0f, 0.0f, 8.0f));
        REQUIRE_NEAR(first, second, 0.001f);
    }

    SECTION("the normal leans away from the slope and stays unit length")
    {
        const Vector3 normal = terrain->GetNormal(Vector3::ZERO);
        REQUIRE_NEAR(normal.Length(), 1.0f, 0.01f);
        REQUIRE(normal.y_ > 0.0f);
        REQUIRE(normal.x_ < 0.0f);
    }

    SECTION("a point outside the terrain clamps to the edge")
    {
        const float far = terrain->GetHeight(Vector3(1000.0f, 0.0f, 0.0f));
        REQUIRE_NEAR(far, high, 0.001f);
    }

    SECTION("moving the node moves the sampled surface with it")
    {
        const float before = terrain->GetHeight(Vector3::ZERO);
        fixture.node_->SetPosition(Vector3(0.0f, 10.0f, 0.0f));
        REQUIRE_NEAR(terrain->GetHeight(Vector3::ZERO), before + 10.0f, 0.001f);
    }

    SECTION("world and height map coordinates convert both ways")
    {
        const IntVector2 index = terrain->WorldToHeightMap(Vector3(0.0f, 0.0f, 0.0f));
        REQUIRE(index.x_ >= 0);
        REQUIRE(index.x_ < terrain->GetNumVertices().x_);

        const Vector3 back = terrain->HeightMapToWorld(index);
        REQUIRE_NEAR(back.x_, 0.0f, terrain->GetSpacing().x_);
    }
}

TEST_CASE("Terrain reacts to a changed height map", "[Graphics]")
{
    TerrainFixture fixture;
    Terrain* terrain = fixture.terrain_;

    const float before = terrain->GetHeight(Vector3(8.0f, 0.0f, 0.0f));

    fixture.heightMap_->Clear(Color::BLACK);
    terrain->ApplyHeightMap();

    const float after = terrain->GetHeight(Vector3(8.0f, 0.0f, 0.0f));
    REQUIRE(after < before);
    REQUIRE_NEAR(after, 0.0f, 0.01f);

    SECTION("a taller spacing scales the surface up")
    {
        fixture.heightMap_ = MakeRamp(HeadlessContext());
        terrain->SetHeightMap(fixture.heightMap_);

        const float unitSpacing = terrain->GetHeight(Vector3(16.0f, 0.0f, 0.0f));
        terrain->SetSpacing(Vector3(1.0f, 0.5f, 1.0f));
        REQUIRE(terrain->GetHeight(Vector3(16.0f, 0.0f, 0.0f)) > unitSpacing);
    }

    SECTION("smoothing changes the sampled surface")
    {
        fixture.heightMap_ = MakeRamp(HeadlessContext());
        terrain->SetHeightMap(fixture.heightMap_);
        REQUIRE_FALSE(terrain->GetSmoothing());

        PODVector<float> sharp;
        for (int i = 0; i <= 32; ++i)
            sharp.Push(terrain->GetHeight(Vector3(-16.0f + i, 0.0f, 0.0f)));

        terrain->SetSmoothing(true);
        REQUIRE(terrain->GetSmoothing());

        bool changed = false;
        for (int i = 0; i <= 32; ++i)
            changed = changed || !Equals(terrain->GetHeight(Vector3(-16.0f + i, 0.0f, 0.0f)), sharp[i]);
        REQUIRE(changed);
    }
}

TEST_CASE("Terrain forwards drawable settings to its patches", "[Graphics]")
{
    TerrainFixture fixture;
    Terrain* terrain = fixture.terrain_;
    TerrainPatch* patch = terrain->GetPatch(0, 0);
    REQUIRE(patch != nullptr);

    terrain->SetViewMask(0x5);
    terrain->SetLightMask(0x9);
    terrain->SetShadowMask(0x11);
    terrain->SetZoneMask(0x21);
    REQUIRE(terrain->GetViewMask() == 0x5);
    REQUIRE(patch->GetViewMask() == 0x5);
    REQUIRE(patch->GetLightMask() == 0x9);
    REQUIRE(patch->GetShadowMask() == 0x11);
    REQUIRE(patch->GetZoneMask() == 0x21);

    terrain->SetCastShadows(true);
    REQUIRE(terrain->GetCastShadows());
    REQUIRE(patch->GetCastShadows());

    terrain->SetDrawDistance(120.0f);
    terrain->SetShadowDistance(60.0f);
    REQUIRE_EQ_F(terrain->GetDrawDistance(), 120.0f);
    REQUIRE_EQ_F(patch->GetDrawDistance(), 120.0f);
    REQUIRE_EQ_F(patch->GetShadowDistance(), 60.0f);

    terrain->SetOccluder(true);
    terrain->SetOccludee(false);
    REQUIRE(terrain->IsOccluder());
    REQUIRE_FALSE(terrain->IsOccludee());
    REQUIRE_FALSE(patch->IsOccludee());

    terrain->SetMaxLights(3);
    REQUIRE(terrain->GetMaxLights() == 3);
    REQUIRE(patch->GetMaxLights() == 3);

    SECTION("the material reaches the patches too")
    {
        auto* material = HeadlessContext()->GetSubsystem<ResourceCache>()
            ->GetResource<Material>("Materials/Stone.xml");
        REQUIRE(material != nullptr);
        terrain->SetMaterial(material);
        REQUIRE(terrain->GetMaterial() == material);
    }
}

TEST_CASE("Terrain patches know their neighbours", "[Graphics]")
{
    TerrainFixture fixture;
    Terrain* terrain = fixture.terrain_;

    TerrainPatch* topLeft = terrain->GetPatch(0, 0);
    TerrainPatch* topRight = terrain->GetPatch(1, 0);
    REQUIRE(topLeft != nullptr);
    REQUIRE(topRight != nullptr);

    REQUIRE(topLeft->GetEastPatch() == topRight);
    REQUIRE(topRight->GetWestPatch() == topLeft);
    REQUIRE(topLeft->GetWestPatch() == nullptr);

    SECTION("a neighbouring terrain is remembered")
    {
        Node* otherNode = fixture.scene_->CreateChild("East");
        otherNode->SetPosition(Vector3(32.0f, 0.0f, 0.0f));
        auto* other = otherNode->CreateComponent<Terrain>();
        other->SetPatchSize(16);
        other->SetHeightMap(MakeRamp(HeadlessContext()));

        terrain->SetEastNeighbor(other);
        REQUIRE(terrain->GetEastNeighbor() == other);

        terrain->SetNeighbors(nullptr, nullptr, nullptr, nullptr);
        REQUIRE(terrain->GetEastNeighbor() == nullptr);
        REQUIRE(terrain->GetNorthNeighbor() == nullptr);
    }
}

TEST_CASE("Terrain raycasts against its surface", "[Graphics]")
{
    TerrainFixture fixture;
    TerrainPatch* patch = fixture.terrain_->GetPatch(0, 0);
    REQUIRE(patch != nullptr);

    FrameInfo frame{};
    fixture.scene_->GetComponent<Octree>()->Update(frame);

    const BoundingBox bounds = patch->GetWorldBoundingBox();
    const Vector3 above(bounds.Center().x_, bounds.max_.y_ + 50.0f, bounds.Center().z_);

    PODVector<RayQueryResult> results;
    RayOctreeQuery query(results, Ray(above, Vector3::DOWN), RAY_TRIANGLE, 1000.0f);
    fixture.scene_->GetComponent<Octree>()->Raycast(query);

    REQUIRE_FALSE(results.Empty());
    REQUIRE(results[0].distance_ > 0.0f);
    REQUIRE_NEAR(results[0].position_.y_, fixture.terrain_->GetHeight(above), 0.5f);
}

#include "TestUtils.h"

#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Graphics/StaticModel.h>
#include <Urho3D/Navigation/DynamicNavigationMesh.h>
#include <Urho3D/Navigation/Navigable.h>
#include <Urho3D/Navigation/Obstacle.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/Scene.h>

using namespace Urho3D;
using namespace U3DTest;

namespace
{

struct DynamicFixture
{
    DynamicFixture() :
        scene_(new Scene(HeadlessContext()))
    {
        scene_->CreateComponent<Octree>();

        Node* floor = scene_->CreateChild("Floor");
        floor->SetScale(Vector3(40.0f, 1.0f, 40.0f));
        auto* drawable = floor->CreateComponent<StaticModel>();
        drawable->SetModel(HeadlessContext()->GetSubsystem<ResourceCache>()->GetResource<Model>("Models/Box.mdl"));

        scene_->CreateComponent<Navigable>();
        navMesh_ = scene_->CreateComponent<DynamicNavigationMesh>();
        navMesh_->SetCellSize(0.3f);
        navMesh_->SetAgentRadius(0.5f);
        navMesh_->SetAgentHeight(2.0f);
        navMesh_->SetTileSize(32);
    }

    void Settle(int frames = 20)
    {
        for (int i = 0; i < frames; ++i)
            scene_->Update(0.1f);
    }

    float PathLength(const Vector3& start, const Vector3& end)
    {
        PODVector<Vector3> path;
        navMesh_->FindPath(path, start, end);

        float length = 0.0f;
        for (unsigned i = 1; i < path.Size(); ++i)
            length += (path[i] - path[i - 1]).Length();
        return length;
    }

    Obstacle* AddObstacle(const Vector3& position, float radius, float height)
    {
        Node* node = scene_->CreateChild("Obstacle");
        node->SetPosition(position);
        auto* obstacle = node->CreateComponent<Obstacle>();
        obstacle->SetRadius(radius);
        obstacle->SetHeight(height);
        return obstacle;
    }

    SharedPtr<Scene> scene_;
    DynamicNavigationMesh* navMesh_{};
};

}

TEST_CASE("DynamicNavigationMesh builds and finds paths like a static one", "[Navigation]")
{
    DynamicFixture fixture;
    REQUIRE(fixture.navMesh_->Build());
    REQUIRE(fixture.navMesh_->GetBoundingBox().Defined());
    REQUIRE(fixture.navMesh_->GetNumTiles().x_ > 0);

    const Vector3 start(-12.0f, 0.0f, 0.0f);
    const Vector3 end(12.0f, 0.0f, 0.0f);

    PODVector<Vector3> path;
    fixture.navMesh_->FindPath(path, start, end);
    REQUIRE(path.Size() >= 2);
    REQUIRE_NEAR((path.Back() - end).Length(), 0.0f, 2.0f);

    SECTION("the obstacle and layer budgets round trip")
    {
        fixture.navMesh_->SetMaxObstacles(512);
        REQUIRE(fixture.navMesh_->GetMaxObstacles() == 512);

        fixture.navMesh_->SetMaxLayers(4);
        REQUIRE(fixture.navMesh_->GetMaxLayers() == 4);

        fixture.navMesh_->SetMaxLayers(100000);
        REQUIRE(fixture.navMesh_->GetMaxLayers() < 100000);

        fixture.navMesh_->SetDrawObstacles(true);
        REQUIRE(fixture.navMesh_->GetDrawObstacles());
    }

    SECTION("tiles can be read back and re-added")
    {
        const IntVector2 tile = fixture.navMesh_->GetTileIndex(Vector3::ZERO);
        REQUIRE(fixture.navMesh_->HasTile(tile));

        const PODVector<unsigned char> data = fixture.navMesh_->GetTileData(tile);
        REQUIRE_FALSE(data.Empty());

        fixture.navMesh_->RemoveTile(tile);
        REQUIRE_FALSE(fixture.navMesh_->HasTile(tile));

        REQUIRE(fixture.navMesh_->AddTile(data));
        REQUIRE(fixture.navMesh_->HasTile(tile));
    }

    SECTION("removing every tile leaves nothing to path over")
    {
        fixture.navMesh_->RemoveAllTiles();
        PODVector<Vector3> none;
        fixture.navMesh_->FindPath(none, start, end);
        REQUIRE(none.Empty());
    }
}

TEST_CASE("An obstacle carves a hole in a dynamic navigation mesh", "[Navigation]")
{
    DynamicFixture fixture;
    REQUIRE(fixture.navMesh_->Build());

    const Vector3 blocked(0.0f, 0.0f, 0.0f);
    const Vector3 start(-12.0f, 0.0f, 0.0f);
    const Vector3 end(12.0f, 0.0f, 0.0f);

    const float straight = fixture.PathLength(start, end);
    REQUIRE_NEAR(straight, (end - start).Length(), 0.5f);

    Obstacle* obstacle = fixture.AddObstacle(blocked, 4.0f, 5.0f);
    REQUIRE(obstacle != nullptr);
    REQUIRE_EQ_F(obstacle->GetRadius(), 4.0f);
    REQUIRE_EQ_F(obstacle->GetHeight(), 5.0f);
    REQUIRE(obstacle->GetObstacleID() != 0);
    REQUIRE(fixture.navMesh_->IsObstacleInTile(obstacle, fixture.navMesh_->GetTileIndex(blocked)));

    fixture.Settle();
    REQUIRE(fixture.PathLength(start, end) > straight + 0.5f);

    SECTION("removing the obstacle opens the floor back up")
    {
        obstacle->GetNode()->Remove();
        fixture.Settle();
        REQUIRE_NEAR(fixture.PathLength(start, end), straight, 0.5f);
    }

    SECTION("shrinking the obstacle shortens the detour")
    {
        const float wide = fixture.PathLength(start, end);

        obstacle->SetRadius(1.0f);
        REQUIRE_EQ_F(obstacle->GetRadius(), 1.0f);
        obstacle->SetHeight(2.0f);
        REQUIRE_EQ_F(obstacle->GetHeight(), 2.0f);

        fixture.Settle();
        const float narrow = fixture.PathLength(start, end);
        REQUIRE(narrow < wide);
        REQUIRE(narrow > straight);
    }
}

TEST_CASE("An obstacle outside any tile is harmless", "[Navigation]")
{
    DynamicFixture fixture;
    REQUIRE(fixture.navMesh_->Build());

    Obstacle* far = fixture.AddObstacle(Vector3(500.0f, 0.0f, 500.0f), 2.0f, 2.0f);
    REQUIRE(far != nullptr);
    REQUIRE_FALSE(fixture.navMesh_->IsObstacleInTile(far, fixture.navMesh_->GetTileIndex(Vector3::ZERO)));

    for (int i = 0; i < 10; ++i)
        fixture.scene_->Update(0.1f);

    PODVector<Vector3> path;
    fixture.navMesh_->FindPath(path, Vector3(-12.0f, 0.0f, 0.0f), Vector3(12.0f, 0.0f, 0.0f));
    REQUIRE(path.Size() >= 2);
}

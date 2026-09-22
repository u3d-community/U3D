#include "TestUtils.h"

#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Graphics/StaticModel.h>
#include <Urho3D/Navigation/CrowdAgent.h>
#include <Urho3D/Navigation/CrowdManager.h>
#include <Urho3D/Navigation/NavArea.h>
#include <Urho3D/Navigation/Navigable.h>
#include <Urho3D/Navigation/NavigationMesh.h>
#include <Urho3D/Navigation/OffMeshConnection.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/Scene.h>

using namespace Urho3D;
using namespace U3DTest;

namespace
{

struct NavigationFixture
{
    NavigationFixture() :
        scene_(new Scene(HeadlessContext()))
    {
        scene_->CreateComponent<Octree>();

        Node* floor = scene_->CreateChild("Floor");
        floor->SetScale(Vector3(40.0f, 1.0f, 40.0f));
        auto* drawable = floor->CreateComponent<StaticModel>();
        drawable->SetModel(HeadlessContext()->GetSubsystem<ResourceCache>()->GetResource<Model>("Models/Box.mdl"));

        scene_->CreateComponent<Navigable>();
        navMesh_ = scene_->CreateComponent<NavigationMesh>();
        navMesh_->SetCellSize(0.3f);
        navMesh_->SetAgentRadius(0.5f);
        navMesh_->SetAgentHeight(2.0f);
    }

    SharedPtr<Scene> scene_;
    NavigationMesh* navMesh_{};
};

}

TEST_CASE("NavigationMesh clamps its build parameters", "[Navigation]")
{
    SharedPtr<Scene> scene(new Scene(HeadlessContext()));
    auto* mesh = scene->CreateComponent<NavigationMesh>();

    mesh->SetTileSize(0);
    REQUIRE(mesh->GetTileSize() >= 1);
    mesh->SetTileSize(64);
    REQUIRE(mesh->GetTileSize() == 64);

    mesh->SetCellSize(-1.0f);
    REQUIRE(mesh->GetCellSize() > 0.0f);
    mesh->SetCellHeight(-1.0f);
    REQUIRE(mesh->GetCellHeight() > 0.0f);

    mesh->SetAgentHeight(-1.0f);
    REQUIRE(mesh->GetAgentHeight() > 0.0f);
    mesh->SetAgentRadius(-1.0f);
    REQUIRE(mesh->GetAgentRadius() >= 0.0f);
    mesh->SetAgentMaxClimb(-1.0f);
    REQUIRE(mesh->GetAgentMaxClimb() >= 0.0f);

    mesh->SetAgentMaxSlope(45.0f);
    REQUIRE_EQ_F(mesh->GetAgentMaxSlope(), 45.0f);

    mesh->SetRegionMinSize(-1.0f);
    REQUIRE(mesh->GetRegionMinSize() >= 0.0f);
    mesh->SetEdgeMaxLength(-1.0f);
    REQUIRE(mesh->GetEdgeMaxLength() >= 0.0f);
    mesh->SetEdgeMaxError(-1.0f);
    REQUIRE(mesh->GetEdgeMaxError() >= 0.0f);
    mesh->SetDetailSampleDistance(-1.0f);
    REQUIRE(mesh->GetDetailSampleDistance() >= 0.0f);
    mesh->SetDetailSampleMaxError(-1.0f);
    REQUIRE(mesh->GetDetailSampleMaxError() >= 0.0f);

    mesh->SetPadding(Vector3(2.0f, 3.0f, 4.0f));
    REQUIRE(mesh->GetPadding() == Vector3(2.0f, 3.0f, 4.0f));

    mesh->SetAreaCost(1, 5.0f);
    REQUIRE_EQ_F(mesh->GetAreaCost(1), 5.0f);

    SECTION("an unbuilt mesh has no bounds and finds no path")
    {
        REQUIRE_FALSE(mesh->GetBoundingBox().Defined());
        REQUIRE(mesh->GetNumTiles() == IntVector2::ZERO);

        PODVector<Vector3> path;
        mesh->FindPath(path, Vector3::ZERO, Vector3(5.0f, 0.0f, 5.0f));
        REQUIRE(path.Empty());
    }

    SECTION("building without any navigable geometry is a no op rather than a failure")
    {
        REQUIRE(mesh->Build());
        REQUIRE_FALSE(mesh->GetBoundingBox().Defined());

        PODVector<Vector3> path;
        mesh->FindPath(path, Vector3::ZERO, Vector3(5.0f, 0.0f, 5.0f));
        REQUIRE(path.Empty());
    }
}

TEST_CASE("NavigationMesh builds over scene geometry and finds paths", "[Navigation]")
{
    NavigationFixture fixture;
    REQUIRE(fixture.navMesh_->Build());
    REQUIRE(fixture.navMesh_->GetBoundingBox().Defined());
    REQUIRE(fixture.navMesh_->GetNumTiles().x_ > 0);

    const Vector3 start(-10.0f, 0.0f, -10.0f);
    const Vector3 end(10.0f, 0.0f, 10.0f);

    PODVector<Vector3> path;
    fixture.navMesh_->FindPath(path, start, end);
    REQUIRE(path.Size() >= 2);
    REQUIRE_NEAR((path.Front() - start).Length(), 0.0f, 2.0f);
    REQUIRE_NEAR((path.Back() - end).Length(), 0.0f, 2.0f);

    SECTION("the nearest point on the mesh is close to a point above it")
    {
        const Vector3 above(0.0f, 20.0f, 0.0f);
        const Vector3 nearest = fixture.navMesh_->FindNearestPoint(above, Vector3(1.0f, 50.0f, 1.0f));
        REQUIRE_NEAR(nearest.x_, 0.0f, 1.0f);
        REQUIRE(nearest.y_ < above.y_);
    }

    SECTION("moving along the surface stops at the destination")
    {
        const Vector3 moved = fixture.navMesh_->MoveAlongSurface(start, end, Vector3(1.0f, 5.0f, 1.0f), 32);
        REQUIRE((moved - start).Length() > 0.0f);
    }

    SECTION("a random point lands inside the mesh bounds")
    {
        const Vector3 random = fixture.navMesh_->GetRandomPoint();
        const BoundingBox bounds = fixture.navMesh_->GetBoundingBox();
        REQUIRE(random.x_ >= bounds.min_.x_);
        REQUIRE(random.x_ <= bounds.max_.x_);
        REQUIRE(random.z_ >= bounds.min_.z_);
        REQUIRE(random.z_ <= bounds.max_.z_);

        const Vector3 nearby = fixture.navMesh_->GetRandomPointInCircle(Vector3::ZERO, 5.0f, Vector3::ONE);
        REQUIRE(nearby.x_ >= bounds.min_.x_);
        REQUIRE(nearby.x_ <= bounds.max_.x_);
    }

    SECTION("a raycast across open floor reaches the far end")
    {
        const Vector3 hit = fixture.navMesh_->Raycast(start, end);
        REQUIRE_NEAR((hit - end).Length(), 0.0f, 2.0f);
    }

    SECTION("tiles can be read back, removed, and re-added")
    {
        const IntVector2 tile = fixture.navMesh_->GetTileIndex(Vector3::ZERO);
        REQUIRE(fixture.navMesh_->HasTile(tile));
        REQUIRE(fixture.navMesh_->GetTileBoundingBox(tile).Defined());

        const PODVector<unsigned char> data = fixture.navMesh_->GetTileData(tile);
        REQUIRE_FALSE(data.Empty());

        fixture.navMesh_->RemoveTile(tile);
        REQUIRE_FALSE(fixture.navMesh_->HasTile(tile));

        REQUIRE(fixture.navMesh_->AddTile(data));
        REQUIRE(fixture.navMesh_->HasTile(tile));
    }

    SECTION("removing every tile empties the mesh")
    {
        fixture.navMesh_->RemoveAllTiles();
        PODVector<Vector3> none;
        fixture.navMesh_->FindPath(none, start, end);
        REQUIRE(none.Empty());
    }

    SECTION("rebuilding over a smaller box keeps the mesh usable")
    {
        REQUIRE(fixture.navMesh_->Build(BoundingBox(Vector3(-5.0f, -5.0f, -5.0f), Vector3(5.0f, 5.0f, 5.0f))));
        PODVector<Vector3> shortPath;
        fixture.navMesh_->FindPath(shortPath, Vector3(-2.0f, 0.0f, -2.0f), Vector3(2.0f, 0.0f, 2.0f));
        REQUIRE(shortPath.Size() >= 2);
    }
}

TEST_CASE("OffMeshConnection and NavArea expose their geometry", "[Navigation]")
{
    SharedPtr<Scene> scene(new Scene(HeadlessContext()));
    Node* node = scene->CreateChild("Link");
    node->SetPosition(Vector3(1.0f, 0.0f, 0.0f));
    Node* endPoint = scene->CreateChild("LinkEnd");
    endPoint->SetPosition(Vector3(5.0f, 0.0f, 0.0f));

    auto* connection = node->CreateComponent<OffMeshConnection>();
    connection->SetEndPoint(endPoint);
    connection->SetRadius(2.0f);
    connection->SetBidirectional(false);
    connection->SetMask(0x3);
    connection->SetAreaID(4);

    REQUIRE(connection->GetEndPoint() == endPoint);
    REQUIRE_EQ_F(connection->GetRadius(), 2.0f);
    REQUIRE_FALSE(connection->IsBidirectional());
    REQUIRE(connection->GetMask() == 0x3);
    REQUIRE(connection->GetAreaID() == 4);

    auto* area = scene->CreateChild("Area")->CreateComponent<NavArea>();
    area->SetBoundingBox(BoundingBox(Vector3(-1.0f, -1.0f, -1.0f), Vector3(1.0f, 1.0f, 1.0f)));
    area->SetAreaID(7);
    REQUIRE(area->GetAreaID() == 7);
    REQUIRE(area->GetBoundingBox().Size() == Vector3(2.0f, 2.0f, 2.0f));

    area->GetNode()->SetPosition(Vector3(10.0f, 0.0f, 0.0f));
    REQUIRE(area->GetWorldBoundingBox().Center().Equals(Vector3(10.0f, 0.0f, 0.0f)));
    REQUIRE(area->GetBoundingBox().Center().Equals(Vector3::ZERO));
}

TEST_CASE("Navigable controls whether its subtree is collected", "[Navigation]")
{
    SharedPtr<Scene> scene(new Scene(HeadlessContext()));
    auto* navigable = scene->CreateComponent<Navigable>();

    REQUIRE(navigable->IsRecursive());
    navigable->SetRecursive(false);
    REQUIRE_FALSE(navigable->IsRecursive());
    navigable->SetRecursive(true);
    REQUIRE(navigable->IsRecursive());
}

TEST_CASE("CrowdManager steers an agent across a built mesh", "[Navigation]")
{
    NavigationFixture fixture;
    REQUIRE(fixture.navMesh_->Build());

    auto* crowd = fixture.scene_->CreateComponent<CrowdManager>();
    crowd->SetMaxAgents(16);
    crowd->SetMaxAgentRadius(1.0f);
    REQUIRE(crowd->GetMaxAgents() == 16);
    REQUIRE_EQ_F(crowd->GetMaxAgentRadius(), 1.0f);

    crowd->SetAreaCost(0, 1, 3.0f);
    REQUIRE(crowd->GetNumQueryFilterTypes() >= 1);
    REQUIRE_EQ_F(crowd->GetAreaCost(0, 1), 3.0f);

    Node* agentNode = fixture.scene_->CreateChild("Agent");
    agentNode->SetPosition(Vector3(-10.0f, 0.0f, -10.0f));
    auto* agent = agentNode->CreateComponent<CrowdAgent>();
    agent->SetMaxSpeed(5.0f);
    agent->SetMaxAccel(10.0f);
    agent->SetRadius(0.5f);
    agent->SetHeight(2.0f);

    REQUIRE_EQ_F(agent->GetMaxSpeed(), 5.0f);
    REQUIRE_EQ_F(agent->GetRadius(), 0.5f);
    REQUIRE(agent->IsInCrowd());
    REQUIRE(crowd->GetAgents().Size() == 1);

    const Vector3 target(10.0f, 0.0f, 10.0f);
    agent->SetTargetPosition(target);
    REQUIRE(agent->GetTargetPosition().Equals(target));

    const Vector3 before = agentNode->GetWorldPosition();
    for (int i = 0; i < 30; ++i)
        fixture.scene_->Update(1.0f / 30.0f);

    REQUIRE((agentNode->GetWorldPosition() - target).Length() < (before - target).Length());

    SECTION("resetting the target brings the agent to a halt")
    {
        agent->ResetTarget();
        for (int i = 0; i < 30; ++i)
            fixture.scene_->Update(1.0f / 30.0f);

        const Vector3 resting = agentNode->GetWorldPosition();
        for (int i = 0; i < 30; ++i)
            fixture.scene_->Update(1.0f / 30.0f);

        REQUIRE_NEAR((agentNode->GetWorldPosition() - resting).Length(), 0.0f, 0.01f);
    }

    SECTION("removing the node takes the agent out of the crowd")
    {
        agentNode->Remove();
        REQUIRE(crowd->GetAgents().Empty());
    }
}

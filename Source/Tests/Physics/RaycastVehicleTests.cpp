#include "TestUtils.h"

#include <Urho3D/Physics/CollisionShape.h>
#include <Urho3D/Physics/PhysicsWorld.h>
#include <Urho3D/Physics/RaycastVehicle.h>
#include <Urho3D/Physics/RigidBody.h>
#include <Urho3D/Scene/Scene.h>

using namespace Urho3D;
using namespace U3DTest;

namespace
{

struct VehicleFixture
{
    VehicleFixture() :
        scene_(new Scene(HeadlessContext()))
    {
        world_ = scene_->CreateComponent<PhysicsWorld>();
        world_->SetFps(60);

        Node* floor = scene_->CreateChild("Floor");
        floor->SetPosition(Vector3(0.0f, -0.5f, 0.0f));
        floor->CreateComponent<RigidBody>();
        floor->CreateComponent<CollisionShape>()->SetBox(Vector3(500.0f, 1.0f, 500.0f));

        node_ = scene_->CreateChild("Vehicle");
        node_->SetPosition(Vector3(0.0f, 2.0f, 0.0f));

        body_ = node_->CreateComponent<RigidBody>();
        body_->SetMass(800.0f);
        node_->CreateComponent<CollisionShape>()->SetBox(Vector3(2.0f, 1.0f, 4.0f));

        vehicle_ = node_->CreateComponent<RaycastVehicle>();
        vehicle_->Init();

        const float x = 1.0f;
        const float z = 1.6f;
        AddWheel(Vector3(-x, 0.0f, z), true);
        AddWheel(Vector3(x, 0.0f, z), true);
        AddWheel(Vector3(-x, 0.0f, -z), false);
        AddWheel(Vector3(x, 0.0f, -z), false);
    }

    void AddWheel(const Vector3& offset, bool front)
    {
        Node* wheelNode = scene_->CreateChild("Wheel");
        wheelNode->SetPosition(node_->GetPosition() + offset);
        vehicle_->AddWheel(wheelNode, Vector3::DOWN, Vector3::RIGHT, 0.4f, 0.5f, front);
    }

    void Step(int frames)
    {
        for (int i = 0; i < frames; ++i)
            scene_->Update(1.0f / 60.0f);
    }

    SharedPtr<Scene> scene_;
    PhysicsWorld* world_{};
    Node* node_{};
    RigidBody* body_{};
    RaycastVehicle* vehicle_{};
};

}

TEST_CASE("RaycastVehicle builds a wheel set", "[Physics]")
{
    VehicleFixture fixture;
    RaycastVehicle* vehicle = fixture.vehicle_;

    REQUIRE(vehicle->GetNumWheels() == 4);

    for (int i = 0; i < 4; ++i)
    {
        REQUIRE(vehicle->GetWheelNode(i) != nullptr);
        REQUIRE_EQ_F(vehicle->GetWheelRadius(i), 0.5f);
        REQUIRE(vehicle->GetWheelDirection(i) == Vector3::DOWN);
        REQUIRE(vehicle->GetWheelAxle(i) == Vector3::RIGHT);
    }

    REQUIRE(vehicle->IsFrontWheel(0));
    REQUIRE(vehicle->IsFrontWheel(1));
    REQUIRE_FALSE(vehicle->IsFrontWheel(2));
    REQUIRE_FALSE(vehicle->IsFrontWheel(3));

    SECTION("suspension parameters round trip per wheel")
    {
        vehicle->SetWheelSuspensionStiffness(0, 30.0f);
        vehicle->SetWheelDampingRelaxation(0, 4.0f);
        vehicle->SetWheelDampingCompression(0, 2.5f);
        vehicle->SetWheelFrictionSlip(0, 1.5f);
        vehicle->SetWheelRollInfluence(0, 0.2f);
        vehicle->SetMaxSuspensionTravel(0, 25.0f);
        vehicle->SetWheelMaxSuspensionForce(0, 9000.0f);

        REQUIRE_EQ_F(vehicle->GetWheelSuspensionStiffness(0), 30.0f);
        REQUIRE_EQ_F(vehicle->GetWheelDampingRelaxation(0), 4.0f);
        REQUIRE_EQ_F(vehicle->GetWheelDampingCompression(0), 2.5f);
        REQUIRE_EQ_F(vehicle->GetWheelFrictionSlip(0), 1.5f);
        REQUIRE_EQ_F(vehicle->GetWheelRollInfluence(0), 0.2f);
        REQUIRE_EQ_F(vehicle->GetMaxSuspensionTravel(0), 25.0f);
        REQUIRE_EQ_F(vehicle->GetWheelMaxSuspensionForce(0), 9000.0f);

        REQUIRE_FALSE(Equals(vehicle->GetWheelSuspensionStiffness(1), 30.0f));
    }

    SECTION("the whole vehicle settings round trip")
    {
        vehicle->SetMaxSideSlipSpeed(5.0f);
        vehicle->SetInAirRPM(1800.0f);
        REQUIRE_EQ_F(vehicle->GetMaxSideSlipSpeed(), 5.0f);
        REQUIRE_EQ_F(vehicle->GetInAirRPM(), 1800.0f);

        vehicle->SetCoordinateSystem(IntVector3(0, 2, 1));
        REQUIRE(vehicle->GetCoordinateSystem() == IntVector3(0, 2, 1));
        vehicle->SetCoordinateSystem();
        REQUIRE(vehicle->GetCoordinateSystem() == RaycastVehicle::RIGHT_FORWARD_UP);
    }

    SECTION("the wheel connection point follows the node it was built from")
    {
        const Vector3 connection = vehicle->GetWheelConnectionPoint(0);
        REQUIRE(connection.x_ < 0.0f);
        REQUIRE(connection.z_ > 0.0f);
    }

    SECTION("the last wheel added is the last index")
    {
        REQUIRE(vehicle->GetWheelNode(vehicle->GetNumWheels() - 1) != nullptr);
        REQUIRE(vehicle->GetWheelNode(0) != vehicle->GetWheelNode(1));
    }
}

TEST_CASE("RaycastVehicle rests on the ground and drives", "[Physics]")
{
    VehicleFixture fixture;
    RaycastVehicle* vehicle = fixture.vehicle_;

    for (int i = 0; i < 4; ++i)
    {
        vehicle->SetWheelSuspensionStiffness(i, 40.0f);
        vehicle->SetWheelDampingRelaxation(i, 3.0f);
        vehicle->SetWheelDampingCompression(i, 4.0f);
        vehicle->SetWheelFrictionSlip(i, 2.0f);
        vehicle->SetMaxSuspensionTravel(i, 30.0f);
    }

    const float dropped = fixture.node_->GetWorldPosition().y_;
    fixture.Step(120);
    const float settled = fixture.node_->GetWorldPosition().y_;

    REQUIRE(settled < dropped);
    REQUIRE(settled > 0.5f);

    fixture.Step(60);
    REQUIRE_NEAR(fixture.node_->GetWorldPosition().y_, settled, 0.05f);

    SECTION("engine force moves the vehicle forwards")
    {
        const Vector3 start = fixture.node_->GetWorldPosition();

        for (int i = 0; i < 4; ++i)
            vehicle->SetEngineForce(i, 2000.0f);

        fixture.Step(120);

        const Vector3 end = fixture.node_->GetWorldPosition();
        REQUIRE((end - start).Length() > 1.0f);
        REQUIRE(fixture.body_->GetLinearVelocity().Length() > 0.5f);
    }

    SECTION("the brake brings it back to a stop")
    {
        for (int i = 0; i < 4; ++i)
            vehicle->SetEngineForce(i, 2000.0f);
        fixture.Step(120);
        REQUIRE(fixture.body_->GetLinearVelocity().Length() > 0.5f);

        for (int i = 0; i < 4; ++i)
        {
            vehicle->SetEngineForce(i, 0.0f);
            vehicle->SetBrake(i, 2000.0f);
        }
        fixture.Step(240);

        REQUIRE(fixture.body_->GetLinearVelocity().Length() < 0.5f);
    }

    SECTION("steering turns the front wheels")
    {
        vehicle->SetSteeringValue(0, 0.4f);
        vehicle->SetSteeringValue(1, 0.4f);
        REQUIRE_EQ_F(vehicle->GetSteeringValue(0), 0.4f);
        REQUIRE_EQ_F(vehicle->GetSteeringValue(1), 0.4f);
        REQUIRE_EQ_F(vehicle->GetSteeringValue(2), 0.0f);

        for (int i = 0; i < 4; ++i)
            vehicle->SetEngineForce(i, 2000.0f);

        const Quaternion start = fixture.node_->GetWorldRotation();
        fixture.Step(180);

        REQUIRE_FALSE(fixture.node_->GetWorldRotation().Equals(start));
    }

    SECTION("wheel transforms track the suspension")
    {
        vehicle->UpdateWheelTransform(0, false);
        const Vector3 position = vehicle->GetWheelPosition(0);
        REQUIRE(position.y_ < fixture.node_->GetWorldPosition().y_ + 1.0f);
        REQUIRE(position.x_ < 0.0f);
        REQUIRE(position.z_ > 0.0f);
        REQUIRE_FALSE(vehicle->GetWheelRotation(0).IsNaN());

        REQUIRE(vehicle->GetWheelSideSlipSpeed(0) >= 0.0f);
    }

    SECTION("resetting the suspension and wheels is harmless mid simulation")
    {
        vehicle->ResetSuspension();
        vehicle->ResetWheels();
        fixture.Step(10);
        REQUIRE(fixture.node_->GetWorldPosition().y_ > 0.0f);
    }

    SECTION("skid info is tracked per wheel")
    {
        vehicle->SetWheelSkidInfo(0, 0.5f);
        REQUIRE_EQ_F(vehicle->GetWheelSkidInfo(0), 0.5f);

        vehicle->SetWheelSkidInfoCumulative(0, 0.25f);
        REQUIRE_EQ_F(vehicle->GetWheelSkidInfoCumulative(0), 0.25f);
    }
}

TEST_CASE("A vehicle with no wheels still simulates as a rigid body", "[Physics]")
{
    SharedPtr<Scene> scene(new Scene(HeadlessContext()));
    auto* world = scene->CreateComponent<PhysicsWorld>();

    Node* node = scene->CreateChild("Bare");
    node->SetPosition(Vector3(0.0f, 10.0f, 0.0f));
    auto* body = node->CreateComponent<RigidBody>();
    body->SetMass(100.0f);
    node->CreateComponent<CollisionShape>()->SetBox(Vector3::ONE);

    auto* vehicle = node->CreateComponent<RaycastVehicle>();
    vehicle->Init();
    REQUIRE(vehicle->GetNumWheels() == 0);

    for (int i = 0; i < 60; ++i)
        world->Update(1.0f / 60.0f);

    REQUIRE(node->GetWorldPosition().y_ < 10.0f);
}

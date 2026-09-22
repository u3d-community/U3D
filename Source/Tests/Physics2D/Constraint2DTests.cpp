#include "TestUtils.h"

#include <Urho3D/Physics2D/CollisionBox2D.h>
#include <Urho3D/Physics2D/ConstraintDistance2D.h>
#include <Urho3D/Physics2D/ConstraintFriction2D.h>
#include <Urho3D/Physics2D/ConstraintGear2D.h>
#include <Urho3D/Physics2D/ConstraintMotor2D.h>
#include <Urho3D/Physics2D/ConstraintMouse2D.h>
#include <Urho3D/Physics2D/ConstraintPrismatic2D.h>
#include <Urho3D/Physics2D/ConstraintPulley2D.h>
#include <Urho3D/Physics2D/ConstraintRevolute2D.h>
#include <Urho3D/Physics2D/ConstraintRope2D.h>
#include <Urho3D/Physics2D/ConstraintWeld2D.h>
#include <Urho3D/Physics2D/ConstraintWheel2D.h>
#include <Urho3D/Physics2D/PhysicsWorld2D.h>
#include <Urho3D/Physics2D/RigidBody2D.h>
#include <Urho3D/Scene/Scene.h>

using namespace Urho3D;
using namespace U3DTest;

namespace
{

struct JointFixture
{
    JointFixture() :
        scene_(new Scene(HeadlessContext()))
    {
        world_ = scene_->CreateComponent<PhysicsWorld2D>();
        world_->SetGravity(Vector2(0.0f, -10.0f));
        anchor_ = MakeBody(Vector2::ZERO, BT_STATIC);
        hanging_ = MakeBody(Vector2(0.0f, -3.0f), BT_DYNAMIC);
    }

    RigidBody2D* MakeBody(const Vector2& position, BodyType2D type)
    {
        Node* node = scene_->CreateChild();
        node->SetPosition(Vector3(position.x_, position.y_, 0.0f));
        auto* body = node->CreateComponent<RigidBody2D>();
        body->SetBodyType(type);
        node->CreateComponent<CollisionBox2D>()->SetSize(1.0f, 1.0f);
        return body;
    }

    void Step(int frames)
    {
        for (int i = 0; i < frames; ++i)
            world_->Update(1.0f / 60.0f);
    }

    SharedPtr<Scene> scene_;
    PhysicsWorld2D* world_{};
    RigidBody2D* anchor_{};
    RigidBody2D* hanging_{};
};

template <class T> T* Attach(JointFixture& fixture)
{
    auto* constraint = fixture.hanging_->GetNode()->CreateComponent<T>();
    constraint->SetOtherBody(fixture.anchor_);
    return constraint;
}

}

TEST_CASE("Constraint2D links two bodies and can be released", "[Physics2D]")
{
    JointFixture fixture;
    auto* constraint = Attach<ConstraintDistance2D>(fixture);

    REQUIRE(constraint->GetOwnerBody() == fixture.hanging_);
    REQUIRE(constraint->GetOtherBody() == fixture.anchor_);
    REQUIRE(constraint->GetJoint() != nullptr);

    constraint->SetCollideConnected(true);
    REQUIRE(constraint->GetCollideConnected());

    SECTION("clearing the other body drops the joint")
    {
        constraint->SetOtherBody(nullptr);
        REQUIRE(constraint->GetOtherBody() == nullptr);
        REQUIRE(constraint->GetJoint() == nullptr);
    }

    SECTION("removing the component drops the joint too")
    {
        Node* node = fixture.hanging_->GetNode();
        node->RemoveComponent(constraint);
        REQUIRE(node->GetComponent<ConstraintDistance2D>() == nullptr);
        fixture.Step(10);
    }

    SECTION("a constraint with no other body has no joint")
    {
        auto* lonely = fixture.hanging_->GetNode()->CreateComponent<ConstraintFriction2D>();
        REQUIRE(lonely->GetJoint() == nullptr);
        REQUIRE(lonely->GetOtherBody() == nullptr);
    }
}

TEST_CASE("ConstraintDistance2D holds a body at a fixed distance", "[Physics2D]")
{
    JointFixture fixture;
    auto* constraint = Attach<ConstraintDistance2D>(fixture);

    constraint->SetOwnerBodyAnchor(Vector2::ZERO);
    constraint->SetOtherBodyAnchor(Vector2::ZERO);
    constraint->SetFrequencyHz(0.0f);
    constraint->SetDampingRatio(0.0f);

    REQUIRE(constraint->GetOwnerBodyAnchor() == Vector2::ZERO);
    REQUIRE(constraint->GetOtherBodyAnchor() == Vector2::ZERO);
    REQUIRE_EQ_F(constraint->GetFrequencyHz(), 0.0f);
    REQUIRE_EQ_F(constraint->GetDampingRatio(), 0.0f);

    const float startDistance = fixture.hanging_->GetNode()->GetWorldPosition().Length();
    fixture.Step(240);

    REQUIRE_NEAR(fixture.hanging_->GetNode()->GetWorldPosition().Length(), startDistance, 0.2f);
}

TEST_CASE("ConstraintRevolute2D limits and motors a hinge", "[Physics2D]")
{
    JointFixture fixture;
    auto* constraint = Attach<ConstraintRevolute2D>(fixture);

    constraint->SetAnchor(Vector2::ZERO);
    REQUIRE(constraint->GetAnchor() == Vector2::ZERO);

    constraint->SetEnableLimit(true);
    constraint->SetLowerAngle(-30.0f);
    constraint->SetUpperAngle(30.0f);
    REQUIRE(constraint->GetEnableLimit());
    REQUIRE_EQ_F(constraint->GetLowerAngle(), -30.0f);
    REQUIRE_EQ_F(constraint->GetUpperAngle(), 30.0f);

    constraint->SetEnableMotor(true);
    constraint->SetMotorSpeed(90.0f);
    constraint->SetMaxMotorTorque(50.0f);
    REQUIRE(constraint->GetEnableMotor());
    REQUIRE_EQ_F(constraint->GetMotorSpeed(), 90.0f);
    REQUIRE_EQ_F(constraint->GetMaxMotorTorque(), 50.0f);

    REQUIRE(constraint->GetJoint() != nullptr);
    fixture.Step(60);

    REQUIRE(fixture.hanging_->GetNode()->GetWorldPosition().Length() < 5.0f);
}

TEST_CASE("ConstraintPrismatic2D slides along an axis", "[Physics2D]")
{
    JointFixture fixture;
    auto* constraint = Attach<ConstraintPrismatic2D>(fixture);

    constraint->SetAnchor(Vector2::ZERO);
    constraint->SetAxis(Vector2(0.0f, 1.0f));
    REQUIRE(constraint->GetAnchor() == Vector2::ZERO);
    REQUIRE(constraint->GetAxis() == Vector2(0.0f, 1.0f));

    constraint->SetEnableLimit(true);
    constraint->SetLowerTranslation(-2.0f);
    constraint->SetUpperTranslation(2.0f);
    REQUIRE(constraint->GetEnableLimit());
    REQUIRE_EQ_F(constraint->GetLowerTranslation(), -2.0f);
    REQUIRE_EQ_F(constraint->GetUpperTranslation(), 2.0f);

    constraint->SetEnableMotor(true);
    constraint->SetMotorSpeed(1.0f);
    constraint->SetMaxMotorForce(10.0f);
    REQUIRE(constraint->GetEnableMotor());
    REQUIRE_EQ_F(constraint->GetMotorSpeed(), 1.0f);
    REQUIRE_EQ_F(constraint->GetMaxMotorForce(), 10.0f);

    fixture.Step(120);

    REQUIRE_NEAR(fixture.hanging_->GetNode()->GetWorldPosition().x_, 0.0f, 0.1f);
}

TEST_CASE("ConstraintRope2D caps the distance between two bodies", "[Physics2D]")
{
    JointFixture fixture;
    auto* constraint = Attach<ConstraintRope2D>(fixture);

    constraint->SetOwnerBodyAnchor(Vector2::ZERO);
    constraint->SetOtherBodyAnchor(Vector2::ZERO);
    constraint->SetMaxLength(4.0f);

    REQUIRE(constraint->GetOwnerBodyAnchor() == Vector2::ZERO);
    REQUIRE(constraint->GetOtherBodyAnchor() == Vector2::ZERO);
    REQUIRE_EQ_F(constraint->GetMaxLength(), 4.0f);

    fixture.Step(300);
    REQUIRE(fixture.hanging_->GetNode()->GetWorldPosition().Length() <= 4.5f);
}

TEST_CASE("ConstraintWeld2D fuses two bodies together", "[Physics2D]")
{
    JointFixture fixture;
    auto* constraint = Attach<ConstraintWeld2D>(fixture);

    constraint->SetAnchor(Vector2(0.0f, -1.5f));
    constraint->SetFrequencyHz(0.0f);
    constraint->SetDampingRatio(0.0f);

    REQUIRE(constraint->GetAnchor() == Vector2(0.0f, -1.5f));
    REQUIRE_EQ_F(constraint->GetFrequencyHz(), 0.0f);
    REQUIRE_EQ_F(constraint->GetDampingRatio(), 0.0f);

    const Vector3 start = fixture.hanging_->GetNode()->GetWorldPosition();
    fixture.Step(180);

    REQUIRE_NEAR((fixture.hanging_->GetNode()->GetWorldPosition() - start).Length(), 0.0f, 0.2f);
}

TEST_CASE("ConstraintMotor2D drives a body towards an offset", "[Physics2D]")
{
    JointFixture fixture;
    auto* constraint = Attach<ConstraintMotor2D>(fixture);

    constraint->SetLinearOffset(Vector2(3.0f, 0.0f));
    constraint->SetAngularOffset(45.0f);
    constraint->SetMaxForce(500.0f);
    constraint->SetMaxTorque(500.0f);
    constraint->SetCorrectionFactor(0.5f);

    REQUIRE(constraint->GetLinearOffset() == Vector2(3.0f, 0.0f));
    REQUIRE_EQ_F(constraint->GetAngularOffset(), 45.0f);
    REQUIRE_EQ_F(constraint->GetMaxForce(), 500.0f);
    REQUIRE_EQ_F(constraint->GetMaxTorque(), 500.0f);
    REQUIRE_EQ_F(constraint->GetCorrectionFactor(), 0.5f);

    const Vector3 start = fixture.hanging_->GetNode()->GetWorldPosition();
    fixture.Step(180);
    const Vector3 end = fixture.hanging_->GetNode()->GetWorldPosition();

    REQUIRE((end - start).Length() > 1.0f);
    REQUIRE(end.y_ > -6.0f);
}

TEST_CASE("ConstraintWheel2D suspends a body on a spring", "[Physics2D]")
{
    JointFixture fixture;
    auto* constraint = Attach<ConstraintWheel2D>(fixture);

    constraint->SetAnchor(Vector2(0.0f, -3.0f));
    constraint->SetAxis(Vector2(0.0f, 1.0f));
    constraint->SetEnableMotor(true);
    constraint->SetMaxMotorTorque(20.0f);
    constraint->SetMotorSpeed(180.0f);
    constraint->SetFrequencyHz(4.0f);
    constraint->SetDampingRatio(0.7f);

    REQUIRE(constraint->GetAnchor() == Vector2(0.0f, -3.0f));
    REQUIRE(constraint->GetAxis() == Vector2(0.0f, 1.0f));
    REQUIRE(constraint->GetEnableMotor());
    REQUIRE_EQ_F(constraint->GetMaxMotorTorque(), 20.0f);
    REQUIRE_EQ_F(constraint->GetMotorSpeed(), 180.0f);
    REQUIRE_EQ_F(constraint->GetFrequencyHz(), 4.0f);
    REQUIRE_EQ_F(constraint->GetDampingRatio(), 0.7f);

    REQUIRE(constraint->GetJoint() != nullptr);

    fixture.Step(120);

    REQUIRE_NEAR(fixture.hanging_->GetNode()->GetWorldPosition().x_, 0.0f, 0.1f);
}

TEST_CASE("ConstraintFriction2D resists sliding", "[Physics2D]")
{
    JointFixture fixture;
    auto* constraint = Attach<ConstraintFriction2D>(fixture);

    constraint->SetAnchor(Vector2::ZERO);
    constraint->SetMaxForce(100.0f);
    constraint->SetMaxTorque(100.0f);

    REQUIRE(constraint->GetAnchor() == Vector2::ZERO);
    REQUIRE_EQ_F(constraint->GetMaxForce(), 100.0f);
    REQUIRE_EQ_F(constraint->GetMaxTorque(), 100.0f);
    REQUIRE(constraint->GetJoint() != nullptr);

    fixture.hanging_->SetLinearVelocity(Vector2(10.0f, 0.0f));
    const float startSpeed = fixture.hanging_->GetLinearVelocity().Length();
    fixture.Step(30);
    REQUIRE(fixture.hanging_->GetLinearVelocity().x_ < startSpeed);
}

TEST_CASE("ConstraintPulley2D and ConstraintGear2D expose their setup", "[Physics2D]")
{
    JointFixture fixture;

    auto* pulley = Attach<ConstraintPulley2D>(fixture);
    pulley->SetOwnerBodyGroundAnchor(Vector2(-1.0f, 5.0f));
    pulley->SetOtherBodyGroundAnchor(Vector2(1.0f, 5.0f));
    pulley->SetOwnerBodyAnchor(Vector2::ZERO);
    pulley->SetOtherBodyAnchor(Vector2::ZERO);
    pulley->SetRatio(2.0f);

    REQUIRE(pulley->GetOwnerBodyGroundAnchor() == Vector2(-1.0f, 5.0f));
    REQUIRE(pulley->GetOtherBodyGroundAnchor() == Vector2(1.0f, 5.0f));
    REQUIRE_EQ_F(pulley->GetRatio(), 2.0f);

    SECTION("a gear chains two other constraints")
    {
        RigidBody2D* third = fixture.MakeBody(Vector2(5.0f, 0.0f), BT_DYNAMIC);

        auto* first = fixture.hanging_->GetNode()->CreateComponent<ConstraintRevolute2D>();
        first->SetOtherBody(fixture.anchor_);
        first->SetAnchor(Vector2::ZERO);

        auto* second = third->GetNode()->CreateComponent<ConstraintRevolute2D>();
        second->SetOtherBody(fixture.anchor_);
        second->SetAnchor(Vector2(5.0f, 0.0f));

        auto* gear = third->GetNode()->CreateComponent<ConstraintGear2D>();
        gear->SetOtherBody(fixture.hanging_);
        gear->SetOwnerConstraint(second);
        gear->SetOtherConstraint(first);
        gear->SetRatio(1.5f);

        REQUIRE(gear->GetOwnerConstraint() == second);
        REQUIRE(gear->GetOtherConstraint() == first);
        REQUIRE_EQ_F(gear->GetRatio(), 1.5f);
        REQUIRE(gear->GetJoint() != nullptr);

        fixture.Step(30);

        third->GetNode()->RemoveComponent(gear);
    }
}

TEST_CASE("ConstraintMouse2D drags a body towards a target", "[Physics2D]")
{
    JointFixture fixture;
    auto* constraint = Attach<ConstraintMouse2D>(fixture);

    constraint->SetTarget(Vector2(5.0f, 5.0f));
    constraint->SetMaxForce(1000.0f);
    constraint->SetFrequencyHz(5.0f);
    constraint->SetDampingRatio(0.7f);

    REQUIRE(constraint->GetTarget() == Vector2(5.0f, 5.0f));
    REQUIRE_EQ_F(constraint->GetMaxForce(), 1000.0f);
    REQUIRE_EQ_F(constraint->GetFrequencyHz(), 5.0f);
    REQUIRE_EQ_F(constraint->GetDampingRatio(), 0.7f);

    const float startDistance = (fixture.hanging_->GetNode()->GetWorldPosition2D() - Vector2(5.0f, 5.0f)).Length();
    fixture.Step(120);
    const float endDistance = (fixture.hanging_->GetNode()->GetWorldPosition2D() - Vector2(5.0f, 5.0f)).Length();

    REQUIRE(endDistance < startDistance);
}

TEST_CASE("PhysicsWorld2D raycasts and queries its bodies", "[Physics2D]")
{
    JointFixture fixture;
    fixture.hanging_->GetNode()->SetPosition(Vector3(0.0f, -3.0f, 0.0f));
    fixture.hanging_->SetBodyType(BT_STATIC);

    PhysicsRaycastResult2D result;
    fixture.world_->RaycastSingle(result, Vector2(-10.0f, 0.0f), Vector2(10.0f, 0.0f));
    REQUIRE(result.body_ == fixture.anchor_);
    REQUIRE(result.distance_ > 0.0f);

    SECTION("a ray that misses reports no body")
    {
        PhysicsRaycastResult2D miss;
        fixture.world_->RaycastSingle(miss, Vector2(-10.0f, 50.0f), Vector2(10.0f, 50.0f));
        REQUIRE(miss.body_ == nullptr);
    }

    SECTION("the multi hit form collects both bodies")
    {
        PODVector<PhysicsRaycastResult2D> results;
        fixture.world_->Raycast(results, Vector2(0.0f, 5.0f), Vector2(0.0f, -5.0f));
        REQUIRE(results.Size() == 2);
    }

    SECTION("a point query finds the body under it")
    {
        REQUIRE(fixture.world_->GetRigidBody(Vector2::ZERO) == fixture.anchor_);
        REQUIRE(fixture.world_->GetRigidBody(Vector2(100.0f, 100.0f)) == nullptr);
    }

    SECTION("a box query collects everything it overlaps")
    {
        PODVector<RigidBody2D*> bodies;
        fixture.world_->GetRigidBodies(bodies, Rect(-10.0f, -10.0f, 10.0f, 10.0f));
        REQUIRE(bodies.Size() == 2);

        fixture.world_->GetRigidBodies(bodies, Rect(50.0f, 50.0f, 60.0f, 60.0f));
        REQUIRE(bodies.Empty());
    }
}

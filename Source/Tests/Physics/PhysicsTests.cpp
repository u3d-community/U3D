#include "TestUtils.h"

#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Math/Ray.h>
#include <Urho3D/Physics/CollisionShape.h>
#include <Urho3D/Physics/Constraint.h>
#include <Urho3D/Physics/PhysicsWorld.h>
#include <Urho3D/Physics/RigidBody.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/Scene.h>

using namespace Urho3D;
using namespace U3DTest;

namespace
{

SharedPtr<Scene> MakePhysicsScene(PhysicsWorld*& world)
{
    SharedPtr<Scene> scene(new Scene(HeadlessContext()));
    world = scene->CreateComponent<PhysicsWorld>();
    world->SetFps(60);
    return scene;
}

RigidBody* MakeBox(Scene* scene, const Vector3& position, float mass)
{
    Node* node = scene->CreateChild();
    node->SetPosition(position);
    auto* body = node->CreateComponent<RigidBody>();
    body->SetMass(mass);
    node->CreateComponent<CollisionShape>()->SetBox(Vector3::ONE);
    return body;
}

}

TEST_CASE("PhysicsWorld clamps and reports its settings", "[Physics]")
{
    PhysicsWorld* world = nullptr;
    SharedPtr<Scene> scene = MakePhysicsScene(world);

    REQUIRE(world->GetGravity() == Vector3(0.0f, -9.81f, 0.0f));
    world->SetGravity(Vector3(0.0f, -20.0f, 0.0f));
    REQUIRE(world->GetGravity() == Vector3(0.0f, -20.0f, 0.0f));

    world->SetFps(0);
    REQUIRE(world->GetFps() == 1);
    world->SetFps(120);
    REQUIRE(world->GetFps() == 120);

    world->SetMaxSubSteps(3);
    REQUIRE(world->GetMaxSubSteps() == 3);

    world->SetNumIterations(0);
    REQUIRE(world->GetNumIterations() == 1);
    world->SetNumIterations(12);
    REQUIRE(world->GetNumIterations() == 12);

    world->SetInterpolation(false);
    world->SetInternalEdge(false);
    world->SetSplitImpulse(true);
    world->SetUpdateEnabled(false);
    REQUIRE_FALSE(world->GetInterpolation());
    REQUIRE_FALSE(world->GetInternalEdge());
    REQUIRE(world->GetSplitImpulse());
    REQUIRE_FALSE(world->IsUpdateEnabled());
    REQUIRE_FALSE(world->IsSimulating());
    REQUIRE(world->GetWorld() != nullptr);
}

TEST_CASE("RigidBody properties round trip through Bullet", "[Physics]")
{
    PhysicsWorld* world = nullptr;
    SharedPtr<Scene> scene = MakePhysicsScene(world);
    RigidBody* body = MakeBox(scene, Vector3::ZERO, 2.0f);

    REQUIRE(body->GetBody() != nullptr);
    REQUIRE(body->GetPhysicsWorld() == world);
    REQUIRE_EQ_F(body->GetMass(), 2.0f);

    body->SetLinearVelocity(Vector3(1.0f, 2.0f, 3.0f));
    REQUIRE(body->GetLinearVelocity().Equals(Vector3(1.0f, 2.0f, 3.0f)));

    body->SetAngularVelocity(Vector3(0.0f, 4.0f, 0.0f));
    REQUIRE(body->GetAngularVelocity().Equals(Vector3(0.0f, 4.0f, 0.0f)));

    body->SetLinearFactor(Vector3(1.0f, 0.0f, 1.0f));
    body->SetAngularFactor(Vector3(0.0f, 1.0f, 0.0f));
    REQUIRE(body->GetLinearFactor() == Vector3(1.0f, 0.0f, 1.0f));
    REQUIRE(body->GetAngularFactor() == Vector3(0.0f, 1.0f, 0.0f));

    body->SetLinearDamping(0.25f);
    body->SetAngularDamping(0.5f);
    REQUIRE_NEAR(body->GetLinearDamping(), 0.25f, 0.001f);
    REQUIRE_NEAR(body->GetAngularDamping(), 0.5f, 0.001f);

    body->SetFriction(0.75f);
    body->SetRestitution(0.4f);
    body->SetRollingFriction(0.1f);
    REQUIRE_NEAR(body->GetFriction(), 0.75f, 0.001f);
    REQUIRE_NEAR(body->GetRestitution(), 0.4f, 0.001f);
    REQUIRE_NEAR(body->GetRollingFriction(), 0.1f, 0.001f);

    body->SetCollisionLayerAndMask(0x2, 0x6);
    REQUIRE(body->GetCollisionLayer() == 0x2);
    REQUIRE(body->GetCollisionMask() == 0x6);

    SECTION("a zero mass body is static and does not fall")
    {
        body->SetMass(0.0f);
        REQUIRE_EQ_F(body->GetMass(), 0.0f);
        const Vector3 before = body->GetPosition();
        world->Update(0.1f);
        REQUIRE(body->GetPosition().Equals(before));
    }

    SECTION("a kinematic body is driven by its node rather than by gravity")
    {
        body->SetKinematic(true);
        REQUIRE(body->IsKinematic());
        body->GetNode()->SetPosition(Vector3(0.0f, 5.0f, 0.0f));
        world->Update(0.1f);
        REQUIRE(body->GetPosition().Equals(Vector3(0.0f, 5.0f, 0.0f)));
    }

    SECTION("a trigger reports itself as one")
    {
        body->SetTrigger(true);
        REQUIRE(body->IsTrigger());
    }

    SECTION("gravity can be disabled per body")
    {
        body->SetUseGravity(false);
        REQUIRE_FALSE(body->GetUseGravity());
        body->SetLinearVelocity(Vector3::ZERO);
        const float before = body->GetPosition().y_;
        world->Update(0.1f);
        REQUIRE_NEAR(body->GetPosition().y_, before, 0.001f);
    }
}

TEST_CASE("RigidBody falls under gravity and responds to forces", "[Physics]")
{
    PhysicsWorld* world = nullptr;
    SharedPtr<Scene> scene = MakePhysicsScene(world);
    RigidBody* body = MakeBox(scene, Vector3(0.0f, 10.0f, 0.0f), 1.0f);

    for (int i = 0; i < 10; ++i)
        world->Update(1.0f / 60.0f);

    REQUIRE(body->GetPosition().y_ < 10.0f);
    REQUIRE(body->GetLinearVelocity().y_ < 0.0f);

    SECTION("an impulse changes the velocity immediately")
    {
        body->SetLinearVelocity(Vector3::ZERO);
        body->ApplyImpulse(Vector3(5.0f, 0.0f, 0.0f));
        REQUIRE(body->GetLinearVelocity().x_ > 0.0f);
    }

    SECTION("a torque impulse starts a rotation")
    {
        body->SetAngularVelocity(Vector3::ZERO);
        body->ApplyTorqueImpulse(Vector3(0.0f, 2.0f, 0.0f));
        REQUIRE(body->GetAngularVelocity().y_ > 0.0f);
    }

    SECTION("a continuous force only takes effect once the world steps")
    {
        body->SetLinearVelocity(Vector3::ZERO);
        body->ApplyForce(Vector3(100.0f, 0.0f, 0.0f));
        REQUIRE_EQ_F(body->GetLinearVelocity().x_, 0.0f);
        world->Update(1.0f / 60.0f);
        REQUIRE(body->GetLinearVelocity().x_ > 0.0f);
    }

    SECTION("Activate wakes a body that has come to rest")
    {
        body->SetLinearVelocity(Vector3::ZERO);
        body->SetAngularVelocity(Vector3::ZERO);
        for (int i = 0; i < 200; ++i)
            world->Update(1.0f / 60.0f);
        body->Activate();
        REQUIRE(body->IsActive());
    }
}

TEST_CASE("A falling body comes to rest on a static floor", "[Physics]")
{
    PhysicsWorld* world = nullptr;
    SharedPtr<Scene> scene = MakePhysicsScene(world);

    Node* floorNode = scene->CreateChild("Floor");
    floorNode->SetPosition(Vector3(0.0f, -0.5f, 0.0f));
    floorNode->CreateComponent<RigidBody>();
    floorNode->CreateComponent<CollisionShape>()->SetBox(Vector3(100.0f, 1.0f, 100.0f));

    RigidBody* box = MakeBox(scene, Vector3(0.0f, 5.0f, 0.0f), 1.0f);

    for (int i = 0; i < 400; ++i)
        world->Update(1.0f / 60.0f);

    REQUIRE_NEAR(box->GetPosition().y_, 0.5f, 0.05f);
    REQUIRE_NEAR(box->GetLinearVelocity().Length(), 0.0f, 0.05f);
}

TEST_CASE("PhysicsWorld raycasts against a body", "[Physics]")
{
    PhysicsWorld* world = nullptr;
    SharedPtr<Scene> scene = MakePhysicsScene(world);
    RigidBody* target = MakeBox(scene, Vector3(0.0f, 0.0f, 10.0f), 0.0f);
    target->SetCollisionLayer(0x2);

    PhysicsRaycastResult result;
    world->RaycastSingle(result, Ray(Vector3::ZERO, Vector3::FORWARD), 100.0f);
    REQUIRE(result.body_ == target);
    REQUIRE_NEAR(result.distance_, 9.5f, 0.1f);
    REQUIRE(result.normal_.z_ < 0.0f);

    SECTION("a ray pointing away misses")
    {
        PhysicsRaycastResult miss;
        world->RaycastSingle(miss, Ray(Vector3::ZERO, -Vector3::FORWARD), 100.0f);
        REQUIRE(miss.body_ == nullptr);
        REQUIRE(miss.distance_ == M_INFINITY);
    }

    SECTION("a ray that stops short of the body misses")
    {
        PhysicsRaycastResult tooShort;
        world->RaycastSingle(tooShort, Ray(Vector3::ZERO, Vector3::FORWARD), 5.0f);
        REQUIRE(tooShort.body_ == nullptr);
    }

    SECTION("the collision mask filters the hit out")
    {
        PhysicsRaycastResult filtered;
        world->RaycastSingle(filtered, Ray(Vector3::ZERO, Vector3::FORWARD), 100.0f, 0x1);
        REQUIRE(filtered.body_ == nullptr);
    }

    SECTION("the segmented form finds the same body")
    {
        PhysicsRaycastResult segmented;
        world->RaycastSingleSegmented(segmented, Ray(Vector3::ZERO, Vector3::FORWARD), 100.0f, 4.0f);
        REQUIRE(segmented.body_ == target);
        REQUIRE_NEAR(segmented.distance_, 9.5f, 0.1f);
    }

    SECTION("the multi hit form collects every body along the ray")
    {
        MakeBox(scene, Vector3(0.0f, 0.0f, 20.0f), 0.0f);
        PODVector<PhysicsRaycastResult> results;
        world->Raycast(results, Ray(Vector3::ZERO, Vector3::FORWARD), 100.0f);
        REQUIRE(results.Size() == 2);
        REQUIRE(results[0].distance_ < results[1].distance_);
    }
}

TEST_CASE("PhysicsWorld queries bodies by volume", "[Physics]")
{
    PhysicsWorld* world = nullptr;
    SharedPtr<Scene> scene = MakePhysicsScene(world);
    RigidBody* near = MakeBox(scene, Vector3::ZERO, 0.0f);
    MakeBox(scene, Vector3(50.0f, 0.0f, 0.0f), 0.0f);

    PODVector<RigidBody*> result;
    world->GetRigidBodies(result, Sphere(Vector3::ZERO, 2.0f));
    REQUIRE(result.Size() == 1);
    REQUIRE(result[0] == near);

    world->GetRigidBodies(result, BoundingBox(Vector3(-100.0f, -100.0f, -100.0f), Vector3(100.0f, 100.0f, 100.0f)));
    REQUIRE(result.Size() == 2);

    SECTION("a volume containing nothing returns an empty result")
    {
        PODVector<RigidBody*> empty;
        world->GetRigidBodies(empty, Sphere(Vector3(0.0f, 500.0f, 0.0f), 1.0f));
        REQUIRE(empty.Empty());
    }
}

TEST_CASE("CollisionShape builds each primitive and reports its bounds", "[Physics]")
{
    PhysicsWorld* world = nullptr;
    SharedPtr<Scene> scene = MakePhysicsScene(world);
    Node* node = scene->CreateChild();
    auto* shape = node->CreateComponent<CollisionShape>();

    shape->SetBox(Vector3(2.0f, 4.0f, 6.0f));
    REQUIRE(shape->GetShapeType() == SHAPE_BOX);
    REQUIRE(shape->GetSize() == Vector3(2.0f, 4.0f, 6.0f));
    REQUIRE(shape->GetCollisionShape() != nullptr);

    const BoundingBox bounds = shape->GetWorldBoundingBox();
    REQUIRE(bounds.Size().x_ >= 2.0f);
    REQUIRE(bounds.Size().y_ >= 4.0f);
    REQUIRE(bounds.Size().z_ >= 6.0f);

    shape->SetSphere(3.0f);
    REQUIRE(shape->GetShapeType() == SHAPE_SPHERE);
    REQUIRE_EQ_F(shape->GetSize().x_, 3.0f);

    shape->SetCapsule(2.0f, 5.0f);
    REQUIRE(shape->GetShapeType() == SHAPE_CAPSULE);
    REQUIRE(shape->GetSize() == Vector3(2.0f, 5.0f, 2.0f));

    shape->SetCylinder(2.0f, 5.0f);
    REQUIRE(shape->GetShapeType() == SHAPE_CYLINDER);

    shape->SetCone(2.0f, 5.0f);
    REQUIRE(shape->GetShapeType() == SHAPE_CONE);

    shape->SetStaticPlane();
    REQUIRE(shape->GetShapeType() == SHAPE_STATICPLANE);

    SECTION("the local transform is kept separately from the node transform")
    {
        shape->SetBox(Vector3::ONE, Vector3(1.0f, 2.0f, 3.0f), Quaternion(90.0f, Vector3::UP));
        REQUIRE(shape->GetPosition() == Vector3(1.0f, 2.0f, 3.0f));
        REQUIRE(shape->GetRotation().Equals(Quaternion(90.0f, Vector3::UP)));

        shape->SetTransform(Vector3::ZERO, Quaternion::IDENTITY);
        REQUIRE(shape->GetPosition() == Vector3::ZERO);
    }

    SECTION("the node scale is folded into the world bounding box")
    {
        shape->SetBox(Vector3::ONE);
        const Vector3 unscaled = shape->GetWorldBoundingBox().Size();
        node->SetScale(4.0f);
        REQUIRE(shape->GetWorldBoundingBox().Size().x_ > unscaled.x_);
    }

    SECTION("the margin is settable and clamped to non negative")
    {
        shape->SetMargin(0.1f);
        REQUIRE_NEAR(shape->GetMargin(), 0.1f, 0.001f);
        shape->SetMargin(-1.0f);
        REQUIRE(shape->GetMargin() >= 0.0f);
    }
}

TEST_CASE("CollisionShape builds a triangle mesh and a convex hull from a model", "[Physics]")
{
    Context* context = HeadlessContext();
    auto* model = context->GetSubsystem<ResourceCache>()->GetResource<Model>("Models/Box.mdl");
    REQUIRE(model != nullptr);

    PhysicsWorld* world = nullptr;
    SharedPtr<Scene> scene = MakePhysicsScene(world);
    Node* node = scene->CreateChild();
    auto* shape = node->CreateComponent<CollisionShape>();

    shape->SetTriangleMesh(model);
    REQUIRE(shape->GetShapeType() == SHAPE_TRIANGLEMESH);
    REQUIRE(shape->GetModel() == model);
    REQUIRE(shape->GetCollisionShape() != nullptr);
    REQUIRE(shape->GetGeometryData() != nullptr);

    shape->SetConvexHull(model);
    REQUIRE(shape->GetShapeType() == SHAPE_CONVEXHULL);
    REQUIRE(shape->GetCollisionShape() != nullptr);

    SECTION("the geometry cache hands the same data to a second shape")
    {
        Node* second = scene->CreateChild();
        auto* other = second->CreateComponent<CollisionShape>();
        other->SetTriangleMesh(model);
        shape->SetTriangleMesh(model);
        REQUIRE(other->GetGeometryData() == shape->GetGeometryData());
    }

    SECTION("a null model is refused and leaves the previous shape in place")
    {
        const ShapeType before = shape->GetShapeType();
        shape->SetTriangleMesh(nullptr);
        REQUIRE(shape->GetShapeType() == before);
        REQUIRE(shape->GetModel() == model);
        REQUIRE(shape->GetCollisionShape() != nullptr);
    }
}

TEST_CASE("Constraint links two bodies", "[Physics]")
{
    PhysicsWorld* world = nullptr;
    SharedPtr<Scene> scene = MakePhysicsScene(world);

    RigidBody* anchor = MakeBox(scene, Vector3::ZERO, 0.0f);
    RigidBody* hanging = MakeBox(scene, Vector3(0.0f, -2.0f, 0.0f), 1.0f);

    auto* constraint = hanging->GetNode()->CreateComponent<Constraint>();
    constraint->SetConstraintType(CONSTRAINT_POINT);
    constraint->SetOtherBody(anchor);
    constraint->SetPosition(Vector3(0.0f, 1.0f, 0.0f));
    constraint->SetOtherPosition(Vector3(0.0f, -1.0f, 0.0f));

    REQUIRE(constraint->GetConstraintType() == CONSTRAINT_POINT);
    REQUIRE(constraint->GetOtherBody() == anchor);
    REQUIRE(constraint->GetOwnBody() == hanging);
    REQUIRE(constraint->GetConstraint() != nullptr);
    REQUIRE(constraint->GetPosition() == Vector3(0.0f, 1.0f, 0.0f));

    SECTION("the constrained body stays near its anchor while gravity pulls on it")
    {
        for (int i = 0; i < 120; ++i)
            world->Update(1.0f / 60.0f);

        REQUIRE(hanging->GetPosition().Length() < 6.0f);
    }

    SECTION("a hinge exposes its limits")
    {
        constraint->SetConstraintType(CONSTRAINT_HINGE);
        constraint->SetHighLimit(Vector2(45.0f, 0.0f));
        constraint->SetLowLimit(Vector2(-45.0f, 0.0f));
        REQUIRE(constraint->GetConstraintType() == CONSTRAINT_HINGE);
        REQUIRE_EQ_F(constraint->GetHighLimit().x_, 45.0f);
        REQUIRE_EQ_F(constraint->GetLowLimit().x_, -45.0f);
    }

    SECTION("disabling collision between the pair is remembered")
    {
        constraint->SetDisableCollision(true);
        REQUIRE(constraint->GetDisableCollision());
    }

    SECTION("removing the other body leaves the constraint attached to the world")
    {
        constraint->SetOtherBody(nullptr);
        REQUIRE(constraint->GetOtherBody() == nullptr);
        REQUIRE(constraint->GetConstraint() != nullptr);
    }
}

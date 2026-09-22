#include "TestUtils.h"
#include <Urho3D/Physics2D/CollisionBox2D.h>
#include <Urho3D/Physics2D/CollisionChain2D.h>
#include <Urho3D/Physics2D/CollisionCircle2D.h>
#include <Urho3D/Physics2D/CollisionEdge2D.h>
#include <Urho3D/Physics2D/CollisionPolygon2D.h>
#include <Urho3D/Physics2D/PhysicsUtils2D.h>
#include <Urho3D/Physics2D/PhysicsWorld2D.h>
#include <Urho3D/Physics2D/RigidBody2D.h>
#include <Urho3D/Scene/Scene.h>

using namespace Urho3D;
using namespace U3DTest;

TEST_CASE("Physics2D conversion helpers preserve coordinates", "[Physics2D]")
{
    const Vector2 vector(1.25f, -2.5f);
    REQUIRE(ToVector2(ToB2Vec2(vector)) == vector);
    REQUIRE(ToVector3(ToB2Vec2(Vector3(3.0f, 4.0f, 5.0f))) == Vector3(3.0f, 4.0f, 0.0f));
    REQUIRE(ToColor(b2Color(0.1f, 0.2f, 0.3f)) == Color(0.1f, 0.2f, 0.3f));
}

TEST_CASE("CollisionShape2D properties and geometry round trip", "[Physics2D]")
{
    Context* context = HeadlessContext();
    SharedPtr<CollisionBox2D> box(new CollisionBox2D(context));
    box->SetSize(3.0f, 4.0f);
    box->SetCenter(1.0f, 2.0f);
    box->SetAngle(30.0f);
    box->SetTrigger(true);
    box->SetCategoryBits(0x12);
    box->SetMaskBits(0x34);
    box->SetGroupIndex(-2);
    box->SetDensity(2.5f);
    box->SetFriction(0.4f);
    box->SetRestitution(0.6f);
    REQUIRE(box->GetSize() == Vector2(3.0f, 4.0f));
    REQUIRE(box->GetCenter() == Vector2(1.0f, 2.0f));
    REQUIRE_EQ_F(box->GetAngle(), 30.0f);
    REQUIRE(box->IsTrigger());
    REQUIRE(box->GetCategoryBits() == 0x12);
    REQUIRE(box->GetMaskBits() == 0x34);
    REQUIRE(box->GetGroupIndex() == -2);
    REQUIRE_EQ_F(box->GetDensity(), 2.5f);
    REQUIRE_EQ_F(box->GetFriction(), 0.4f);
    REQUIRE_EQ_F(box->GetRestitution(), 0.6f);

    CollisionCircle2D circle(context);
    circle.SetRadius(2.0f);
    circle.SetCenter(-1.0f, 3.0f);
    REQUIRE_EQ_F(circle.GetRadius(), 2.0f);
    REQUIRE(circle.GetCenter() == Vector2(-1.0f, 3.0f));
    CollisionEdge2D edge(context);
    edge.SetVertices(Vector2::ZERO, Vector2::ONE);
    REQUIRE(edge.GetVertex1() == Vector2::ZERO);
    REQUIRE(edge.GetVertex2() == Vector2::ONE);
}

TEST_CASE("Polygon and chain vertex attributes round trip", "[Physics2D]")
{
    Context* context = HeadlessContext();
    PODVector<Vector2> vertices;
    vertices.Push(Vector2(0, 0));
    vertices.Push(Vector2(2, 0));
    vertices.Push(Vector2(0, 2));
    CollisionPolygon2D polygon(context);
    polygon.SetVertices(vertices);
    REQUIRE(polygon.GetVertexCount() == 3);
    REQUIRE(polygon.GetVertex(99) == Vector2::ZERO);
    const PODVector<unsigned char> encoded = polygon.GetVerticesAttr();
    CollisionPolygon2D restored(context);
    restored.SetVerticesAttr(encoded);
    REQUIRE(restored.GetVertices() == vertices);

    CollisionChain2D chain(context);
    chain.SetLoop(true);
    chain.SetVertices(vertices);
    REQUIRE(chain.GetLoop());
    CollisionChain2D restoredChain(context);
    restoredChain.SetVerticesAttr(chain.GetVerticesAttr());
    REQUIRE(restoredChain.GetVertices() == vertices);
}

TEST_CASE("PhysicsWorld2D simulates a dynamic body", "[Physics2D]")
{
    SharedPtr<Scene> scene(new Scene(HeadlessContext()));
    PhysicsWorld2D* world = scene->CreateComponent<PhysicsWorld2D>();
    world->SetGravity(Vector2(0.0f, -10.0f));
    world->SetVelocityIterations(6);
    world->SetPositionIterations(3);
    REQUIRE(world->GetGravity() == Vector2(0.0f, -10.0f));
    REQUIRE(world->GetVelocityIterations() == 6);
    REQUIRE(world->GetPositionIterations() == 3);
    Node* node = scene->CreateChild("Body");
    RigidBody2D* body = node->CreateComponent<RigidBody2D>();
    body->SetBodyType(BT_DYNAMIC);
    node->CreateComponent<CollisionBox2D>()->SetSize(1.0f, 1.0f);
    REQUIRE(body->GetBody() != nullptr);
    const float initialY = node->GetWorldPosition().y_;
    world->Update(0.1f);
    REQUIRE(node->GetWorldPosition().y_ < initialY);
}

TEST_CASE("RigidBody2D force APIs are safe before attachment", "[Physics2D]")
{
    RigidBody2D body(HeadlessContext());
    body.ApplyForce(Vector2::ONE, Vector2::ZERO, true);
    body.ApplyForceToCenter(Vector2::ONE, true);
    body.ApplyTorque(1.0f, true);
    REQUIRE(body.GetBody() == nullptr);
}

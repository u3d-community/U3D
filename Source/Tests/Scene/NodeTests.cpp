#include "TestUtils.h"

#include <Urho3D/IO/VectorBuffer.h>
#include <Urho3D/Resource/JSONFile.h>
#include <Urho3D/Resource/XMLFile.h>
#include <Urho3D/Scene/Node.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/SplinePath.h>
#include <Urho3D/Scene/ValueAnimation.h>

using namespace Urho3D;
using namespace U3DTest;

namespace
{

SharedPtr<Scene> MakeScene()
{
    return SharedPtr<Scene>(new Scene(HeadlessContext()));
}

}

TEST_CASE("Node hierarchy", "[Scene]")
{
    SharedPtr<Scene> scene = MakeScene();

    Node* parent = scene->CreateChild("Parent");
    REQUIRE(parent != nullptr);
    REQUIRE(parent->GetName() == "Parent");
    REQUIRE(parent->GetParent() == scene.Get());
    REQUIRE(scene->GetNumChildren() == 1);

    Node* child = parent->CreateChild("Child");
    REQUIRE(child->GetParent() == parent);
    REQUIRE(parent->GetNumChildren() == 1);
    REQUIRE(scene->GetNumChildren(true) == 2);

    REQUIRE(parent->GetChild("Child") == child);
    REQUIRE(parent->GetChild("Missing") == nullptr);
    REQUIRE(scene->GetChild("Child", true) == child);
    REQUIRE(scene->GetChild("Child", false) == nullptr);

    SECTION("nodes get unique ids and can be looked up by them")
    {
        REQUIRE(parent->GetID() != 0);
        REQUIRE(parent->GetID() != child->GetID());
        REQUIRE(scene->GetNode(child->GetID()) == child);
        REQUIRE(scene->GetNode(0) == nullptr);
    }

    SECTION("reparenting moves the node")
    {
        Node* other = scene->CreateChild("Other");
        child->SetParent(other);
        REQUIRE(child->GetParent() == other);
        REQUIRE(parent->GetNumChildren() == 0);
        REQUIRE(other->GetNumChildren() == 1);
    }

    SECTION("removing a child detaches the whole subtree")
    {
        const unsigned childId = child->GetID();
        parent->RemoveChild(child);
        REQUIRE(parent->GetNumChildren() == 0);
        REQUIRE(scene->GetNode(childId) == nullptr);
    }

    SECTION("RemoveAllChildren clears the subtree")
    {
        scene->RemoveAllChildren();
        REQUIRE(scene->GetNumChildren() == 0);
    }

    SECTION("a node knows its scene")
    {
        REQUIRE(child->GetScene() == scene.Get());
    }
}

TEST_CASE("Node transforms", "[Scene]")
{
    SharedPtr<Scene> scene = MakeScene();
    Node* node = scene->CreateChild("Node");

    node->SetPosition(Vector3(1.0f, 2.0f, 3.0f));
    REQUIRE(node->GetPosition() == Vector3(1.0f, 2.0f, 3.0f));

    node->SetRotation(Quaternion(90.0f, Vector3::UP));
    REQUIRE(node->GetRotation().Equals(Quaternion(90.0f, Vector3::UP)));

    node->SetScale(2.0f);
    REQUIRE(node->GetScale() == Vector3(2.0f, 2.0f, 2.0f));

    node->SetScale(Vector3(1.0f, 2.0f, 3.0f));
    REQUIRE(node->GetScale() == Vector3(1.0f, 2.0f, 3.0f));

    SECTION("world transform composes with the parent")
    {
        Node* child = node->CreateChild("Child");
        node->SetScale(1.0f);
        node->SetRotation(Quaternion::IDENTITY);
        node->SetPosition(Vector3(10.0f, 0.0f, 0.0f));
        child->SetPosition(Vector3(1.0f, 0.0f, 0.0f));

        REQUIRE(child->GetWorldPosition().Equals(Vector3(11.0f, 0.0f, 0.0f)));
        REQUIRE(child->GetPosition().Equals(Vector3(1.0f, 0.0f, 0.0f)));
    }

    SECTION("setting a world position back solves the local one")
    {
        Node* child = node->CreateChild("Child");
        node->SetPosition(Vector3(5.0f, 0.0f, 0.0f));
        node->SetScale(1.0f);
        node->SetRotation(Quaternion::IDENTITY);

        child->SetWorldPosition(Vector3(7.0f, 0.0f, 0.0f));
        REQUIRE(child->GetWorldPosition().Equals(Vector3(7.0f, 0.0f, 0.0f)));
        REQUIRE(child->GetPosition().Equals(Vector3(2.0f, 0.0f, 0.0f)));
    }

    SECTION("Translate and Rotate accumulate")
    {
        Node* mover = scene->CreateChild("Mover");
        mover->SetPosition(Vector3::ZERO);
        mover->Translate(Vector3(1.0f, 0.0f, 0.0f));
        mover->Translate(Vector3(1.0f, 0.0f, 0.0f));
        REQUIRE(mover->GetPosition().Equals(Vector3(2.0f, 0.0f, 0.0f)));

        mover->SetRotation(Quaternion::IDENTITY);
        mover->Rotate(Quaternion(45.0f, Vector3::UP));
        mover->Rotate(Quaternion(45.0f, Vector3::UP));
        REQUIRE(mover->GetRotation().Equals(Quaternion(90.0f, Vector3::UP)));
    }

    SECTION("direction conversion round trips through world space")
    {
        Node* rotated = scene->CreateChild("Rotated");
        rotated->SetRotation(Quaternion(90.0f, Vector3::UP));
        const Vector3 local(0.0f, 0.0f, 1.0f);
        REQUIRE(rotated->WorldToLocal(rotated->LocalToWorld(local)).Equals(local));
    }

    SECTION("LookAt orients the node towards a target")
    {
        Node* looker = scene->CreateChild("Looker");
        looker->SetPosition(Vector3::ZERO);
        REQUIRE(looker->LookAt(Vector3(0.0f, 0.0f, 10.0f)));
        REQUIRE(looker->GetWorldDirection().Equals(Vector3::FORWARD));
    }
}

TEST_CASE("Node enabling cascades", "[Scene]")
{
    SharedPtr<Scene> scene = MakeScene();
    Node* parent = scene->CreateChild("Parent");
    Node* child = parent->CreateChild("Child");

    REQUIRE(parent->IsEnabled());
    REQUIRE(child->IsEnabled());

    parent->SetEnabled(false);
    REQUIRE_FALSE(parent->IsEnabled());
    REQUIRE_FALSE(parent->IsEnabledSelf());

    SECTION("recursive disable also disables the children")
    {
        parent->SetEnabled(true);
        parent->SetEnabledRecursive(false);
        REQUIRE_FALSE(child->IsEnabled());

        parent->SetEnabledRecursive(true);
        REQUIRE(child->IsEnabled());
    }
}

TEST_CASE("Node variables", "[Scene]")
{
    SharedPtr<Scene> scene = MakeScene();
    Node* node = scene->CreateChild("Node");

    node->SetVar("health", 100);
    REQUIRE(node->GetVar("health").GetInt() == 100);
    REQUIRE(node->GetVar("missing").IsEmpty());
    REQUIRE(node->GetVars().Size() == 1);

    node->SetVar("position", Vector3::ONE);
    REQUIRE(node->GetVar("position").GetVector3() == Vector3::ONE);
    REQUIRE(node->GetVars().Size() == 2);

    SECTION("assigning an empty variant clears the slot's value but keeps the key")
    {
        node->SetVar("health", Variant::EMPTY);
        REQUIRE(node->GetVar("health").IsEmpty());
        REQUIRE(node->GetVars().Size() == 2);
    }

    SECTION("overwriting a variable replaces its value and type")
    {
        node->SetVar("health", "full");
        REQUIRE(node->GetVar("health").GetString() == "full");
        REQUIRE(node->GetVars().Size() == 2);
    }
}

TEST_CASE("Node tags", "[Scene]")
{
    SharedPtr<Scene> scene = MakeScene();
    Node* node = scene->CreateChild("Node");

    node->AddTag("enemy");
    REQUIRE(node->HasTag("enemy"));
    REQUIRE_FALSE(node->HasTag("friend"));
    REQUIRE(node->GetTags().Size() == 1);

    node->AddTag("flying");
    REQUIRE(node->GetTags().Size() == 2);

    PODVector<Node*> tagged;
    scene->GetNodesWithTag(tagged, "enemy");
    REQUIRE(tagged.Size() == 1);
    REQUIRE(tagged[0] == node);

    REQUIRE(node->RemoveTag("enemy"));
    REQUIRE_FALSE(node->HasTag("enemy"));
    REQUIRE_FALSE(node->RemoveTag("nonexistent"));

    node->RemoveAllTags();
    REQUIRE(node->GetTags().Empty());
}

TEST_CASE("Node cloning", "[Scene]")
{
    SharedPtr<Scene> scene = MakeScene();
    Node* original = scene->CreateChild("Original");
    original->SetPosition(Vector3(1.0f, 2.0f, 3.0f));
    original->SetVar("tag", 7);
    original->CreateChild("Sub");

    Node* clone = original->Clone();
    REQUIRE(clone != nullptr);
    REQUIRE(clone != original);
    REQUIRE(clone->GetName() == "Original");
    REQUIRE(clone->GetPosition() == Vector3(1.0f, 2.0f, 3.0f));
    REQUIRE(clone->GetVar("tag").GetInt() == 7);
    REQUIRE(clone->GetNumChildren() == 1);
    REQUIRE(clone->GetID() != original->GetID());

    SECTION("mutating the clone leaves the original alone")
    {
        clone->SetPosition(Vector3::ZERO);
        REQUIRE(original->GetPosition() == Vector3(1.0f, 2.0f, 3.0f));
    }
}

TEST_CASE("Scene binary serialisation round trip", "[Scene]")
{
    SharedPtr<Scene> source = MakeScene();
    Node* node = source->CreateChild("Saved");
    node->SetPosition(Vector3(1.0f, 2.0f, 3.0f));
    node->SetRotation(Quaternion(45.0f, Vector3::UP));
    node->SetVar("value", 42);
    node->AddTag("tagged");
    node->CreateChild("Nested");

    VectorBuffer buffer;
    REQUIRE(source->Save(buffer));
    REQUIRE(buffer.GetSize() > 0);

    buffer.Seek(0);
    SharedPtr<Scene> loaded = MakeScene();
    REQUIRE(loaded->Load(buffer));

    Node* restored = loaded->GetChild("Saved");
    REQUIRE(restored != nullptr);
    REQUIRE(restored->GetPosition().Equals(Vector3(1.0f, 2.0f, 3.0f)));
    REQUIRE(restored->GetRotation().Equals(Quaternion(45.0f, Vector3::UP)));
    REQUIRE(restored->GetVar("value").GetInt() == 42);
    REQUIRE(restored->HasTag("tagged"));
    REQUIRE(restored->GetChild("Nested") != nullptr);
}

TEST_CASE("Scene XML serialisation round trip", "[Scene]")
{
    SharedPtr<Scene> source = MakeScene();
    Node* node = source->CreateChild("XmlNode");
    node->SetPosition(Vector3(4.0f, 5.0f, 6.0f));
    node->SetVar("value", "text");

    VectorBuffer buffer;
    REQUIRE(source->SaveXML(buffer));

    buffer.Seek(0);
    SharedPtr<Scene> loaded = MakeScene();
    REQUIRE(loaded->LoadXML(buffer));

    Node* restored = loaded->GetChild("XmlNode");
    REQUIRE(restored != nullptr);
    REQUIRE(restored->GetPosition().Equals(Vector3(4.0f, 5.0f, 6.0f)));
    REQUIRE(restored->GetVar("value").GetString() == "text");
}

TEST_CASE("Scene JSON serialisation round trip", "[Scene]")
{
    SharedPtr<Scene> source = MakeScene();
    Node* node = source->CreateChild("JsonNode");
    node->SetPosition(Vector3(7.0f, 8.0f, 9.0f));

    VectorBuffer buffer;
    REQUIRE(source->SaveJSON(buffer));

    buffer.Seek(0);
    SharedPtr<Scene> loaded = MakeScene();
    REQUIRE(loaded->LoadJSON(buffer));

    Node* restored = loaded->GetChild("JsonNode");
    REQUIRE(restored != nullptr);
    REQUIRE(restored->GetPosition().Equals(Vector3(7.0f, 8.0f, 9.0f)));
}

TEST_CASE("Scene node id allocation", "[Scene]")
{
    SharedPtr<Scene> scene = MakeScene();

    Node* replicated = scene->CreateChild("Replicated", REPLICATED);
    Node* local = scene->CreateChild("Local", LOCAL);

    REQUIRE(replicated->GetID() < FIRST_LOCAL_ID);
    REQUIRE(local->GetID() >= FIRST_LOCAL_ID);
    REQUIRE(replicated->IsReplicated());
    REQUIRE_FALSE(local->IsReplicated());

    SECTION("Clear resets the scene")
    {
        scene->Clear();
        REQUIRE(scene->GetNumChildren() == 0);
    }
}

TEST_CASE("ValueAnimation interpolates keyframes", "[Scene]")
{
    SharedPtr<ValueAnimation> animation(new ValueAnimation(HeadlessContext()));

    animation->SetKeyFrame(0.0f, Variant(0.0f));
    animation->SetKeyFrame(1.0f, Variant(10.0f));

    REQUIRE(animation->GetValueType() == VAR_FLOAT);
    REQUIRE(animation->IsValid());
    REQUIRE_EQ_F(animation->GetBeginTime(), 0.0f);
    REQUIRE_EQ_F(animation->GetEndTime(), 1.0f);

    REQUIRE_EQ_F(animation->GetAnimationValue(0.0f).GetFloat(), 0.0f);
    REQUIRE_EQ_F(animation->GetAnimationValue(1.0f).GetFloat(), 10.0f);
    REQUIRE_NEAR(animation->GetAnimationValue(0.5f).GetFloat(), 5.0f, 0.001f);

    SECTION("past the last keyframe the value is held")
    {
        REQUIRE_EQ_F(animation->GetAnimationValue(99.0f).GetFloat(), 10.0f);
    }

    SECTION("before the first keyframe the value is extrapolated, not clamped")
    {
        REQUIRE_NEAR(animation->GetAnimationValue(-1.0f).GetFloat(), -10.0f, 0.001f);
        REQUIRE_NEAR(animation->GetAnimationValue(-0.5f).GetFloat(), -5.0f, 0.001f);
    }

    SECTION("vector values interpolate componentwise")
    {
        SharedPtr<ValueAnimation> vectorAnimation(new ValueAnimation(HeadlessContext()));
        vectorAnimation->SetKeyFrame(0.0f, Variant(Vector3::ZERO));
        vectorAnimation->SetKeyFrame(2.0f, Variant(Vector3(2.0f, 4.0f, 6.0f)));
        REQUIRE(vectorAnimation->GetAnimationValue(1.0f).GetVector3().Equals(Vector3(1.0f, 2.0f, 3.0f)));
    }
}

TEST_CASE("SplinePath interpolates through control points", "[Scene]")
{
    SharedPtr<Scene> scene = MakeScene();
    Node* node = scene->CreateChild("Path");
    auto* spline = node->CreateComponent<SplinePath>();

    Node* first = scene->CreateChild("P0");
    first->SetPosition(Vector3::ZERO);
    Node* second = scene->CreateChild("P1");
    second->SetPosition(Vector3(10.0f, 0.0f, 0.0f));

    spline->AddControlPoint(first);
    spline->AddControlPoint(second);

    REQUIRE(spline->GetLength() > 0.0f);
    REQUIRE(spline->GetPoint(0.0f).Equals(Vector3::ZERO));
    REQUIRE(spline->GetPoint(1.0f).Equals(Vector3(10.0f, 0.0f, 0.0f)));

    SECTION("removing a control point shortens the path")
    {
        const float before = spline->GetLength();
        spline->RemoveControlPoint(second);
        REQUIRE(spline->GetLength() < before);
    }

    SECTION("clearing removes every control point but leaves the cached length stale")
    {
        const float before = spline->GetLength();
        spline->ClearControlPoints();
        REQUIRE(spline->GetPoint(0.5f) == Vector3::ZERO);
        REQUIRE_EQ_F(spline->GetLength(), before);
    }
}

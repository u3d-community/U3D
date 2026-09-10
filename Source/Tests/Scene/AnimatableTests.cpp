#include "TestUtils.h"

#include <Urho3D/Resource/JSONFile.h>
#include <Urho3D/Resource/XMLFile.h>
#include <Urho3D/Scene/Component.h>
#include <Urho3D/Scene/Node.h>
#include <Urho3D/Scene/ObjectAnimation.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/ValueAnimation.h>

using namespace Urho3D;
using namespace U3DTest;

namespace
{

class Dial : public Component
{
    URHO3D_OBJECT(Dial, Component);

public:
    explicit Dial(Context* context) :
        Component(context),
        value_(0.0f)
    {
    }

    static void RegisterObject(Context* context)
    {
        context->RegisterFactory<Dial>();
        URHO3D_ATTRIBUTE("Value", float, value_, 0.0f, AM_DEFAULT);
        URHO3D_ATTRIBUTE("Position", Vector3, position_, Vector3::ZERO, AM_DEFAULT);
    }

    float value_;
    Vector3 position_;
};

Context* DialContext()
{
    Context* context = HeadlessContext();
    static bool registered = false;
    if (!registered)
    {
        Dial::RegisterObject(context);
        registered = true;
    }
    return context;
}

SharedPtr<ValueAnimation> Ramp(Context* context, float from = 0.0f, float to = 10.0f, float length = 1.0f)
{
    SharedPtr<ValueAnimation> animation(new ValueAnimation(context));
    animation->SetKeyFrame(0.0f, Variant(from));
    animation->SetKeyFrame(length, Variant(to));
    return animation;
}

struct DialFixture
{
    DialFixture() :
        scene_(new Scene(DialContext()))
    {
        dial_ = scene_->CreateChild("Dial")->CreateComponent<Dial>();
    }

    SharedPtr<Scene> scene_;
    Dial* dial_{};
};

}

TEST_CASE("Animatable drives an attribute from a value animation", "[Scene]")
{
    DialFixture fixture;
    Dial* dial = fixture.dial_;

    REQUIRE(dial->GetAnimationEnabled());
    REQUIRE(dial->GetAttributeAnimation("Value") == nullptr);

    SharedPtr<ValueAnimation> ramp = Ramp(DialContext());
    dial->SetAttributeAnimation("Value", ramp, WM_CLAMP, 1.0f);

    REQUIRE(dial->GetAttributeAnimation("Value") == ramp);
    REQUIRE(dial->GetAttributeAnimationWrapMode("Value") == WM_CLAMP);
    REQUIRE_EQ_F(dial->GetAttributeAnimationSpeed("Value"), 1.0f);
    REQUIRE_EQ_F(dial->GetAttributeAnimationTime("Value"), 0.0f);

    fixture.scene_->Update(0.5f);
    REQUIRE_NEAR(dial->value_, 5.0f, 0.01f);
    REQUIRE_NEAR(dial->GetAttributeAnimationTime("Value"), 0.5f, 0.01f);

    fixture.scene_->Update(0.5f);
    REQUIRE_NEAR(dial->value_, 10.0f, 0.01f);

    SECTION("a clamped animation holds at the last value")
    {
        fixture.scene_->Update(5.0f);
        REQUIRE_NEAR(dial->value_, 10.0f, 0.01f);
    }

    SECTION("a looped animation wraps back to the start")
    {
        dial->SetAttributeAnimation("Value", ramp, WM_LOOP, 1.0f);
        fixture.scene_->Update(1.25f);
        REQUIRE_NEAR(dial->value_, 2.5f, 0.1f);
    }

    SECTION("a one shot animation removes itself when it finishes")
    {
        dial->SetAttributeAnimation("Value", ramp, WM_ONCE, 1.0f);
        fixture.scene_->Update(2.0f);
        REQUIRE(dial->GetAttributeAnimation("Value") == nullptr);
        REQUIRE_NEAR(dial->value_, 10.0f, 0.01f);
    }

    SECTION("the speed scales how fast the animation runs")
    {
        dial->SetAttributeAnimation("Value", ramp, WM_CLAMP, 1.0f);
        dial->SetAttributeAnimationSpeed("Value", 4.0f);
        REQUIRE_EQ_F(dial->GetAttributeAnimationSpeed("Value"), 4.0f);

        fixture.scene_->Update(0.25f);
        REQUIRE_NEAR(dial->value_, 10.0f, 0.01f);
    }

    SECTION("the time can be set directly")
    {
        dial->SetAttributeAnimation("Value", ramp, WM_CLAMP, 1.0f);
        dial->SetAttributeAnimationTime("Value", 0.75f);
        REQUIRE_NEAR(dial->GetAttributeAnimationTime("Value"), 0.75f, 0.01f);

        fixture.scene_->Update(0.0f);
        REQUIRE_NEAR(dial->value_, 7.5f, 0.1f);
    }

    SECTION("removing the animation leaves the attribute where it stopped")
    {
        const float held = dial->value_;
        dial->RemoveAttributeAnimation("Value");
        REQUIRE(dial->GetAttributeAnimation("Value") == nullptr);

        fixture.scene_->Update(1.0f);
        REQUIRE_EQ_F(dial->value_, held);
    }

    SECTION("passing a null animation removes it too")
    {
        dial->SetAttributeAnimation("Value", nullptr);
        REQUIRE(dial->GetAttributeAnimation("Value") == nullptr);
    }

    SECTION("disabling animation freezes the attribute and the clock behind it")
    {
        dial->SetAttributeAnimationTime("Value", 0.0f);
        dial->SetAnimationEnabled(false);
        REQUIRE_FALSE(dial->GetAnimationEnabled());

        const float frozen = dial->value_;
        fixture.scene_->Update(1.0f);
        REQUIRE_EQ_F(dial->value_, frozen);
        REQUIRE_EQ_F(dial->GetAttributeAnimationTime("Value"), 0.0f);

        dial->SetAnimationEnabled(true);
        fixture.scene_->Update(0.5f);
        REQUIRE_NEAR(dial->value_, 5.0f, 0.1f);
    }

    SECTION("an unknown attribute name is ignored")
    {
        dial->SetAttributeAnimation("NoSuchAttribute", ramp);
        REQUIRE(dial->GetAttributeAnimation("NoSuchAttribute") == nullptr);
        REQUIRE(dial->GetAttributeAnimationWrapMode("NoSuchAttribute") == WM_LOOP);
        REQUIRE_EQ_F(dial->GetAttributeAnimationSpeed("NoSuchAttribute"), 1.0f);
        REQUIRE_EQ_F(dial->GetAttributeAnimationTime("NoSuchAttribute"), 0.0f);
    }
}

TEST_CASE("Animatable animates a vector attribute", "[Scene]")
{
    DialFixture fixture;

    SharedPtr<ValueAnimation> animation(new ValueAnimation(DialContext()));
    animation->SetKeyFrame(0.0f, Variant(Vector3::ZERO));
    animation->SetKeyFrame(1.0f, Variant(Vector3(10.0f, 20.0f, 30.0f)));

    fixture.dial_->SetAttributeAnimation("Position", animation, WM_CLAMP, 1.0f);
    fixture.scene_->Update(0.5f);

    REQUIRE(fixture.dial_->position_.Equals(Vector3(5.0f, 10.0f, 15.0f)));
}

TEST_CASE("An object animation drives several attributes at once", "[Scene]")
{
    DialFixture fixture;
    Context* context = DialContext();

    SharedPtr<ObjectAnimation> objectAnimation(new ObjectAnimation(context));
    objectAnimation->AddAttributeAnimation("Value", Ramp(context), WM_CLAMP, 1.0f);

    SharedPtr<ValueAnimation> positionAnimation(new ValueAnimation(context));
    positionAnimation->SetKeyFrame(0.0f, Variant(Vector3::ZERO));
    positionAnimation->SetKeyFrame(1.0f, Variant(Vector3::ONE));
    objectAnimation->AddAttributeAnimation("Position", positionAnimation, WM_CLAMP, 1.0f);

    fixture.dial_->SetObjectAnimation(objectAnimation);
    REQUIRE(fixture.dial_->GetObjectAnimation() == objectAnimation);
    REQUIRE(fixture.dial_->GetAttributeAnimation("Value") != nullptr);
    REQUIRE(fixture.dial_->GetAttributeAnimation("Position") != nullptr);

    fixture.scene_->Update(0.5f);
    REQUIRE_NEAR(fixture.dial_->value_, 5.0f, 0.01f);
    REQUIRE(fixture.dial_->position_.Equals(Vector3(0.5f, 0.5f, 0.5f)));

    SECTION("removing the object animation takes its attributes with it")
    {
        fixture.dial_->RemoveObjectAnimation();
        REQUIRE(fixture.dial_->GetObjectAnimation() == nullptr);
        REQUIRE(fixture.dial_->GetAttributeAnimation("Value") == nullptr);
        REQUIRE(fixture.dial_->GetAttributeAnimation("Position") == nullptr);
    }

    SECTION("replacing the object animation swaps the whole set")
    {
        SharedPtr<ObjectAnimation> replacement(new ObjectAnimation(context));
        replacement->AddAttributeAnimation("Value", Ramp(context, 100.0f, 200.0f), WM_CLAMP, 1.0f);

        fixture.dial_->SetObjectAnimation(replacement);
        REQUIRE(fixture.dial_->GetObjectAnimation() == replacement);
        REQUIRE(fixture.dial_->GetAttributeAnimation("Position") == nullptr);

        fixture.scene_->Update(0.5f);
        REQUIRE_NEAR(fixture.dial_->value_, 150.0f, 0.5f);
    }

    SECTION("editing the object animation adds and removes live attributes")
    {
        objectAnimation->RemoveAttributeAnimation("Position");
        REQUIRE(fixture.dial_->GetAttributeAnimation("Position") == nullptr);
        REQUIRE(fixture.dial_->GetAttributeAnimation("Value") != nullptr);
    }

    SECTION("setting a null object animation clears it")
    {
        fixture.dial_->SetObjectAnimation(nullptr);
        REQUIRE(fixture.dial_->GetObjectAnimation() == nullptr);
    }

    SECTION("a global time can be applied across the whole set")
    {
        fixture.dial_->SetAnimationTime(1.0f);
        REQUIRE_NEAR(fixture.dial_->GetAttributeAnimationTime("Value"), 1.0f, 0.01f);
    }
}

TEST_CASE("Attribute animations survive a document round trip", "[Scene]")
{
    DialFixture fixture;
    Context* context = DialContext();

    fixture.dial_->SetAttributeAnimation("Value", Ramp(context), WM_LOOP, 2.0f);

    SECTION("XML")
    {
        SharedPtr<XMLFile> document(new XMLFile(context));
        XMLElement root = document->CreateRoot("component");
        REQUIRE(fixture.dial_->SaveXML(root));

        auto* restored = fixture.scene_->CreateChild("Restored")->CreateComponent<Dial>();
        REQUIRE(restored->LoadXML(root));

        REQUIRE(restored->GetAttributeAnimation("Value") != nullptr);
        REQUIRE(restored->GetAttributeAnimationWrapMode("Value") == WM_LOOP);
        REQUIRE_EQ_F(restored->GetAttributeAnimationSpeed("Value"), 2.0f);
    }

    SECTION("JSON")
    {
        JSONValue json;
        REQUIRE(fixture.dial_->SaveJSON(json));

        auto* restored = fixture.scene_->CreateChild("Restored")->CreateComponent<Dial>();
        REQUIRE(restored->LoadJSON(json));

        REQUIRE(restored->GetAttributeAnimation("Value") != nullptr);
        REQUIRE(restored->GetAttributeAnimationWrapMode("Value") == WM_LOOP);
    }
}

TEST_CASE("A node animates its own transform attributes", "[Scene]")
{
    SharedPtr<Scene> scene(new Scene(DialContext()));
    Node* node = scene->CreateChild("Mover");

    SharedPtr<ValueAnimation> animation(new ValueAnimation(DialContext()));
    animation->SetKeyFrame(0.0f, Variant(Vector3::ZERO));
    animation->SetKeyFrame(1.0f, Variant(Vector3(10.0f, 0.0f, 0.0f)));

    node->SetAttributeAnimation("Position", animation, WM_CLAMP, 1.0f);
    REQUIRE(node->GetAttributeAnimation("Position") == animation);

    scene->Update(0.5f);
    REQUIRE(node->GetPosition().Equals(Vector3(5.0f, 0.0f, 0.0f)));

    SECTION("a child component can be reached through its own animation")
    {
        auto* dial = node->CreateComponent<Dial>();
        dial->SetAttributeAnimation("Value", Ramp(DialContext()), WM_CLAMP, 1.0f);

        scene->Update(0.5f);
        REQUIRE_NEAR(dial->value_, 5.0f, 0.1f);
        REQUIRE(node->GetPosition().Equals(Vector3(10.0f, 0.0f, 0.0f)));
    }
}

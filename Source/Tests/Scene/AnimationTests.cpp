#include "TestUtils.h"
#include <Urho3D/IO/VectorBuffer.h>
#include <Urho3D/Resource/JSONValue.h>
#include <Urho3D/Resource/XMLFile.h>
#include <Urho3D/Scene/ObjectAnimation.h>
#include <Urho3D/Scene/ValueAnimation.h>

using namespace Urho3D;
using namespace U3DTest;

TEST_CASE("ValueAnimation interpolates and orders keyframes", "[Scene]")
{
    TestContext context;
    ValueAnimation animation(context);
    REQUIRE_FALSE(animation.IsValid());
    REQUIRE(animation.SetKeyFrame(2.0f, 20.0f));
    REQUIRE(animation.SetKeyFrame(0.0f, 0.0f));
    REQUIRE(animation.SetKeyFrame(1.0f, 10.0f));
    REQUIRE(animation.IsValid());
    REQUIRE(animation.GetValueType() == VAR_FLOAT);
    REQUIRE_EQ_F(animation.GetBeginTime(), 0.0f);
    REQUIRE_EQ_F(animation.GetEndTime(), 2.0f);
    REQUIRE_EQ_F(animation.GetAnimationValue(-1.0f).GetFloat(), -10.0f);
    REQUIRE_EQ_F(animation.GetAnimationValue(0.5f).GetFloat(), 5.0f);
    REQUIRE_EQ_F(animation.GetAnimationValue(3.0f).GetFloat(), 20.0f);
    animation.SetInterpolationMethod(IM_NONE);
    REQUIRE_EQ_F(animation.GetAnimationValue(0.5f).GetFloat(), 0.0f);
    animation.SetInterpolationMethod(IM_SPLINE);
    animation.SetSplineTension(0.25f);
    REQUIRE(animation.GetInterpolationMethod() == IM_SPLINE);
    REQUIRE_EQ_F(animation.GetSplineTension(), 0.25f);
    REQUIRE(animation.GetAnimationValue(1.0f).GetFloat() == 10.0f);
    REQUIRE_FALSE(animation.SetKeyFrame(3.0f, String("wrong type")));
    REQUIRE_FALSE(animation.SetKeyFrame(1.0f, 99.0f));
}

TEST_CASE("ValueAnimation interpolates additional numeric and geometry types", "[Scene]")
{
    TestContext context;
    ValueAnimation rectangles(context);
    rectangles.SetKeyFrame(0.0f, IntRect(0, 10, 20, 30));
    rectangles.SetKeyFrame(1.0f, IntRect(20, 30, 40, 50));
    REQUIRE(rectangles.GetAnimationValue(0.25f).GetIntRect() == IntRect(5, 15, 25, 35));

    ValueAnimation spline(context);
    spline.SetInterpolationMethod(IM_SPLINE);
    spline.SetKeyFrame(0.0f, Vector2(0.0f, 1.0f));
    spline.SetKeyFrame(1.0f, Vector2(2.0f, 4.0f));
    spline.SetKeyFrame(2.0f, Vector2(5.0f, 8.0f));
    const Vector2 midpoint = spline.GetAnimationValue(0.5f).GetVector2();
    REQUIRE(midpoint.x_ > 0.0f);
    REQUIRE(midpoint.y_ > 1.0f);
    REQUIRE(midpoint.x_ < 2.0f);
    REQUIRE(midpoint.y_ < 4.0f);
}

TEST_CASE("ValueAnimation event ranges and document formats", "[Scene]")
{
    TestContext context;
    ValueAnimation source(context);
    source.SetKeyFrame(0.0f, Vector3::ZERO);
    source.SetKeyFrame(1.0f, Vector3::ONE);
    VariantMap data;
    data["value"] = 42;
    source.SetEventFrame(0.25f, StringHash("First"), data);
    source.SetEventFrame(0.75f, StringHash("Second"));
    REQUIRE(source.HasEventFrames());
    PODVector<const VAnimEventFrame*> events;
    source.GetEventFrames(0.2f, 0.5f, events);
    REQUIRE(events.Size() == 1);
    REQUIRE(events[0]->eventType_ == StringHash("First"));
    REQUIRE(events[0]->eventData_["value"]->GetInt() == 42);

    SharedPtr<XMLFile> xml(new XMLFile(context));
    XMLElement root = xml->CreateRoot("valueanimation");
    REQUIRE(source.SaveXML(root));
    ValueAnimation fromXML(context);
    REQUIRE(fromXML.LoadXML(root));
    REQUIRE(fromXML.GetKeyFrames().Size() == 2);
    REQUIRE(fromXML.GetAnimationValue(1.0f).GetVector3() == Vector3::ONE);

    JSONValue json;
    REQUIRE(source.SaveJSON(json));
    ValueAnimation fromJSON(context);
    REQUIRE(fromJSON.LoadJSON(json));
    REQUIRE(fromJSON.GetKeyFrames().Size() == 2);
    REQUIRE(fromJSON.HasEventFrames());
}

TEST_CASE("ObjectAnimation owns named attribute animations", "[Scene]")
{
    TestContext context;
    SharedPtr<ValueAnimation> value(new ValueAnimation(context));
    value->SetKeyFrame(0.0f, 0.0f);
    value->SetKeyFrame(1.0f, 1.0f);
    ObjectAnimation object(context);
    object.AddAttributeAnimation("Position", value, WM_CLAMP, 2.0f);
    REQUIRE(object.GetAttributeAnimation("Position") == value);
    REQUIRE(object.GetAttributeAnimationWrapMode("Position") == WM_CLAMP);
    REQUIRE_EQ_F(object.GetAttributeAnimationSpeed("Position"), 2.0f);
    REQUIRE(object.GetAttributeAnimationInfo("Position") != nullptr);
    REQUIRE(object.GetAttributeAnimationInfos().Size() == 1);

    JSONValue json;
    REQUIRE(object.SaveJSON(json));
    ObjectAnimation restored(context);
    REQUIRE(restored.LoadJSON(json));
    REQUIRE(restored.GetAttributeAnimation("Position") != nullptr);
    restored.RemoveAttributeAnimation("Position");
    REQUIRE(restored.GetAttributeAnimationInfos().Empty());
    object.RemoveAttributeAnimation(value);
    REQUIRE(object.GetAttributeAnimationInfos().Empty());
}

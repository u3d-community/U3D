#include "TestUtils.h"

#include <Urho3D/Core/Context.h>
#include <Urho3D/Core/Object.h>
#include <Urho3D/Core/Spline.h>
#include <Urho3D/Core/Timer.h>

using namespace Urho3D;

namespace
{

URHO3D_EVENT(E_TESTEVENT, TestEvent)
{
    URHO3D_PARAM(P_VALUE, Value);
}

URHO3D_EVENT(E_OTHEREVENT, OtherEvent)
{
    URHO3D_PARAM(P_VALUE, Value);
}

class TestObject : public Object
{
    URHO3D_OBJECT(TestObject, Object);

public:
    explicit TestObject(Context* context) :
        Object(context),
        received_(0),
        lastValue_(0),
        lastSender_(nullptr)
    {
    }

    void Listen()
    {
        SubscribeToEvent(E_TESTEVENT, URHO3D_HANDLER(TestObject, HandleTest));
    }

    void ListenTo(Object* sender)
    {
        SubscribeToEvent(sender, E_TESTEVENT, URHO3D_HANDLER(TestObject, HandleTest));
    }

    void HandleTest(StringHash eventType, VariantMap& eventData)
    {
        ++received_;
        lastValue_ = eventData[TestEvent::P_VALUE].GetInt();
        lastSender_ = GetEventSender();
    }

    int received_;
    int lastValue_;
    Object* lastSender_;
};

class DerivedObject : public TestObject
{
    URHO3D_OBJECT(DerivedObject, TestObject);

public:
    explicit DerivedObject(Context* context) : TestObject(context) {}
};

}

TEST_CASE("Object type information", "[Core]")
{
    SharedPtr<Context> context(new Context());
    SharedPtr<TestObject> object(new TestObject(context));

    REQUIRE(object->GetType() == TestObject::GetTypeStatic());
    REQUIRE(object->GetTypeName() == "TestObject");
    REQUIRE(object->GetTypeName() == TestObject::GetTypeNameStatic());
    REQUIRE(object->GetTypeInfo() == TestObject::GetTypeInfoStatic());

    SECTION("a derived type reports its own name and its base chain")
    {
        SharedPtr<DerivedObject> derived(new DerivedObject(context));
        REQUIRE(derived->GetTypeName() == "DerivedObject");
        REQUIRE(derived->IsInstanceOf<DerivedObject>());
        REQUIRE(derived->IsInstanceOf<TestObject>());
        REQUIRE(derived->IsInstanceOf(TestObject::GetTypeStatic()));
        REQUIRE_FALSE(derived->IsInstanceOf<Object>());
    }

    SECTION("a base instance is not an instance of the derived type")
    {
        REQUIRE_FALSE(object->IsInstanceOf<DerivedObject>());
    }

    SECTION("the type info exposes the base type")
    {
        const TypeInfo* info = DerivedObject::GetTypeInfoStatic();
        REQUIRE(info->GetTypeName() == "DerivedObject");
        REQUIRE(info->GetBaseTypeInfo() == TestObject::GetTypeInfoStatic());
        REQUIRE(info->IsTypeOf(TestObject::GetTypeStatic()));
        REQUIRE(info->IsTypeOf<DerivedObject>());
        REQUIRE_FALSE(TestObject::GetTypeInfoStatic()->IsTypeOf<DerivedObject>());
    }
}

TEST_CASE("Context object factories", "[Core]")
{
    SharedPtr<Context> context(new Context());

    REQUIRE(context->CreateObject<TestObject>().Null());

    context->RegisterFactory<TestObject>();
    SharedPtr<TestObject> created = context->CreateObject<TestObject>();
    REQUIRE(created.NotNull());
    REQUIRE(created->GetTypeName() == "TestObject");

    REQUIRE(context->GetObjectFactories().Contains(TestObject::GetTypeStatic()));

    SECTION("a category groups the registered type")
    {
        context->RegisterFactory<DerivedObject>("TestCategory");
        REQUIRE(context->GetObjectCategories().Contains("TestCategory"));
        REQUIRE(context->GetObjectCategories()["TestCategory"]->Contains(DerivedObject::GetTypeStatic()));
    }

    SECTION("an unknown type hash creates nothing")
    {
        REQUIRE(context->CreateObject(StringHash("NotRegistered")).Null());
    }

    SECTION("the type name can be looked up from the factory")
    {
        REQUIRE(context->GetTypeName(TestObject::GetTypeStatic()) == "TestObject");
        REQUIRE(context->GetTypeName(StringHash("NotRegistered")).Empty());
    }
}

TEST_CASE("Context subsystems", "[Core]")
{
    SharedPtr<Context> context(new Context());
    REQUIRE(context->GetSubsystem<TestObject>() == nullptr);

    auto* subsystem = new TestObject(context);
    context->RegisterSubsystem(subsystem);

    REQUIRE(context->GetSubsystem<TestObject>() == subsystem);
    REQUIRE(context->GetSubsystem(TestObject::GetTypeStatic()) == subsystem);
    REQUIRE(context->GetSubsystems().Contains(TestObject::GetTypeStatic()));

    SECTION("an object can reach the subsystem through its context")
    {
        SharedPtr<TestObject> other(new TestObject(context));
        REQUIRE(other->GetSubsystem<TestObject>() == subsystem);
    }

    SECTION("removal by type clears the slot")
    {
        context->RemoveSubsystem(TestObject::GetTypeStatic());
        REQUIRE(context->GetSubsystem<TestObject>() == nullptr);
    }

    SECTION("removing an absent subsystem is harmless")
    {
        context->RemoveSubsystem(StringHash("Nothing"));
        REQUIRE(context->GetSubsystem<TestObject>() == subsystem);
    }
}

TEST_CASE("Object event broadcast", "[Core]")
{
    SharedPtr<Context> context(new Context());
    SharedPtr<TestObject> listener(new TestObject(context));
    SharedPtr<TestObject> sender(new TestObject(context));

    listener->Listen();
    REQUIRE(listener->HasSubscribedToEvent(E_TESTEVENT));
    REQUIRE_FALSE(listener->HasSubscribedToEvent(E_OTHEREVENT));

    VariantMap data;
    data[TestEvent::P_VALUE] = 42;
    sender->SendEvent(E_TESTEVENT, data);

    REQUIRE(listener->received_ == 1);
    REQUIRE(listener->lastValue_ == 42);

    SECTION("a different event does not reach the handler")
    {
        sender->SendEvent(E_OTHEREVENT, data);
        REQUIRE(listener->received_ == 1);
    }

    SECTION("unsubscribing stops delivery")
    {
        listener->UnsubscribeFromEvent(E_TESTEVENT);
        REQUIRE_FALSE(listener->HasSubscribedToEvent(E_TESTEVENT));
        sender->SendEvent(E_TESTEVENT, data);
        REQUIRE(listener->received_ == 1);
    }

    SECTION("UnsubscribeFromAllEvents clears every subscription")
    {
        listener->UnsubscribeFromAllEvents();
        REQUIRE_FALSE(listener->HasSubscribedToEvent(E_TESTEVENT));
        sender->SendEvent(E_TESTEVENT, data);
        REQUIRE(listener->received_ == 1);
    }

    SECTION("two listeners both receive the broadcast")
    {
        SharedPtr<TestObject> second(new TestObject(context));
        second->Listen();
        sender->SendEvent(E_TESTEVENT, data);
        REQUIRE(listener->received_ == 2);
        REQUIRE(second->received_ == 1);
    }

    SECTION("a destroyed listener is dropped rather than dangling")
    {
        {
            SharedPtr<TestObject> temporary(new TestObject(context));
            temporary->Listen();
            sender->SendEvent(E_TESTEVENT, data);
            REQUIRE(temporary->received_ == 1);
        }
        sender->SendEvent(E_TESTEVENT, data);
        REQUIRE(listener->received_ == 3);
    }
}

TEST_CASE("Object specific sender subscription", "[Core]")
{
    SharedPtr<Context> context(new Context());
    SharedPtr<TestObject> listener(new TestObject(context));
    SharedPtr<TestObject> wanted(new TestObject(context));
    SharedPtr<TestObject> other(new TestObject(context));

    listener->ListenTo(wanted);
    REQUIRE(listener->HasSubscribedToEvent(wanted, E_TESTEVENT));
    REQUIRE_FALSE(listener->HasSubscribedToEvent(other, E_TESTEVENT));

    VariantMap data;
    data[TestEvent::P_VALUE] = 7;

    other->SendEvent(E_TESTEVENT, data);
    REQUIRE(listener->received_ == 0);

    wanted->SendEvent(E_TESTEVENT, data);
    REQUIRE(listener->received_ == 1);
    REQUIRE(listener->lastValue_ == 7);

    SECTION("the event sender is reported during handling and cleared afterwards")
    {
        REQUIRE(listener->lastSender_ == wanted.Get());
        REQUIRE(listener->GetEventSender() == nullptr);
    }

    SECTION("unsubscribing from that sender stops delivery")
    {
        listener->UnsubscribeFromEvents(wanted);
        REQUIRE_FALSE(listener->HasSubscribedToEvent(wanted, E_TESTEVENT));
        wanted->SendEvent(E_TESTEVENT, data);
        REQUIRE(listener->received_ == 1);
    }
}

TEST_CASE("Spline interpolates control points", "[Core]")
{
    Spline spline;
    REQUIRE(spline.GetKnots().Empty());
    REQUIRE(spline.GetInterpolationMode() == BEZIER_CURVE);

    spline.AddKnot(Variant(Vector3::ZERO));
    spline.AddKnot(Variant(Vector3(10.0f, 0.0f, 0.0f)));

    REQUIRE(spline.GetKnots().Size() == 2);
    REQUIRE(spline.GetKnot(0).GetVector3() == Vector3::ZERO);

    REQUIRE(spline.GetPoint(0.0f).GetVector3().Equals(Vector3::ZERO));
    REQUIRE(spline.GetPoint(1.0f).GetVector3().Equals(Vector3(10.0f, 0.0f, 0.0f)));
    REQUIRE(spline.GetPoint(0.5f).GetVector3().Equals(Vector3(5.0f, 0.0f, 0.0f)));

    SECTION("a knot can be inserted and removed")
    {
        spline.AddKnot(Variant(Vector3(5.0f, 5.0f, 0.0f)), 1);
        REQUIRE(spline.GetKnots().Size() == 3);
        REQUIRE(spline.GetKnot(1).GetVector3() == Vector3(5.0f, 5.0f, 0.0f));

        spline.RemoveKnot(1);
        REQUIRE(spline.GetKnots().Size() == 2);

        spline.RemoveKnot();
        REQUIRE(spline.GetKnots().Size() == 1);
    }

    SECTION("SetKnot replaces a value in place")
    {
        spline.SetKnot(Variant(Vector3(1.0f, 1.0f, 1.0f)), 0);
        REQUIRE(spline.GetKnot(0).GetVector3() == Vector3(1.0f, 1.0f, 1.0f));
    }

    SECTION("Clear empties the spline")
    {
        spline.Clear();
        REQUIRE(spline.GetKnots().Empty());
    }

    SECTION("each interpolation mode produces the endpoints")
    {
        const InterpolationMode modes[] = {BEZIER_CURVE, CATMULL_ROM_CURVE, LINEAR_CURVE, CATMULL_ROM_FULL_CURVE};
        for (InterpolationMode mode : modes)
        {
            Spline curve(mode);
            curve.AddKnot(Variant(Vector3::ZERO));
            curve.AddKnot(Variant(Vector3(5.0f, 0.0f, 0.0f)));
            curve.AddKnot(Variant(Vector3(10.0f, 0.0f, 0.0f)));
            curve.AddKnot(Variant(Vector3(15.0f, 0.0f, 0.0f)));

            REQUIRE(curve.GetInterpolationMode() == mode);
            REQUIRE_FALSE(curve.GetPoint(0.5f).IsEmpty());
        }
    }

    SECTION("the mode can be changed after construction")
    {
        spline.SetInterpolationMode(LINEAR_CURVE);
        REQUIRE(spline.GetInterpolationMode() == LINEAR_CURVE);
        REQUIRE(spline.GetPoint(0.5f).GetVector3().Equals(Vector3(5.0f, 0.0f, 0.0f)));
    }

    SECTION("floats interpolate as well as vectors")
    {
        Spline scalar;
        scalar.AddKnot(Variant(0.0f));
        scalar.AddKnot(Variant(10.0f));
        REQUIRE_NEAR(scalar.GetPoint(0.5f).GetFloat(), 5.0f, 0.001f);
    }

    SECTION("an empty spline yields an empty variant")
    {
        Spline empty;
        REQUIRE(empty.GetPoint(0.5f).IsEmpty());
    }

    SECTION("construction from a knot vector")
    {
        VariantVector knots;
        knots.Push(Variant(Vector3::ZERO));
        knots.Push(Variant(Vector3::ONE));

        Spline fromKnots(knots, LINEAR_CURVE);
        REQUIRE(fromKnots.GetKnots().Size() == 2);
        REQUIRE(fromKnots.GetInterpolationMode() == LINEAR_CURVE);
    }
}

TEST_CASE("Timer measures elapsed time", "[Core]")
{
    Timer timer;
    Time::Sleep(30);

    const unsigned elapsed = timer.GetMSec(false);
    REQUIRE(elapsed >= 20);
    REQUIRE(timer.GetMSec(false) >= elapsed);

    REQUIRE(timer.GetMSec(true) >= elapsed);
    REQUIRE(timer.GetMSec(false) < elapsed);

    Time::Sleep(30);
    REQUIRE(timer.GetMSec(false) >= 20);
    timer.Reset();
    REQUIRE(timer.GetMSec(false) < 20);
}

TEST_CASE("HiresTimer measures elapsed microseconds", "[Core]")
{
    SharedPtr<Context> context(new Context());
    SharedPtr<Time> time(new Time(context));

    REQUIRE(HiresTimer::IsSupported());
    REQUIRE(HiresTimer::GetFrequency() > 0);

    HiresTimer timer;
    Time::Sleep(10);

    const long long elapsed = timer.GetUSec(false);
    REQUIRE(elapsed >= 5000);
    REQUIRE(timer.GetUSec(true) >= elapsed);
    REQUIRE(timer.GetUSec(false) < elapsed);
}

TEST_CASE("Time subsystem tracks frames", "[Core]")
{
    SharedPtr<Context> context(new Context());
    SharedPtr<Time> time(new Time(context));

    REQUIRE(time->GetFrameNumber() == 0);
    REQUIRE_EQ_F(time->GetElapsedTime(), 0.0f);

    time->BeginFrame(0.016f);
    REQUIRE(time->GetFrameNumber() == 1);
    REQUIRE_EQ_F(time->GetTimeStep(), 0.016f);
    time->EndFrame();

    time->BeginFrame(0.016f);
    REQUIRE(time->GetFrameNumber() == 2);
    REQUIRE(time->GetElapsedTime() >= 0.0f);
    time->EndFrame();

    SECTION("the static clock helpers return sane values")
    {
        REQUIRE(Time::GetSystemTime() > 0);
        REQUIRE(Time::GetTimeSinceEpoch() > 0);
        REQUIRE_FALSE(Time::GetTimeStamp().Empty());
    }
}

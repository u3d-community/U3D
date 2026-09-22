#include "TestUtils.h"

#include <Urho3D/Core/Condition.h>
#include <Urho3D/Core/Context.h>
#include <Urho3D/Core/Mutex.h>
#include <Urho3D/Core/Object.h>
#include <Urho3D/Core/StringHashRegister.h>
#include <Urho3D/Core/StringUtils.h>
#include <Urho3D/Core/Thread.h>
#include <Urho3D/Core/Variant.h>

#include <atomic>

using namespace Urho3D;

namespace
{

URHO3D_EVENT(E_EXTRAEVENT, ExtraEvent)
{
    URHO3D_PARAM(P_VALUE, Value);
}

class Subscriber : public Object
{
    URHO3D_OBJECT(Subscriber, Object);

public:
    explicit Subscriber(Context* context) : Object(context), count_(0) {}

    void Handle(StringHash eventType, VariantMap& eventData) { ++count_; }
    void HandleAlternate(StringHash eventType, VariantMap& eventData) { count_ += 10; }

    void Listen() { SubscribeToEvent(E_EXTRAEVENT, URHO3D_HANDLER(Subscriber, Handle)); }
    void ListenAlternate() { SubscribeToEvent(E_EXTRAEVENT, URHO3D_HANDLER(Subscriber, HandleAlternate)); }
    void ListenTo(Object* sender) { SubscribeToEvent(sender, E_EXTRAEVENT, URHO3D_HANDLER(Subscriber, Handle)); }
    void ListenToAlternate(Object* sender)
    {
        SubscribeToEvent(sender, E_EXTRAEVENT, URHO3D_HANDLER(Subscriber, HandleAlternate));
    }

    int count_;
};

class SelfRemovingSubscriber : public Object
{
    URHO3D_OBJECT(SelfRemovingSubscriber, Object);

public:
    explicit SelfRemovingSubscriber(Context* context) : Object(context), count_(0) {}

    void Listen() { SubscribeToEvent(E_EXTRAEVENT, URHO3D_HANDLER(SelfRemovingSubscriber, Handle)); }

    void Handle(StringHash eventType, VariantMap& eventData)
    {
        ++count_;
        UnsubscribeFromEvent(E_EXTRAEVENT);
    }

    int count_;
};

class SignallingThread : public Thread
{
public:
    explicit SignallingThread(Condition& condition) : condition_(condition), ran_(false) {}

    void ThreadFunction() override
    {
        ran_ = true;
        condition_.Set();
    }

    Condition& condition_;
    std::atomic<bool> ran_;
};

class CountingThread : public Thread
{
public:
    CountingThread() : value_(0) {}

    void ThreadFunction() override
    {
        for (int i = 0; i < 1000; ++i)
        {
            MutexLock lock(mutex_);
            ++value_;
        }
    }

    Mutex mutex_;
    int value_;
};

}

TEST_CASE("StringUtils String overloads forward to the raw pointer form", "[Core]")
{
    REQUIRE(ToInt64(String("1234567890123")) == 1234567890123LL);
    REQUIRE(ToInt64(String("ff"), 16) == 255);
    REQUIRE(ToUInt64(String("1234567890123")) == 1234567890123ULL);
    REQUIRE(ToUInt64(String("ff"), 16) == 255ULL);
    REQUIRE(ToDouble(String("2.5")) == 2.5);
    REQUIRE(ToIntVector2(String("1 2")) == IntVector2(1, 2));
    REQUIRE(ToIntVector3(String("1 2 3")) == IntVector3(1, 2, 3));
    REQUIRE(ToVectorVariant(String("1 2 3")).GetVector3() == Vector3(1.0f, 2.0f, 3.0f));
}

TEST_CASE("StringUtils parsers tolerate a null pointer", "[Core]")
{
    const char* nothing = nullptr;

    REQUIRE(ToInt(nothing) == 0);
    REQUIRE(ToUInt(nothing) == 0u);
    REQUIRE(ToInt64(nothing) == 0);
    REQUIRE(ToUInt64(nothing) == 0ULL);
    REQUIRE_EQ_F(ToFloat(nothing), 0.0f);
    REQUIRE(ToDouble(nothing) == 0.0);
    REQUIRE(ToIntVector2(nothing) == IntVector2::ZERO);
    REQUIRE(ToIntVector3(nothing) == IntVector3::ZERO);
    REQUIRE(ToVector2(nothing) == Vector2::ZERO);
    REQUIRE(ToVector3(nothing) == Vector3::ZERO);
    REQUIRE(ToVector4(nothing) == Vector4::ZERO);
    REQUIRE(ToColor(nothing) == Color::WHITE);
    REQUIRE(ToRect(nothing) == Rect::ZERO);
    REQUIRE(ToIntRect(nothing) == IntRect::ZERO);
    REQUIRE(ToQuaternion(nothing).Equals(Quaternion::IDENTITY));
    REQUIRE(ToVectorVariant(nothing).GetType() == VAR_NONE);
}

TEST_CASE("ToVector4 fills only the components present", "[Core]")
{
    REQUIRE(ToVector4("1 2 3 4", true) == Vector4(1.0f, 2.0f, 3.0f, 4.0f));
    REQUIRE(ToVector4("1 2", true) == Vector4(1.0f, 2.0f, 0.0f, 0.0f));
    REQUIRE(ToVector4("1", true) == Vector4(1.0f, 0.0f, 0.0f, 0.0f));
    REQUIRE(ToVector4("", true) == Vector4::ZERO);

    REQUIRE(ToVector4("1 2", false) == Vector4::ZERO);
}

TEST_CASE("ToVectorVariant picks a matrix type from the element count", "[Core]")
{
    REQUIRE(ToVectorVariant("1 0 0 0 1 0 0 0 1").GetType() == VAR_MATRIX3);
    REQUIRE(ToVectorVariant("1 0 0 0 1 0 0 0 1").GetMatrix3().Equals(Matrix3::IDENTITY));

    REQUIRE(ToVectorVariant("1 0 0 0 0 1 0 0 0 0 1 0").GetType() == VAR_MATRIX3X4);
    REQUIRE(ToVectorVariant("1 0 0 0 0 1 0 0 0 0 1 0").GetMatrix3x4().Equals(Matrix3x4::IDENTITY));

    REQUIRE(ToVectorVariant("1 0 0 0 0 1 0 0 0 0 1 0 0 0 0 1").GetType() == VAR_MATRIX4);
    REQUIRE(ToVectorVariant("1 0 0 0 0 1 0 0 0 0 1 0 0 0 0 1").GetMatrix4().Equals(Matrix4::IDENTITY));

    SECTION("an unrecognised element count yields nothing")
    {
        REQUIRE(ToVectorVariant("1 2 3 4 5").GetType() == VAR_NONE);
    }
}

TEST_CASE("Object subscribes with a std function", "[Core]")
{
    SharedPtr<Context> context(new Context());
    SharedPtr<Subscriber> listener(new Subscriber(context));
    SharedPtr<Subscriber> sender(new Subscriber(context));

    int lambdaCount = 0;
    listener->SubscribeToEvent(E_EXTRAEVENT,
        [&lambdaCount](StringHash eventType, VariantMap& eventData) { ++lambdaCount; });

    REQUIRE(listener->HasSubscribedToEvent(E_EXTRAEVENT));

    VariantMap data;
    sender->SendEvent(E_EXTRAEVENT, data);
    REQUIRE(lambdaCount == 1);

    SECTION("the sender specific form only fires for that sender")
    {
        SharedPtr<Subscriber> wanted(new Subscriber(context));
        SharedPtr<Subscriber> other(new Subscriber(context));

        int targeted = 0;
        listener->SubscribeToEvent(wanted, E_EXTRAEVENT,
            [&targeted](StringHash eventType, VariantMap& eventData) { ++targeted; });

        other->SendEvent(E_EXTRAEVENT, data);
        REQUIRE(targeted == 0);

        wanted->SendEvent(E_EXTRAEVENT, data);
        REQUIRE(targeted == 1);
    }
}

TEST_CASE("Object resubscription replaces the previous handler", "[Core]")
{
    SharedPtr<Context> context(new Context());
    SharedPtr<Subscriber> listener(new Subscriber(context));
    SharedPtr<Subscriber> sender(new Subscriber(context));

    listener->Listen();
    listener->ListenAlternate();

    VariantMap data;
    sender->SendEvent(E_EXTRAEVENT, data);

    REQUIRE(listener->count_ == 10);

    SECTION("the same applies to sender specific handlers")
    {
        SharedPtr<Subscriber> specific(new Subscriber(context));
        listener->count_ = 0;

        listener->ListenTo(specific);
        listener->ListenToAlternate(specific);

        specific->SendEvent(E_EXTRAEVENT, data);
        REQUIRE(listener->count_ == 10);
    }
}

TEST_CASE("Object unsubscribes from a specific sender", "[Core]")
{
    SharedPtr<Context> context(new Context());
    SharedPtr<Subscriber> listener(new Subscriber(context));
    SharedPtr<Subscriber> first(new Subscriber(context));
    SharedPtr<Subscriber> second(new Subscriber(context));

    listener->ListenTo(first);
    listener->ListenTo(second);

    VariantMap data;
    first->SendEvent(E_EXTRAEVENT, data);
    second->SendEvent(E_EXTRAEVENT, data);
    REQUIRE(listener->count_ == 2);

    listener->UnsubscribeFromEvent(first, E_EXTRAEVENT);
    REQUIRE_FALSE(listener->HasSubscribedToEvent(first, E_EXTRAEVENT));
    REQUIRE(listener->HasSubscribedToEvent(second, E_EXTRAEVENT));

    listener->count_ = 0;
    first->SendEvent(E_EXTRAEVENT, data);
    REQUIRE(listener->count_ == 0);
    second->SendEvent(E_EXTRAEVENT, data);
    REQUIRE(listener->count_ == 1);

    SECTION("a null sender is ignored rather than crashing")
    {
        listener->UnsubscribeFromEvent(nullptr, E_EXTRAEVENT);
        REQUIRE(listener->HasSubscribedToEvent(second, E_EXTRAEVENT));
    }

    SECTION("unsubscribing from a sender that was never subscribed is harmless")
    {
        SharedPtr<Subscriber> stranger(new Subscriber(context));
        listener->UnsubscribeFromEvent(stranger, E_EXTRAEVENT);
        REQUIRE(listener->HasSubscribedToEvent(second, E_EXTRAEVENT));
    }

    SECTION("UnsubscribeFromAllEventsExcept keeps the listed events")
    {
        PODVector<StringHash> keep;
        keep.Push(E_EXTRAEVENT);
        listener->UnsubscribeFromAllEventsExcept(keep, true);
        REQUIRE(listener->HasSubscribedToEvent(second, E_EXTRAEVENT));
    }
}

TEST_CASE("Object reports whether anyone is listening", "[Core]")
{
    SharedPtr<Context> context(new Context());
    SharedPtr<Subscriber> listener(new Subscriber(context));
    SharedPtr<Subscriber> sender(new Subscriber(context));

    REQUIRE_FALSE(sender->HasEventHandlers());

    listener->Listen();
    REQUIRE(listener->HasEventHandlers());

    SECTION("sending an event nobody listens to is harmless")
    {
        VariantMap data;
        sender->SendEvent(StringHash("NobodyListens"), data);
        REQUIRE(listener->count_ == 0);
    }

    SECTION("the parameterless SendEvent overload works too")
    {
        sender->SendEvent(E_EXTRAEVENT);
        REQUIRE(listener->count_ == 1);
    }
}

TEST_CASE("StringHashRegister resolves and reports collisions", "[Core]")
{
    StringHashRegister registry(false);

    const StringHash first = registry.RegisterString("First");
    registry.RegisterString("Second");

    REQUIRE(registry.Contains(first));
    REQUIRE_FALSE(registry.Contains(StringHash("Absent")));
    REQUIRE(registry.GetString(first) == "First");
    REQUIRE(registry.GetString(StringHash("Absent")).Empty());
    REQUIRE(registry.GetInternalMap().Size() == 2);

    SECTION("an explicit hash can be registered against a name")
    {
        const StringHash custom(0x12345678u);
        registry.RegisterString(custom, "Custom");
        REQUIRE(registry.GetStringCopy(custom) == "Custom");
    }

    SECTION("a thread safe register behaves the same way")
    {
        StringHashRegister threadSafe(true);
        const StringHash hash = threadSafe.RegisterString("Threaded");
        REQUIRE(threadSafe.GetStringCopy(hash) == "Threaded");
        REQUIRE(threadSafe.Contains(hash));
    }
}

TEST_CASE("Mutex serialises access across threads", "[Core]")
{
    CountingThread worker;
    REQUIRE_FALSE(worker.IsStarted());

    REQUIRE(worker.Run());
    REQUIRE(worker.IsStarted());

    for (int i = 0; i < 1000; ++i)
    {
        MutexLock lock(worker.mutex_);
        ++worker.value_;
    }

    worker.Stop();
    REQUIRE_FALSE(worker.IsStarted());
    REQUIRE(worker.value_ == 2000);

    SECTION("running an already started thread is refused")
    {
        CountingThread second;
        REQUIRE(second.Run());
        REQUIRE_FALSE(second.Run());
        second.Stop();
    }

    SECTION("stopping a thread that never ran is harmless")
    {
        CountingThread idle;
        idle.Stop();
        REQUIRE_FALSE(idle.IsStarted());
    }
}

TEST_CASE("Condition wakes a waiting thread", "[Core]")
{
    Condition condition;
    SignallingThread worker(condition);

    REQUIRE(worker.Run());
    condition.Wait();

    REQUIRE(worker.ran_);
    worker.Stop();
}

TEST_CASE("Thread reports the main thread", "[Core]")
{
    REQUIRE(Thread::IsMainThread());
    REQUIRE(Thread::GetCurrentThreadID() != 0);

    SECTION("a worker sees itself as a non main thread")
    {
        class Checker : public Thread
        {
        public:
            Checker() : wasMain_(true) {}

            void ThreadFunction() override { wasMain_ = Thread::IsMainThread(); }

            std::atomic<bool> wasMain_;
        };

        Checker checker;
        REQUIRE(checker.Run());
        checker.Stop();
        REQUIRE_FALSE(checker.wasMain_);
    }
}

TEST_CASE("Event receivers removed during dispatch are cleaned up afterwards", "[Core]")
{
    SharedPtr<Context> context(new Context());
    SharedPtr<Subscriber> sender(new Subscriber(context));

    SharedPtr<SelfRemovingSubscriber> leaving(new SelfRemovingSubscriber(context));
    SharedPtr<Subscriber> staying(new Subscriber(context));

    leaving->Listen();
    staying->Listen();

    VariantMap data;
    sender->SendEvent(E_EXTRAEVENT, data);

    REQUIRE(leaving->count_ == 1);
    REQUIRE(staying->count_ == 1);
    REQUIRE_FALSE(leaving->HasSubscribedToEvent(E_EXTRAEVENT));

    EventReceiverGroup* group = context->GetEventReceivers(E_EXTRAEVENT);
    REQUIRE(group != nullptr);
    REQUIRE(group->receivers_.Size() == 1);
    REQUIRE(group->receivers_[0] == staying.Get());

    sender->SendEvent(E_EXTRAEVENT, data);
    REQUIRE(leaving->count_ == 1);
    REQUIRE(staying->count_ == 2);

    SECTION("a third dispatch still behaves after the deferred cleanup ran")
    {
        sender->SendEvent(E_EXTRAEVENT, data);
        REQUIRE(staying->count_ == 3);
        REQUIRE(leaving->count_ == 1);
    }

    SECTION("every receiver may leave during the same dispatch")
    {
        SharedPtr<SelfRemovingSubscriber> first(new SelfRemovingSubscriber(context));
        SharedPtr<SelfRemovingSubscriber> second(new SelfRemovingSubscriber(context));
        first->Listen();
        second->Listen();

        sender->SendEvent(E_EXTRAEVENT, data);
        REQUIRE(first->count_ == 1);
        REQUIRE(second->count_ == 1);

        EventReceiverGroup* remaining = context->GetEventReceivers(E_EXTRAEVENT);
        REQUIRE(remaining != nullptr);
        REQUIRE(remaining->receivers_.Size() == 1);
        REQUIRE(remaining->receivers_[0] == staying.Get());

        sender->SendEvent(E_EXTRAEVENT, data);
        REQUIRE(first->count_ == 1);
        REQUIRE(second->count_ == 1);
    }
}

TEST_CASE("UnsubscribeFromAllEventsExcept filters on user data", "[Core]")
{
    SharedPtr<Context> context(new Context());
    SharedPtr<Subscriber> listener(new Subscriber(context));
    SharedPtr<Subscriber> sender(new Subscriber(context));

    int plainCount = 0;
    int userDataCount = 0;
    int marker = 0;

    listener->SubscribeToEvent(E_EXTRAEVENT,
        [&plainCount](StringHash eventType, VariantMap& eventData) { ++plainCount; });
    listener->SubscribeToEvent(StringHash("SecondEvent"),
        [&userDataCount](StringHash eventType, VariantMap& eventData) { ++userDataCount; }, &marker);

    REQUIRE(listener->HasSubscribedToEvent(E_EXTRAEVENT));
    REQUIRE(listener->HasSubscribedToEvent(StringHash("SecondEvent")));

    SECTION("with the flag set, handlers without user data are kept")
    {
        listener->UnsubscribeFromAllEventsExcept(PODVector<StringHash>(), true);

        REQUIRE(listener->HasSubscribedToEvent(E_EXTRAEVENT));
        REQUIRE_FALSE(listener->HasSubscribedToEvent(StringHash("SecondEvent")));

        VariantMap data;
        sender->SendEvent(E_EXTRAEVENT, data);
        REQUIRE(plainCount == 1);
    }

    SECTION("without the flag every unlisted handler goes")
    {
        listener->UnsubscribeFromAllEventsExcept(PODVector<StringHash>(), false);

        REQUIRE_FALSE(listener->HasSubscribedToEvent(E_EXTRAEVENT));
        REQUIRE_FALSE(listener->HasSubscribedToEvent(StringHash("SecondEvent")));
    }

    SECTION("listed events survive regardless of the flag")
    {
        PODVector<StringHash> keep;
        keep.Push(StringHash("SecondEvent"));

        listener->UnsubscribeFromAllEventsExcept(keep, false);
        REQUIRE_FALSE(listener->HasSubscribedToEvent(E_EXTRAEVENT));
        REQUIRE(listener->HasSubscribedToEvent(StringHash("SecondEvent")));
    }
}

TEST_CASE("TypeInfo matches by type identity rather than pointer identity", "[Core]")
{
    const TypeInfo duplicate("Subscriber", Object::GetTypeInfoStatic());

    REQUIRE(&duplicate != Subscriber::GetTypeInfoStatic());
    REQUIRE(duplicate.GetType() == Subscriber::GetTypeStatic());
    REQUIRE(Subscriber::GetTypeInfoStatic()->IsTypeOf(&duplicate));

    SECTION("an unrelated type still does not match")
    {
        const TypeInfo unrelated("SomethingElse", Object::GetTypeInfoStatic());
        REQUIRE_FALSE(Subscriber::GetTypeInfoStatic()->IsTypeOf(&unrelated));
    }

    SECTION("the chain is walked, not just the leaf")
    {
        REQUIRE(Subscriber::GetTypeInfoStatic()->GetBaseTypeInfo() == Object::GetTypeInfoStatic());
        REQUIRE(Subscriber::GetTypeInfoStatic()->IsTypeOf(Subscriber::GetTypeStatic()));
        REQUIRE_FALSE(Subscriber::GetTypeInfoStatic()->IsTypeOf(StringHash("Unrelated")));
    }
}

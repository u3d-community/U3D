#include "TestUtils.h"

#include <Urho3D/Container/Ptr.h>
#include <Urho3D/Container/RefCounted.h>

using namespace Urho3D;

namespace
{

class Counted : public RefCounted
{
public:
    static int liveCount_;

    Counted() : value_(0) { ++liveCount_; }
    explicit Counted(int value) : value_(value) { ++liveCount_; }
    ~Counted() override { --liveCount_; }

    int value_;
};

int Counted::liveCount_ = 0;

struct Plain
{
    static int liveCount_;

    Plain() { ++liveCount_; }
    ~Plain() { --liveCount_; }

    int value_{0};
};

int Plain::liveCount_ = 0;

}

TEST_CASE("SharedPtr owns and releases", "[Container]")
{
    Counted::liveCount_ = 0;

    {
        SharedPtr<Counted> ptr(new Counted(7));
        REQUIRE(ptr.NotNull());
        REQUIRE_FALSE(ptr.Null());
        REQUIRE(static_cast<bool>(ptr));
        REQUIRE(ptr->value_ == 7);
        REQUIRE((*ptr).value_ == 7);
        REQUIRE(ptr.Get() != nullptr);
        REQUIRE(ptr.Refs() == 1);
        REQUIRE(Counted::liveCount_ == 1);
    }

    REQUIRE(Counted::liveCount_ == 0);
}

TEST_CASE("SharedPtr copies share ownership", "[Container]")
{
    Counted::liveCount_ = 0;

    SharedPtr<Counted> first(new Counted(1));
    REQUIRE(first.Refs() == 1);

    {
        SharedPtr<Counted> second(first);
        REQUIRE(first.Refs() == 2);
        REQUIRE(second.Get() == first.Get());
        REQUIRE(second == first);
    }

    REQUIRE(first.Refs() == 1);
    REQUIRE(Counted::liveCount_ == 1);

    SECTION("assignment retargets and releases the old object")
    {
        SharedPtr<Counted> other(new Counted(2));
        REQUIRE(Counted::liveCount_ == 2);
        other = first;
        REQUIRE(Counted::liveCount_ == 1);
        REQUIRE(other->value_ == 1);
        REQUIRE(first.Refs() == 2);
    }

    SECTION("self assignment does not destroy the object")
    {
        SharedPtr<Counted>& alias = first;
        first = alias;
        REQUIRE(Counted::liveCount_ == 1);
        REQUIRE(first->value_ == 1);
    }

    SECTION("Reset releases the reference")
    {
        SharedPtr<Counted> temp(first);
        REQUIRE(first.Refs() == 2);
        temp.Reset();
        REQUIRE(temp.Null());
        REQUIRE(first.Refs() == 1);
    }

    SECTION("Swap exchanges the targets")
    {
        SharedPtr<Counted> other(new Counted(2));
        first.Swap(other);
        REQUIRE(first->value_ == 2);
        REQUIRE(other->value_ == 1);
    }

    SECTION("Detach yields a raw pointer with no references left")
    {
        Counted::liveCount_ = 0;
        SharedPtr<Counted> sole(new Counted(3));
        Counted* raw = sole.Detach();

        REQUIRE(raw != nullptr);
        REQUIRE(raw->value_ == 3);
        REQUIRE(sole.Null());
        REQUIRE(raw->Refs() == 0);
        REQUIRE(Counted::liveCount_ == 1);

        delete raw;
        REQUIRE(Counted::liveCount_ == 0);
    }

    SECTION("ToHash distinguishes distinct objects")
    {
        SharedPtr<Counted> other(new Counted(2));
        REQUIRE(first.ToHash() != other.ToHash());
        REQUIRE(first != other);
    }
}

TEST_CASE("WeakPtr does not keep the object alive", "[Container]")
{
    Counted::liveCount_ = 0;

    WeakPtr<Counted> weak;
    REQUIRE(weak.Expired());
    REQUIRE(weak.Null());

    {
        SharedPtr<Counted> strong(new Counted(5));
        weak = strong;

        REQUIRE_FALSE(weak.Expired());
        REQUIRE(weak.NotNull());
        REQUIRE(weak->value_ == 5);
        REQUIRE(weak.Get() == strong.Get());
        REQUIRE(strong.Refs() == 1);
        REQUIRE(weak.WeakRefs() >= 1);

        SharedPtr<Counted> locked = weak.Lock();
        REQUIRE(locked.NotNull());
        REQUIRE(locked->value_ == 5);
    }

    REQUIRE(Counted::liveCount_ == 0);
    REQUIRE(weak.Expired());
    REQUIRE(weak.Get() == nullptr);
    REQUIRE(weak.Lock().Null());
}

TEST_CASE("WeakPtr copies and resets", "[Container]")
{
    SharedPtr<Counted> strong(new Counted(1));

    WeakPtr<Counted> a(strong);
    WeakPtr<Counted> b(a);
    REQUIRE(a == b);
    REQUIRE_FALSE(b.Expired());

    b.Reset();
    REQUIRE(b.Expired());
    REQUIRE_FALSE(a.Expired());

    WeakPtr<Counted> fromRaw(strong.Get());
    REQUIRE(fromRaw.Get() == strong.Get());

    SECTION("Swap exchanges targets")
    {
        SharedPtr<Counted> other(new Counted(2));
        WeakPtr<Counted> weakOther(other);
        a.Swap(weakOther);
        REQUIRE(a->value_ == 2);
        REQUIRE(weakOther->value_ == 1);
    }
}

TEST_CASE("UniquePtr has sole ownership", "[Container]")
{
    Plain::liveCount_ = 0;

    {
        UniquePtr<Plain> ptr(new Plain());
        REQUIRE(static_cast<bool>(ptr));
        REQUIRE(ptr.Get() != nullptr);
        REQUIRE(Plain::liveCount_ == 1);
        ptr->value_ = 42;
        REQUIRE((*ptr).value_ == 42);
    }

    REQUIRE(Plain::liveCount_ == 0);

    SECTION("Reset destroys the held object")
    {
        UniquePtr<Plain> ptr(new Plain());
        REQUIRE(Plain::liveCount_ == 1);
        ptr.Reset();
        REQUIRE(Plain::liveCount_ == 0);
        REQUIRE_FALSE(static_cast<bool>(ptr));
    }

    SECTION("Detach hands ownership to the caller")
    {
        UniquePtr<Plain> ptr(new Plain());
        Plain* raw = ptr.Detach();
        REQUIRE(raw != nullptr);
        REQUIRE_FALSE(static_cast<bool>(ptr));
        REQUIRE(Plain::liveCount_ == 1);
        delete raw;
        REQUIRE(Plain::liveCount_ == 0);
    }

    SECTION("moving transfers ownership without copying")
    {
        UniquePtr<Plain> source(new Plain());
        Plain* raw = source.Get();
        UniquePtr<Plain> target(std::move(source));
        REQUIRE(target.Get() == raw);
        REQUIRE(source.Get() == nullptr);
        REQUIRE(Plain::liveCount_ == 1);
    }

    SECTION("Swap exchanges the held objects")
    {
        UniquePtr<Plain> a(new Plain());
        UniquePtr<Plain> b(new Plain());
        a->value_ = 1;
        b->value_ = 2;
        a.Swap(b);
        REQUIRE(a->value_ == 2);
        REQUIRE(b->value_ == 1);
    }
}

TEST_CASE("RefCounted tracks its own reference counts", "[Container]")
{
    Counted::liveCount_ = 0;

    auto* object = new Counted(1);
    REQUIRE(object->Refs() == 0);

    object->AddRef();
    REQUIRE(object->Refs() == 1);

    object->AddRef();
    REQUIRE(object->Refs() == 2);

    object->ReleaseRef();
    REQUIRE(object->Refs() == 1);
    REQUIRE(Counted::liveCount_ == 1);

    object->ReleaseRef();
    REQUIRE(Counted::liveCount_ == 0);
}

TEST_CASE("StaticCast and DynamicCast between shared pointers", "[Container]")
{
    class Derived : public Counted
    {
    public:
        Derived() : Counted(9) {}
    };

    SharedPtr<Derived> derived(new Derived());
    SharedPtr<Counted> base(derived);
    REQUIRE(base.Get() == derived.Get());
    REQUIRE(base.Refs() == 2);

    const SharedPtr<Derived> backStatic = StaticCast<Derived>(base);
    REQUIRE(backStatic.Get() == derived.Get());

    const SharedPtr<Derived> backDynamic = DynamicCast<Derived>(base);
    REQUIRE(backDynamic.Get() == derived.Get());

    SECTION("a failing dynamic cast yields null")
    {
        SharedPtr<Counted> plain(new Counted(1));
        REQUIRE(DynamicCast<Derived>(plain).Null());
    }
}

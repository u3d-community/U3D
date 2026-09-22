#include "TestUtils.h"

#include <Urho3D/Container/Allocator.h>
#include <Urho3D/Container/ArrayPtr.h>
#include <Urho3D/Container/FlagSet.h>
#include <Urho3D/Container/Hash.h>
#include <Urho3D/Container/LinkedList.h>
#include <Urho3D/Container/Pair.h>
#include <Urho3D/Container/Str.h>
#include <Urho3D/Container/Swap.h>
#include <Urho3D/Container/Vector.h>

using namespace Urho3D;

namespace
{

enum class Permission : unsigned
{
    None = 0,
    Read = 1,
    Write = 2,
    Execute = 4,
};
URHO3D_FLAGSET(Permission, PermissionFlags);

struct Element : public LinkedListNode
{
    explicit Element(int value = 0) : value_(value) {}
    int value_;
};

unsigned CountElements(const LinkedList<Element>& list)
{
    unsigned count = 0;
    for (Element* element = list.First(); element; element = list.Next(element))
        ++count;
    return count;
}

}

TEST_CASE("FlagSet combines and tests flags", "[Container]")
{
    PermissionFlags flags;
    REQUIRE(flags.AsInteger() == 0);
    REQUIRE_FALSE(static_cast<bool>(flags));
    REQUIRE(!flags);

    flags |= Permission::Read;
    REQUIRE(flags.Test(Permission::Read));
    REQUIRE_FALSE(flags.Test(Permission::Write));
    REQUIRE(static_cast<bool>(flags));

    flags |= Permission::Write;
    REQUIRE(flags.Test(Permission::Read));
    REQUIRE(flags.Test(Permission::Write));
    REQUIRE(flags.AsInteger() == 3);

    flags &= Permission::Read;
    REQUIRE(flags.Test(Permission::Read));
    REQUIRE_FALSE(flags.Test(Permission::Write));

    flags ^= Permission::Read;
    REQUIRE(flags.AsInteger() == 0);

    SECTION("construction from an enum and from an integer")
    {
        REQUIRE(PermissionFlags(Permission::Write).AsInteger() == 2);
        REQUIRE(PermissionFlags(5u).Test(Permission::Read));
        REQUIRE(PermissionFlags(5u).Test(Permission::Execute));
    }

    SECTION("the free operators build a flag set from bare enums")
    {
        const PermissionFlags combined = Permission::Read | Permission::Write;
        REQUIRE(combined.AsInteger() == 3);

        REQUIRE((Permission::Read & Permission::Read).AsInteger() == 1);
        REQUIRE((Permission::Read & Permission::Write).AsInteger() == 0);
        REQUIRE((Permission::Read ^ Permission::Read).AsInteger() == 0);
        REQUIRE((~Permission::Read).Test(Permission::Write));
    }

    SECTION("flag set to flag set operators")
    {
        const PermissionFlags read(Permission::Read);
        const PermissionFlags write(Permission::Write);

        REQUIRE((read | write).AsInteger() == 3);
        REQUIRE((read & write).AsInteger() == 0);
        REQUIRE((read ^ read).AsInteger() == 0);

        PermissionFlags accumulate(Permission::Read);
        accumulate |= write;
        REQUIRE(accumulate.AsInteger() == 3);
        accumulate &= read;
        REQUIRE(accumulate.AsInteger() == 1);
        accumulate ^= read;
        REQUIRE(accumulate.AsInteger() == 0);
    }

    SECTION("comparison against enums and flag sets")
    {
        const PermissionFlags read(Permission::Read);
        REQUIRE(read == Permission::Read);
        REQUIRE(read != Permission::Write);
        REQUIRE(read == PermissionFlags(Permission::Read));
        REQUIRE(read != PermissionFlags(Permission::Write));
    }

    SECTION("conversions and hashing")
    {
        const PermissionFlags read(Permission::Read);
        REQUIRE(static_cast<Permission>(read) == Permission::Read);
        REQUIRE(static_cast<unsigned>(read) == 1u);
        REQUIRE(read.ToHash() == 1u);
        REQUIRE(read.Test(1u));
        REQUIRE_FALSE(read.Test(2u));
    }
}

TEST_CASE("Pair holds two values", "[Container]")
{
    const Pair<int, String> pair(1, "one");
    REQUIRE(pair.first_ == 1);
    REQUIRE(pair.second_ == "one");

    REQUIRE(MakePair(1, String("one")) == pair);
    REQUIRE(MakePair(2, String("two")) != pair);

    SECTION("ordering compares the first element then the second")
    {
        REQUIRE(MakePair(1, 1) < MakePair(2, 0));
        REQUIRE(MakePair(1, 1) < MakePair(1, 2));
        REQUIRE_FALSE(MakePair(2, 0) < MakePair(1, 1));

        REQUIRE(MakePair(2, 0) > MakePair(1, 1));
        REQUIRE(MakePair(1, 2) > MakePair(1, 1));
        REQUIRE_FALSE(MakePair(1, 1) > MakePair(2, 0));
    }

    SECTION("hashing mixes both members")
    {
        REQUIRE(MakePair(1, 2).ToHash() == MakePair(1, 2).ToHash());
        REQUIRE(MakePair(1, 2).ToHash() != MakePair(2, 1).ToHash());
    }

    SECTION("a default constructed pair is value initialised")
    {
        const Pair<int, int> defaulted = Pair<int, int>();
        REQUIRE(defaulted.first_ == 0);
        REQUIRE(defaulted.second_ == 0);
    }
}

TEST_CASE("MakeHash covers the built in types", "[Container]")
{
    REQUIRE(MakeHash(42) == MakeHash(42));
    REQUIRE(MakeHash(42) != MakeHash(43));

    REQUIRE(MakeHash(42u) == MakeHash(42u));
    REQUIRE(MakeHash(static_cast<short>(7)) == MakeHash(static_cast<short>(7)));
    REQUIRE(MakeHash(static_cast<unsigned short>(7)) == MakeHash(static_cast<unsigned short>(7)));
    REQUIRE(MakeHash('a') == MakeHash('a'));
    REQUIRE(MakeHash(static_cast<unsigned char>(1)) == MakeHash(static_cast<unsigned char>(1)));
    REQUIRE(MakeHash(1234567890123LL) == MakeHash(1234567890123LL));
    REQUIRE(MakeHash(1234567890123ULL) == MakeHash(1234567890123ULL));

    SECTION("pointers hash by address")
    {
        int first = 1;
        int second = 2;
        REQUIRE(MakeHash(&first) == MakeHash(&first));
        REQUIRE(MakeHash(&first) != MakeHash(&second));

        const int* constPointer = &first;
        REQUIRE(MakeHash(constPointer) == MakeHash(constPointer));

        void* voidPointer = &first;
        REQUIRE(MakeHash(voidPointer) == MakeHash(voidPointer));

        const void* constVoidPointer = &first;
        REQUIRE(MakeHash(constVoidPointer) == MakeHash(constVoidPointer));
    }

    SECTION("a type with its own ToHash is used directly")
    {
        REQUIRE(MakeHash(String("abc")) == String("abc").ToHash());
    }

    SECTION("CombineHash folds a second hash in")
    {
        unsigned result = 0;
        CombineHash(result, MakeHash(1));
        const unsigned afterFirst = result;
        CombineHash(result, MakeHash(2));
        REQUIRE(result != afterFirst);
    }
}

TEST_CASE("Swap exchanges values", "[Container]")
{
    int a = 1;
    int b = 2;
    Swap(a, b);
    REQUIRE(a == 2);
    REQUIRE(b == 1);

    SECTION("the String specialisation swaps buffers")
    {
        String first("first");
        String second("second");
        Swap(first, second);
        REQUIRE(first == "second");
        REQUIRE(second == "first");
    }

    SECTION("the Vector specialisation swaps contents")
    {
        PODVector<int> firstVector;
        firstVector.Push(1);
        PODVector<int> secondVector;
        secondVector.Push(2);
        secondVector.Push(3);

        Swap(firstVector, secondVector);
        REQUIRE(firstVector.Size() == 2);
        REQUIRE(secondVector.Size() == 1);
        REQUIRE(firstVector[0] == 2);
    }

    SECTION("the HashMap specialisation swaps contents")
    {
        HashMap<int, int> firstMap;
        firstMap[1] = 1;
        HashMap<int, int> secondMap;
        secondMap[2] = 2;
        secondMap[3] = 3;

        Swap(firstMap, secondMap);
        REQUIRE(firstMap.Size() == 2);
        REQUIRE(secondMap.Size() == 1);
    }

    SECTION("the List specialisation swaps contents")
    {
        List<int> firstList;
        firstList.Push(1);
        List<int> secondList;
        secondList.Push(2);
        secondList.Push(3);

        Swap(firstList, secondList);
        REQUIRE(firstList.Size() == 2);
        REQUIRE(secondList.Size() == 1);
    }
}

TEST_CASE("LinkedList links elements in insertion order", "[Container]")
{
    LinkedList<Element> list;
    REQUIRE(list.Empty());
    REQUIRE(list.First() == nullptr);
    REQUIRE(CountElements(list) == 0);

    list.Insert(new Element(1));
    list.Insert(new Element(2));
    list.Insert(new Element(3));

    REQUIRE_FALSE(list.Empty());
    REQUIRE(CountElements(list) == 3);
    REQUIRE(list.First()->value_ == 1);
    REQUIRE(list.Last()->value_ == 3);

    SECTION("Next walks the chain")
    {
        Element* element = list.First();
        REQUIRE(element->value_ == 1);
        element = list.Next(element);
        REQUIRE(element->value_ == 2);
        element = list.Next(element);
        REQUIRE(element->value_ == 3);
        REQUIRE(list.Next(element) == nullptr);
    }

    SECTION("InsertFront puts the element at the head")
    {
        list.InsertFront(new Element(0));
        REQUIRE(list.First()->value_ == 0);
        REQUIRE(CountElements(list) == 4);
    }

    SECTION("Erase removes a specific element")
    {
        Element* second = list.Next(list.First());
        REQUIRE(list.Erase(second));
        REQUIRE(CountElements(list) == 2);
        REQUIRE(list.First()->value_ == 1);
        REQUIRE(list.Next(list.First())->value_ == 3);
    }

    SECTION("erasing the head relinks correctly")
    {
        REQUIRE(list.Erase(list.First()));
        REQUIRE(list.First()->value_ == 2);
        REQUIRE(CountElements(list) == 2);
    }

    SECTION("erasing something absent reports failure")
    {
        Element stray(99);
        REQUIRE_FALSE(list.Erase(&stray));
        REQUIRE(CountElements(list) == 3);
    }

    SECTION("Clear destroys every element")
    {
        list.Clear();
        REQUIRE(list.Empty());
        REQUIRE(list.First() == nullptr);
    }

    SECTION("Erase with an explicit previous relinks without a scan")
    {
        Element* first = list.First();
        Element* second = list.Next(first);
        REQUIRE(list.Erase(second, first));
        REQUIRE(CountElements(list) == 2);
        REQUIRE(list.Next(list.First())->value_ == 3);
    }
}

TEST_CASE("SharedArrayPtr manages an array", "[Container]")
{
    SharedArrayPtr<int> array(new int[4]);
    REQUIRE(array.NotNull());
    REQUIRE_FALSE(array.Null());
    REQUIRE(static_cast<bool>(array));
    REQUIRE(array.Refs() == 1);

    for (int i = 0; i < 4; ++i)
        array[i] = i * 10;
    REQUIRE(array[2] == 20);
    REQUIRE(array.Get()[3] == 30);

    SECTION("copies share the array")
    {
        SharedArrayPtr<int> copy(array);
        REQUIRE(array.Refs() == 2);
        REQUIRE(copy.Get() == array.Get());
        REQUIRE(copy == array);
        copy[0] = 99;
        REQUIRE(array[0] == 99);
    }

    SECTION("assignment retargets")
    {
        SharedArrayPtr<int> other(new int[2]);
        other = array;
        REQUIRE(other.Get() == array.Get());
        REQUIRE(array.Refs() == 2);
    }

    SECTION("Reset releases the reference")
    {
        SharedArrayPtr<int> copy(array);
        copy.Reset();
        REQUIRE(copy.Null());
        REQUIRE(array.Refs() == 1);
    }

    SECTION("ToHash distinguishes distinct arrays")
    {
        SharedArrayPtr<int> other(new int[2]);
        REQUIRE(array.ToHash() != other.ToHash());
        REQUIRE(array != other);
    }
}

TEST_CASE("WeakArrayPtr observes without owning", "[Container]")
{
    WeakArrayPtr<int> weak;
    REQUIRE(weak.Expired());

    {
        SharedArrayPtr<int> strong(new int[2]);
        strong[0] = 5;

        weak = strong;
        REQUIRE_FALSE(weak.Expired());
        REQUIRE(weak.Get() == strong.Get());
        REQUIRE(weak.NotNull());
        REQUIRE(weak.WeakRefs() >= 1);
        REQUIRE(weak.Refs() == 1);
        REQUIRE(weak[0] == 5);

        SharedArrayPtr<int> locked = weak.Lock();
        REQUIRE(locked.NotNull());
        REQUIRE(locked.Get() == strong.Get());
        REQUIRE(locked.Refs() == 2);
        REQUIRE(strong.Refs() == 2);
        REQUIRE(locked[0] == 5);
    }

    REQUIRE(weak.Expired());
    REQUIRE(weak.Get() == nullptr);
    REQUIRE(weak.Lock().Null());

    SECTION("Reset clears the observation")
    {
        SharedArrayPtr<int> strong(new int[1]);
        WeakArrayPtr<int> observer(strong);
        REQUIRE_FALSE(observer.Expired());
        observer.Reset();
        REQUIRE(observer.Expired());
    }
}

TEST_CASE("Allocator recycles fixed size blocks", "[Container]")
{
    AllocatorBlock* allocator = AllocatorInitialize(sizeof(int), 4);
    REQUIRE(allocator != nullptr);

    void* first = AllocatorReserve(allocator);
    void* second = AllocatorReserve(allocator);
    REQUIRE(first != nullptr);
    REQUIRE(second != nullptr);
    REQUIRE(first != second);

    AllocatorFree(allocator, first);
    void* recycled = AllocatorReserve(allocator);
    REQUIRE(recycled == first);

    AllocatorFree(allocator, second);
    AllocatorFree(allocator, recycled);

    SECTION("it grows past the initial capacity")
    {
        PODVector<void*> blocks;
        for (int i = 0; i < 32; ++i)
        {
            void* block = AllocatorReserve(allocator);
            REQUIRE(block != nullptr);
            blocks.Push(block);
        }
        for (unsigned i = 0; i < blocks.Size(); ++i)
            AllocatorFree(allocator, blocks[i]);
    }

    AllocatorUninitialize(allocator);
}

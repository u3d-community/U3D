#include "TestUtils.h"

#include <Urho3D/Container/Sort.h>
#include <Urho3D/Container/Str.h>
#include <Urho3D/Container/Vector.h>

using namespace Urho3D;

namespace
{

struct Tracked
{
    static int liveCount_;

    Tracked() : value_(0) { ++liveCount_; }
    explicit Tracked(int value) : value_(value) { ++liveCount_; }
    Tracked(const Tracked& rhs) : value_(rhs.value_) { ++liveCount_; }
    Tracked& operator =(const Tracked& rhs) { value_ = rhs.value_; return *this; }
    ~Tracked() { --liveCount_; }

    bool operator ==(const Tracked& rhs) const { return value_ == rhs.value_; }

    int value_;
};

int Tracked::liveCount_ = 0;

}

TEST_CASE("PODVector basic operations", "[Container]")
{
    PODVector<int> v;
    REQUIRE(v.Empty());
    REQUIRE(v.Size() == 0);

    v.Push(1);
    v.Push(2);
    v.Push(3);

    REQUIRE(v.Size() == 3);
    REQUIRE_FALSE(v.Empty());
    REQUIRE(v[0] == 1);
    REQUIRE(v[2] == 3);
    REQUIRE(v.At(1) == 2);
    REQUIRE(v.Front() == 1);
    REQUIRE(v.Back() == 3);
    REQUIRE(v.Buffer()[0] == 1);

    v.Pop();
    REQUIRE(v.Size() == 2);
    REQUIRE(v.Back() == 2);

    v.Clear();
    REQUIRE(v.Empty());
}

TEST_CASE("PODVector construction and assignment", "[Container]")
{
    PODVector<int> source;
    source.Push(1);
    source.Push(2);

    const PODVector<int> copy(source);
    REQUIRE(copy.Size() == 2);
    REQUIRE(copy[1] == 2);

    PODVector<int> assigned;
    assigned = source;
    REQUIRE(assigned.Size() == 2);
    REQUIRE(assigned == source);

    PODVector<int> sized(5);
    REQUIRE(sized.Size() == 5);

    const int raw[] = {7, 8, 9};
    const PODVector<int> fromBuffer(raw, 3);
    REQUIRE(fromBuffer.Size() == 3);
    REQUIRE(fromBuffer[2] == 9);

    REQUIRE(source != fromBuffer);
}

TEST_CASE("PODVector Resize and Reserve", "[Container]")
{
    PODVector<int> v;

    v.Resize(3);
    REQUIRE(v.Size() == 3);

    v.Resize(5, 9);
    REQUIRE(v.Size() == 5);
    REQUIRE(v[4] == 9);

    v.Resize(2);
    REQUIRE(v.Size() == 2);

    v.Reserve(100);
    REQUIRE(v.Capacity() >= 100);
    REQUIRE(v.Size() == 2);

    v.Compact();
    REQUIRE(v.Size() == 2);
}

TEST_CASE("PODVector Insert and Erase", "[Container]")
{
    PODVector<int> v;
    for (int i = 0; i < 5; ++i)
        v.Push(i);

    v.Insert(0, 99);
    REQUIRE(v[0] == 99);
    REQUIRE(v.Size() == 6);

    v.Erase(0);
    REQUIRE(v[0] == 0);
    REQUIRE(v.Size() == 5);

    v.Erase(1, 2);
    REQUIRE(v.Size() == 3);
    REQUIRE(v[0] == 0);
    REQUIRE(v[1] == 3);

    SECTION("EraseSwap moves the last element into the gap")
    {
        PODVector<int> swap;
        for (int i = 0; i < 5; ++i)
            swap.Push(i);
        swap.EraseSwap(0);
        REQUIRE(swap.Size() == 4);
        REQUIRE(swap[0] == 4);
    }

    SECTION("Remove deletes the first match only")
    {
        PODVector<int> dupes;
        dupes.Push(1);
        dupes.Push(2);
        dupes.Push(1);
        REQUIRE(dupes.Remove(1));
        REQUIRE(dupes.Size() == 2);
        REQUIRE(dupes[0] == 2);
        REQUIRE(dupes[1] == 1);
        REQUIRE_FALSE(dupes.Remove(42));
    }

    SECTION("RemoveSwap reports whether it removed anything")
    {
        PODVector<int> items;
        items.Push(1);
        items.Push(2);
        REQUIRE(items.RemoveSwap(1));
        REQUIRE(items.Size() == 1);
        REQUIRE_FALSE(items.RemoveSwap(99));
    }

    SECTION("inserting another vector splices it in")
    {
        PODVector<int> other;
        other.Push(100);
        other.Push(101);

        PODVector<int> target;
        target.Push(1);
        target.Push(2);
        target.Insert(1, other);
        REQUIRE(target.Size() == 4);
        REQUIRE(target[1] == 100);
        REQUIRE(target[2] == 101);
        REQUIRE(target[3] == 2);
    }
}

TEST_CASE("PODVector search", "[Container]")
{
    PODVector<int> v;
    v.Push(10);
    v.Push(20);
    v.Push(30);

    REQUIRE(v.Contains(20));
    REQUIRE_FALSE(v.Contains(99));
    REQUIRE(v.IndexOf(30) == 2);
    REQUIRE(v.IndexOf(99) == v.Size());

    REQUIRE(v.Find(20) != v.End());
    REQUIRE(v.Find(99) == v.End());

    const PODVector<int>& constVector = v;
    REQUIRE(constVector.Find(10) == constVector.Begin());
}

TEST_CASE("PODVector iteration", "[Container]")
{
    PODVector<int> v;
    for (int i = 0; i < 4; ++i)
        v.Push(i);

    int sum = 0;
    for (PODVector<int>::Iterator i = v.Begin(); i != v.End(); ++i)
        sum += *i;
    REQUIRE(sum == 6);

    const PODVector<int>& constVector = v;
    sum = 0;
    for (PODVector<int>::ConstIterator i = constVector.Begin(); i != constVector.End(); ++i)
        sum += *i;
    REQUIRE(sum == 6);

    SECTION("erasing through an iterator returns the following element")
    {
        PODVector<int>::Iterator next = v.Erase(v.Begin());
        REQUIRE(*next == 1);
        REQUIRE(v.Size() == 3);
    }

    SECTION("erasing an iterator range")
    {
        v.Erase(v.Begin(), v.Begin() + 2);
        REQUIRE(v.Size() == 2);
        REQUIRE(v[0] == 2);
    }
}

TEST_CASE("Vector manages element lifetimes", "[Container]")
{
    Tracked::liveCount_ = 0;

    {
        Vector<Tracked> v;
        v.Push(Tracked(1));
        v.Push(Tracked(2));
        REQUIRE(v.Size() == 2);
        REQUIRE(Tracked::liveCount_ == 2);

        v.Pop();
        REQUIRE(Tracked::liveCount_ == 1);

        v.Push(Tracked(3));
        v.Push(Tracked(4));
        REQUIRE(Tracked::liveCount_ == 3);

        v.Clear();
        REQUIRE(Tracked::liveCount_ == 0);
    }

    REQUIRE(Tracked::liveCount_ == 0);

    SECTION("destroying a populated vector destroys its elements")
    {
        {
            Vector<Tracked> v;
            for (int i = 0; i < 10; ++i)
                v.Push(Tracked(i));
            REQUIRE(Tracked::liveCount_ == 10);
        }
        REQUIRE(Tracked::liveCount_ == 0);
    }

    SECTION("copying a vector copies its elements")
    {
        Vector<Tracked> original;
        original.Push(Tracked(1));
        {
            Vector<Tracked> copy(original);
            REQUIRE(Tracked::liveCount_ == 2);
            REQUIRE(copy[0].value_ == 1);
        }
        REQUIRE(Tracked::liveCount_ == 1);
    }

    SECTION("erasing destroys only the removed elements")
    {
        Vector<Tracked> v;
        for (int i = 0; i < 5; ++i)
            v.Push(Tracked(i));
        v.Erase(1, 2);
        REQUIRE(Tracked::liveCount_ == 3);
        REQUIRE(v[0].value_ == 0);
        REQUIRE(v[1].value_ == 3);
    }
}

TEST_CASE("Vector of strings", "[Container]")
{
    Vector<String> v;
    v.Push("one");
    v.Push("two");
    v.Push("three");

    REQUIRE(v.Size() == 3);
    REQUIRE(v[1] == "two");
    REQUIRE(v.Contains("three"));
    REQUIRE(v.IndexOf("one") == 0);

    v.Insert(1, String("inserted"));
    REQUIRE(v[1] == "inserted");
    REQUIRE(v.Size() == 4);

    REQUIRE(v.Remove("inserted"));
    REQUIRE(v.Size() == 3);
    REQUIRE(v[1] == "two");

    Vector<String> copy = v;
    REQUIRE(copy == v);
    copy.Push("extra");
    REQUIRE(copy != v);
}

TEST_CASE("Vector non-POD operations preserve exact contents", "[Container]")
{
    Vector<String> values;
    values.Push("zero");
    values.Push("one");
    values.Push("two");
    values.Push("three");
    values.Push("four");

    const Vector<String> appended = values + String("five");
    REQUIRE(appended.Size() == 6);
    REQUIRE(appended[4] == "four");
    REQUIRE(appended[5] == "five");
    REQUIRE(values.Size() == 5);

    REQUIRE(values.Find("two") == values.Begin() + 2);
    REQUIRE(values.Find("missing") == values.End());

    values.Erase(1, 2);
    REQUIRE(values.Size() == 3);
    REQUIRE(values[0] == "zero");
    REQUIRE(values[1] == "three");
    REQUIRE(values[2] == "four");
}

TEST_CASE("Sort orders a vector", "[Container]")
{
    PODVector<int> v;
    v.Push(5);
    v.Push(1);
    v.Push(4);
    v.Push(2);
    v.Push(3);

    Sort(v.Begin(), v.End());
    for (unsigned i = 0; i + 1 < v.Size(); ++i)
        REQUIRE(v[i] <= v[i + 1]);

    SECTION("a custom comparator reverses the order")
    {
        Sort(v.Begin(), v.End(), [](int lhs, int rhs) { return lhs > rhs; });
        for (unsigned i = 0; i + 1 < v.Size(); ++i)
            REQUIRE(v[i] >= v[i + 1]);
    }

    SECTION("sorting an already sorted range keeps it sorted")
    {
        Sort(v.Begin(), v.End());
        PODVector<int> again = v;
        Sort(again.Begin(), again.End());
        REQUIRE(again == v);
    }

    SECTION("sorting an empty or single element range is safe")
    {
        PODVector<int> empty;
        Sort(empty.Begin(), empty.End());
        REQUIRE(empty.Empty());

        PODVector<int> single;
        single.Push(42);
        Sort(single.Begin(), single.End());
        REQUIRE(single[0] == 42);
    }

    SECTION("strings sort lexicographically")
    {
        Vector<String> names;
        names.Push("charlie");
        names.Push("alpha");
        names.Push("bravo");
        Sort(names.Begin(), names.End());
        REQUIRE(names[0] == "alpha");
        REQUIRE(names[1] == "bravo");
        REQUIRE(names[2] == "charlie");
    }
}

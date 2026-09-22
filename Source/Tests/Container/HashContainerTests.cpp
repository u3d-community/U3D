#include "TestUtils.h"

#include <Urho3D/Container/HashMap.h>
#include <Urho3D/Container/HashSet.h>
#include <Urho3D/Container/List.h>
#include <Urho3D/Container/Str.h>

using namespace Urho3D;

TEST_CASE("HashMap insert and lookup", "[Container]")
{
    HashMap<String, int> map;
    REQUIRE(map.Empty());
    REQUIRE(map.Size() == 0);

    map["one"] = 1;
    map["two"] = 2;

    REQUIRE(map.Size() == 2);
    REQUIRE_FALSE(map.Empty());
    REQUIRE(map["one"] == 1);
    REQUIRE(map["two"] == 2);

    REQUIRE(map.Contains("one"));
    REQUIRE_FALSE(map.Contains("three"));

    REQUIRE(map.Find("one") != map.End());
    REQUIRE(map.Find("three") == map.End());
    REQUIRE(map.Find("two")->second_ == 2);

    SECTION("operator[] default constructs a missing key")
    {
        REQUIRE(map["missing"] == 0);
        REQUIRE(map.Size() == 3);
    }

    SECTION("TryGetValue reports presence without inserting")
    {
        int value = -1;
        REQUIRE(map.TryGetValue("one", value));
        REQUIRE(value == 1);
        REQUIRE_FALSE(map.TryGetValue("absent", value));
        REQUIRE(map.Size() == 2);
    }

    SECTION("Insert reports whether the key already existed")
    {
        bool exists = false;
        map.Insert(MakePair(String("three"), 3), exists);
        REQUIRE_FALSE(exists);
        REQUIRE(map.Size() == 3);

        map.Insert(MakePair(String("three"), 33), exists);
        REQUIRE(exists);
        REQUIRE(map.Size() == 3);
    }

    SECTION("Populate chains insertions")
    {
        HashMap<String, int> populated;
        populated.Populate("a", 1).Populate("b", 2);
        REQUIRE(populated.Size() == 2);
        REQUIRE(populated["b"] == 2);
    }
}

TEST_CASE("HashMap erase and clear", "[Container]")
{
    HashMap<int, String> map;
    for (int i = 0; i < 5; ++i)
        map[i] = String(i);

    REQUIRE(map.Size() == 5);

    REQUIRE(map.Erase(2));
    REQUIRE(map.Size() == 4);
    REQUIRE_FALSE(map.Contains(2));

    REQUIRE_FALSE(map.Erase(99));
    REQUIRE(map.Size() == 4);

    map.Clear();
    REQUIRE(map.Empty());
    REQUIRE(map.Size() == 0);

    SECTION("erasing through an iterator")
    {
        HashMap<int, int> other;
        other[1] = 10;
        other[2] = 20;
        other.Erase(other.Find(1));
        REQUIRE(other.Size() == 1);
        REQUIRE_FALSE(other.Contains(1));
    }
}

TEST_CASE("HashMap iteration and views", "[Container]")
{
    HashMap<String, int> map;
    map["a"] = 1;
    map["b"] = 2;
    map["c"] = 3;

    int sum = 0;
    unsigned count = 0;
    for (HashMap<String, int>::ConstIterator i = map.Begin(); i != map.End(); ++i)
    {
        sum += i->second_;
        ++count;
    }
    REQUIRE(count == 3);
    REQUIRE(sum == 6);

    const Vector<String> keys = map.Keys();
    const Vector<int> values = map.Values();
    REQUIRE(keys.Size() == 3);
    REQUIRE(values.Size() == 3);
    REQUIRE(keys.Contains("a"));
    REQUIRE(values.Contains(3));

    SECTION("mutating through an iterator changes the stored value")
    {
        for (HashMap<String, int>::Iterator i = map.Begin(); i != map.End(); ++i)
            i->second_ *= 10;
        REQUIRE(map["a"] == 10);
        REQUIRE(map["c"] == 30);
    }

    SECTION("Sort orders the traversal by key")
    {
        map.Sort();
        Vector<String> sortedKeys;
        for (HashMap<String, int>::ConstIterator i = map.Begin(); i != map.End(); ++i)
            sortedKeys.Push(i->first_);
        REQUIRE(sortedKeys[0] == "a");
        REQUIRE(sortedKeys[1] == "b");
        REQUIRE(sortedKeys[2] == "c");
    }
}

TEST_CASE("HashMap copy and equality", "[Container]")
{
    HashMap<String, int> source;
    source["a"] = 1;
    source["b"] = 2;

    const HashMap<String, int> copy(source);
    REQUIRE(copy.Size() == 2);
    REQUIRE(copy == source);

    HashMap<String, int> assigned;
    assigned = source;
    REQUIRE(assigned == source);

    assigned["c"] = 3;
    REQUIRE(assigned != source);

    SECTION("inserting another map merges it")
    {
        HashMap<String, int> other;
        other["c"] = 3;
        HashMap<String, int> target(source);
        target.Insert(other);
        REQUIRE(target.Size() == 3);
        REQUIRE(target["c"] == 3);
    }
}

TEST_CASE("HashMap survives many insertions and rehashing", "[Container]")
{
    HashMap<int, int> map;
    const int total = 500;

    for (int i = 0; i < total; ++i)
        map[i] = i * 2;

    REQUIRE(map.Size() == static_cast<unsigned>(total));
    for (int i = 0; i < total; ++i)
    {
        REQUIRE(map.Contains(i));
        REQUIRE(map[i] == i * 2);
    }

    for (int i = 0; i < total; i += 2)
        REQUIRE(map.Erase(i));

    REQUIRE(map.Size() == static_cast<unsigned>(total / 2));
    for (int i = 1; i < total; i += 2)
        REQUIRE(map[i] == i * 2);

    SECTION("an explicit rehash preserves every entry")
    {
        map.Rehash(1024);
        REQUIRE(map.Size() == static_cast<unsigned>(total / 2));
        for (int i = 1; i < total; i += 2)
            REQUIRE(map[i] == i * 2);
    }

    SECTION("rehashing below the required load capacity is rejected")
    {
        const unsigned oldBuckets = map.NumBuckets();
        REQUIRE_FALSE(map.Rehash(1));
        REQUIRE(map.NumBuckets() == oldBuckets);
        REQUIRE(map.Size() == static_cast<unsigned>(total / 2));
    }

    SECTION("rehashing to a sufficient smaller power of two succeeds")
    {
        REQUIRE(map.Rehash(64));
        REQUIRE(map.NumBuckets() == 64);
        for (int i = 1; i < total; i += 2)
            REQUIRE(map[i] == i * 2);
    }
}

TEST_CASE("HashSet stores unique keys", "[Container]")
{
    HashSet<int> set;
    REQUIRE(set.Empty());

    set.Insert(1);
    set.Insert(2);
    set.Insert(1);

    REQUIRE(set.Size() == 2);
    REQUIRE(set.Contains(1));
    REQUIRE(set.Contains(2));
    REQUIRE_FALSE(set.Contains(3));

    SECTION("Insert reports whether the key was already present")
    {
        bool exists = false;
        set.Insert(3, exists);
        REQUIRE_FALSE(exists);
        set.Insert(3, exists);
        REQUIRE(exists);
    }

    SECTION("erase and clear")
    {
        REQUIRE(set.Erase(1));
        REQUIRE_FALSE(set.Erase(99));
        REQUIRE(set.Size() == 1);
        set.Clear();
        REQUIRE(set.Empty());
    }

    SECTION("iteration visits every key once")
    {
        HashSet<int> many;
        for (int i = 0; i < 100; ++i)
            many.Insert(i);

        unsigned count = 0;
        for (HashSet<int>::ConstIterator i = many.Begin(); i != many.End(); ++i)
            ++count;
        REQUIRE(count == 100);
    }

    SECTION("copy and equality")
    {
        HashSet<int> copy(set);
        REQUIRE(copy == set);
        copy.Insert(42);
        REQUIRE(copy != set);
    }

    SECTION("Sort orders the traversal")
    {
        HashSet<int> unsorted;
        unsorted.Insert(3);
        unsorted.Insert(1);
        unsorted.Insert(2);
        unsorted.Sort();
        REQUIRE(unsorted.Front() == 1);
        REQUIRE(unsorted.Back() == 3);
    }
}

TEST_CASE("List supports both ends", "[Container]")
{
    List<int> list;
    REQUIRE(list.Empty());
    REQUIRE(list.Size() == 0);

    list.Push(2);
    list.Push(3);
    list.PushFront(1);

    REQUIRE(list.Size() == 3);
    REQUIRE(list.Front() == 1);
    REQUIRE(list.Back() == 3);

    list.Pop();
    REQUIRE(list.Back() == 2);

    list.PopFront();
    REQUIRE(list.Front() == 2);
    REQUIRE(list.Size() == 1);

    SECTION("search")
    {
        List<int> items;
        items.Push(10);
        items.Push(20);
        REQUIRE(items.Contains(20));
        REQUIRE_FALSE(items.Contains(99));
        REQUIRE(items.Find(10) != items.End());
        REQUIRE(items.Find(99) == items.End());
    }

    SECTION("iteration walks in order")
    {
        List<int> items;
        for (int i = 0; i < 5; ++i)
            items.Push(i);

        int expected = 0;
        for (List<int>::ConstIterator i = items.Begin(); i != items.End(); ++i)
            REQUIRE(*i == expected++);
        REQUIRE(expected == 5);
    }

    SECTION("insert before an iterator")
    {
        List<int> items;
        items.Push(1);
        items.Push(3);
        List<int>::Iterator last = items.Find(3);
        items.Insert(last, 2);

        REQUIRE(items.Size() == 3);
        List<int>::ConstIterator i = items.Begin();
        REQUIRE(*i++ == 1);
        REQUIRE(*i++ == 2);
        REQUIRE(*i == 3);
    }

    SECTION("erase through an iterator returns the next element")
    {
        List<int> items;
        for (int i = 0; i < 3; ++i)
            items.Push(i);
        List<int>::Iterator next = items.Erase(items.Begin());
        REQUIRE(*next == 1);
        REQUIRE(items.Size() == 2);
    }

    SECTION("copy, equality and clear")
    {
        List<int> items;
        items.Push(1);
        items.Push(2);

        List<int> copy(items);
        REQUIRE(copy == items);
        copy.Push(3);
        REQUIRE(copy != items);

        items.Clear();
        REQUIRE(items.Empty());
    }

    SECTION("Resize grows and shrinks")
    {
        List<int> items;
        items.Resize(3);
        REQUIRE(items.Size() == 3);
        items.Resize(1);
        REQUIRE(items.Size() == 1);
    }
}

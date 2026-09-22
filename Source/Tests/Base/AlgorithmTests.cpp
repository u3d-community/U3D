#include "TestUtils.h"

#include <Urho3D/Base/Algorithm.h>
#include <Urho3D/Base/Iter.h>
#include <Urho3D/Container/Str.h>
#include <Urho3D/Container/Vector.h>

using namespace Urho3D;

TEST_CASE("LowerBound finds the first element not less than the value", "[Base]")
{
    PODVector<int> sorted;
    for (int i = 0; i < 10; ++i)
        sorted.Push(i * 2);

    REQUIRE(LowerBound(sorted.Begin(), sorted.End(), 0) == sorted.Begin());
    REQUIRE(*LowerBound(sorted.Begin(), sorted.End(), 6) == 6);
    REQUIRE(*LowerBound(sorted.Begin(), sorted.End(), 7) == 8);
    REQUIRE(LowerBound(sorted.Begin(), sorted.End(), 100) == sorted.End());
    REQUIRE(*LowerBound(sorted.Begin(), sorted.End(), -5) == 0);

    SECTION("duplicates resolve to the first of the run")
    {
        PODVector<int> duplicates;
        duplicates.Push(1);
        duplicates.Push(2);
        duplicates.Push(2);
        duplicates.Push(2);
        duplicates.Push(3);

        const PODVector<int>::Iterator found = LowerBound(duplicates.Begin(), duplicates.End(), 2);
        REQUIRE(found - duplicates.Begin() == 1);
    }

    SECTION("an empty range returns the end")
    {
        PODVector<int> empty;
        REQUIRE(LowerBound(empty.Begin(), empty.End(), 1) == empty.End());
    }

    SECTION("a single element range")
    {
        PODVector<int> single;
        single.Push(5);
        REQUIRE(LowerBound(single.Begin(), single.End(), 4) == single.Begin());
        REQUIRE(LowerBound(single.Begin(), single.End(), 5) == single.Begin());
        REQUIRE(LowerBound(single.Begin(), single.End(), 6) == single.End());
    }
}

TEST_CASE("UpperBound finds the first element greater than the value", "[Base]")
{
    PODVector<int> sorted;
    for (int i = 0; i < 10; ++i)
        sorted.Push(i * 2);

    REQUIRE(*UpperBound(sorted.Begin(), sorted.End(), 0) == 2);
    REQUIRE(*UpperBound(sorted.Begin(), sorted.End(), 6) == 8);
    REQUIRE(*UpperBound(sorted.Begin(), sorted.End(), 7) == 8);
    REQUIRE(UpperBound(sorted.Begin(), sorted.End(), 100) == sorted.End());
    REQUIRE(UpperBound(sorted.Begin(), sorted.End(), -5) == sorted.Begin());

    SECTION("duplicates resolve past the whole run")
    {
        PODVector<int> duplicates;
        duplicates.Push(1);
        duplicates.Push(2);
        duplicates.Push(2);
        duplicates.Push(2);
        duplicates.Push(3);

        const PODVector<int>::Iterator found = UpperBound(duplicates.Begin(), duplicates.End(), 2);
        REQUIRE(found - duplicates.Begin() == 4);
        REQUIRE(*found == 3);
    }

    SECTION("an empty range returns the end")
    {
        PODVector<int> empty;
        REQUIRE(UpperBound(empty.Begin(), empty.End(), 1) == empty.End());
    }

    SECTION("the two bounds bracket the run of equal values")
    {
        PODVector<int> duplicates;
        duplicates.Push(1);
        duplicates.Push(2);
        duplicates.Push(2);
        duplicates.Push(3);

        REQUIRE(UpperBound(duplicates.Begin(), duplicates.End(), 2)
            - LowerBound(duplicates.Begin(), duplicates.End(), 2) == 2);

        REQUIRE(UpperBound(duplicates.Begin(), duplicates.End(), 5)
            == LowerBound(duplicates.Begin(), duplicates.End(), 5));
    }
}

TEST_CASE("Bounds work on strings as well as numbers", "[Base]")
{
    Vector<String> names;
    names.Push("alpha");
    names.Push("bravo");
    names.Push("charlie");

    REQUIRE(*LowerBound(names.Begin(), names.End(), String("bravo")) == "bravo");
    REQUIRE(*UpperBound(names.Begin(), names.End(), String("bravo")) == "charlie");
    REQUIRE(LowerBound(names.Begin(), names.End(), String("zulu")) == names.End());
}

TEST_CASE("RandomAccessIterator navigates a buffer", "[Base]")
{
    int data[] = {10, 20, 30, 40, 50};

    RandomAccessIterator<int> begin(data);
    RandomAccessIterator<int> end(data + 5);

    REQUIRE(*begin == 10);
    REQUIRE(end - begin == 5);

    RandomAccessIterator<int> it = begin;
    REQUIRE(*(++it) == 20);
    REQUIRE(*(it++) == 20);
    REQUIRE(*it == 30);
    REQUIRE(*(--it) == 20);
    REQUIRE(*(it--) == 20);
    REQUIRE(*it == 10);

    it += 3;
    REQUIRE(*it == 40);
    it -= 2;
    REQUIRE(*it == 20);

    REQUIRE(*(begin + 4) == 50);
    REQUIRE(*(end - 1) == 50);

    SECTION("comparisons order by address")
    {
        REQUIRE(begin < end);
        REQUIRE(end > begin);
        REQUIRE(begin <= begin);
        REQUIRE(begin >= begin);
        REQUIRE(begin != end);
        REQUIRE(begin == RandomAccessIterator<int>(data));
    }

    SECTION("a default constructed iterator is null")
    {
        RandomAccessIterator<int> defaulted;
        REQUIRE(defaulted == RandomAccessIterator<int>(nullptr));
    }

    SECTION("the arrow operator reaches members")
    {
        struct Item
        {
            int value_;
        };
        Item items[] = {{1}, {2}};
        RandomAccessIterator<Item> itemIterator(items);
        REQUIRE(itemIterator->value_ == 1);
        REQUIRE((++itemIterator)->value_ == 2);
    }
}

TEST_CASE("RandomAccessConstIterator navigates a const buffer", "[Base]")
{
    const int data[] = {10, 20, 30};

    RandomAccessConstIterator<int> begin(data);
    RandomAccessConstIterator<int> end(data + 3);

    REQUIRE(*begin == 10);
    REQUIRE(end - begin == 3);

    RandomAccessConstIterator<int> it = begin;
    REQUIRE(*(++it) == 20);
    REQUIRE(*(it++) == 20);
    REQUIRE(*it == 30);
    REQUIRE(*(--it) == 20);
    REQUIRE(*(it--) == 20);

    it += 2;
    REQUIRE(*it == 30);
    it -= 1;
    REQUIRE(*it == 20);

    REQUIRE(*(begin + 2) == 30);
    REQUIRE(*(end - 1) == 30);

    REQUIRE(begin < end);
    REQUIRE(end > begin);
    REQUIRE(begin <= begin);
    REQUIRE(begin >= begin);
    REQUIRE(begin != end);
    REQUIRE(begin == RandomAccessConstIterator<int>(data));

    SECTION("it can be built from a mutable iterator")
    {
        int mutableData[] = {1, 2, 3};
        RandomAccessIterator<int> mutableIterator(mutableData);
        RandomAccessConstIterator<int> constIterator(mutableIterator);
        REQUIRE(*constIterator == 1);
    }
}

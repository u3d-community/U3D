#include "TestUtils.h"

#include <Urho3D/Math/AreaAllocator.h>

using namespace Urho3D;

TEST_CASE("AreaAllocator construction", "[Math]")
{
    AreaAllocator allocator(64, 64);
    REQUIRE(allocator.GetWidth() == 64);
    REQUIRE(allocator.GetHeight() == 64);
    REQUIRE(allocator.GetFastMode());

    AreaAllocator slow(32, 32, false);
    REQUIRE_FALSE(slow.GetFastMode());

    AreaAllocator growable(16, 16, 128, 128);
    REQUIRE(growable.GetWidth() == 16);
    REQUIRE(growable.GetHeight() == 16);

    SECTION("Reset changes the size and mode")
    {
        AreaAllocator target(8, 8);
        target.Reset(64, 32, 0, 0, false);
        REQUIRE(target.GetWidth() == 64);
        REQUIRE(target.GetHeight() == 32);
        REQUIRE_FALSE(target.GetFastMode());
    }
}

TEST_CASE("AreaAllocator places non overlapping rectangles", "[Math]")
{
    AreaAllocator allocator(64, 64);

    int x = -1;
    int y = -1;
    REQUIRE(allocator.Allocate(16, 16, x, y));
    REQUIRE(x >= 0);
    REQUIRE(y >= 0);

    SECTION("every allocation lands inside the area and none of them overlap")
    {
        struct Placed
        {
            int x_, y_, w_, h_;
        };
        PODVector<Placed> placed;
        placed.Push({x, y, 16, 16});

        for (int i = 0; i < 8; ++i)
        {
            int nextX = -1;
            int nextY = -1;
            if (!allocator.Allocate(8, 8, nextX, nextY))
                break;

            REQUIRE(nextX >= 0);
            REQUIRE(nextY >= 0);
            REQUIRE(nextX + 8 <= allocator.GetWidth());
            REQUIRE(nextY + 8 <= allocator.GetHeight());

            for (unsigned j = 0; j < placed.Size(); ++j)
            {
                const Placed& other = placed[j];
                const bool disjoint = nextX + 8 <= other.x_ || other.x_ + other.w_ <= nextX
                    || nextY + 8 <= other.y_ || other.y_ + other.h_ <= nextY;
                REQUIRE(disjoint);
            }

            placed.Push({nextX, nextY, 8, 8});
        }

        REQUIRE(placed.Size() > 1);
    }
}

TEST_CASE("AreaAllocator refuses what does not fit", "[Math]")
{
    AreaAllocator allocator(32, 32);

    int x = 0;
    int y = 0;
    REQUIRE_FALSE(allocator.Allocate(64, 64, x, y));
    REQUIRE_FALSE(allocator.Allocate(33, 16, x, y));
    REQUIRE_FALSE(allocator.Allocate(16, 33, x, y));

    SECTION("an exact fit is accepted and exhausts the area")
    {
        REQUIRE(allocator.Allocate(32, 32, x, y));
        REQUIRE(x == 0);
        REQUIRE(y == 0);

        int nextX = 0;
        int nextY = 0;
        REQUIRE_FALSE(allocator.Allocate(1, 1, nextX, nextY));
    }

    SECTION("filling the area eventually fails")
    {
        int count = 0;
        while (true)
        {
            int nextX = 0;
            int nextY = 0;
            if (!allocator.Allocate(8, 8, nextX, nextY))
                break;
            ++count;
            REQUIRE(nextX + 8 <= 32);
            REQUIRE(nextY + 8 <= 32);
            REQUIRE(count <= 16);
        }
        REQUIRE(count > 0);
        REQUIRE(count <= 16);
    }
}

TEST_CASE("AreaAllocator grows up to the maximum size", "[Math]")
{
    AreaAllocator allocator(16, 16, 64, 64);

    int x = 0;
    int y = 0;
    REQUIRE(allocator.Allocate(16, 16, x, y));

    REQUIRE(allocator.Allocate(16, 16, x, y));
    REQUIRE(allocator.GetWidth() > 16);
    REQUIRE(allocator.GetWidth() <= 64);
    REQUIRE(allocator.GetHeight() <= 64);

    SECTION("growth stops at the maximum")
    {
        for (int i = 0; i < 64; ++i)
        {
            int nextX = 0;
            int nextY = 0;
            if (!allocator.Allocate(16, 16, nextX, nextY))
                break;
        }
        REQUIRE(allocator.GetWidth() <= 64);
        REQUIRE(allocator.GetHeight() <= 64);
    }
}

TEST_CASE("AreaAllocator slow mode also packs without overlap", "[Math]")
{
    AreaAllocator allocator(64, 64, false);
    REQUIRE_FALSE(allocator.GetFastMode());

    int firstX = 0;
    int firstY = 0;
    REQUIRE(allocator.Allocate(32, 8, firstX, firstY));

    int secondX = 0;
    int secondY = 0;
    REQUIRE(allocator.Allocate(32, 8, secondX, secondY));

    const bool disjoint = firstX + 32 <= secondX || secondX + 32 <= firstX
        || firstY + 8 <= secondY || secondY + 8 <= firstY;
    REQUIRE(disjoint);
}

TEST_CASE("AreaAllocator default construction yields an empty area", "[Math]")
{
    AreaAllocator allocator;
    REQUIRE(allocator.GetWidth() == 0);
    REQUIRE(allocator.GetHeight() == 0);

    int x = 0;
    int y = 0;
    REQUIRE_FALSE(allocator.Allocate(1, 1, x, y));

    SECTION("it becomes usable after a Reset")
    {
        allocator.Reset(32, 32);
        REQUIRE(allocator.GetWidth() == 32);
        REQUIRE(allocator.Allocate(16, 16, x, y));
    }
}

TEST_CASE("AreaAllocator clamps a negative or zero request", "[Math]")
{
    AreaAllocator allocator(64, 64);

    int x = -1;
    int y = -1;
    REQUIRE(allocator.Allocate(0, 0, x, y));
    REQUIRE(x >= 0);
    REQUIRE(y >= 0);

    REQUIRE(allocator.Allocate(-5, -5, x, y));
    REQUIRE(x >= 0);
    REQUIRE(y >= 0);
}

TEST_CASE("AreaAllocator splits free areas on both axes", "[Math]")
{
    AreaAllocator allocator(128, 128, false);

    int x = 0;
    int y = 0;
    REQUIRE(allocator.Allocate(40, 30, x, y));
    REQUIRE(allocator.Allocate(20, 70, x, y));
    REQUIRE(allocator.Allocate(60, 10, x, y));
    REQUIRE(allocator.Allocate(10, 60, x, y));

    int count = 4;
    for (int i = 0; i < 40; ++i)
    {
        int nextX = 0;
        int nextY = 0;
        if (!allocator.Allocate(7, 5, nextX, nextY))
            break;
        REQUIRE(nextX + 7 <= 128);
        REQUIRE(nextY + 5 <= 128);
        ++count;
    }
    REQUIRE(count > 4);
}

TEST_CASE("AreaAllocator grows in both directions alternately", "[Math]")
{
    AreaAllocator allocator(8, 8, 256, 256);

    int lastWidth = allocator.GetWidth();
    int lastHeight = allocator.GetHeight();
    bool grewWidth = false;
    bool grewHeight = false;

    for (int i = 0; i < 64; ++i)
    {
        int x = 0;
        int y = 0;
        if (!allocator.Allocate(8, 8, x, y))
            break;

        if (allocator.GetWidth() > lastWidth)
            grewWidth = true;
        if (allocator.GetHeight() > lastHeight)
            grewHeight = true;

        lastWidth = allocator.GetWidth();
        lastHeight = allocator.GetHeight();

        REQUIRE(x + 8 <= allocator.GetWidth());
        REQUIRE(y + 8 <= allocator.GetHeight());
    }

    REQUIRE(grewWidth);
    REQUIRE(grewHeight);
    REQUIRE(allocator.GetWidth() <= 256);
    REQUIRE(allocator.GetHeight() <= 256);
}

TEST_CASE("AreaAllocator grows the single free area in place before anything is allocated", "[Math]")
{
    SECTION("widening")
    {
        AreaAllocator allocator(8, 8, 256, 256);
        int x = 0;
        int y = 0;
        REQUIRE(allocator.Allocate(16, 8, x, y));
        REQUIRE(allocator.GetWidth() >= 16);
        REQUIRE(x + 16 <= allocator.GetWidth());
        REQUIRE(y + 8 <= allocator.GetHeight());
    }

    SECTION("heightening")
    {
        AreaAllocator allocator(8, 8, 256, 256);
        int x = 0;
        int y = 0;
        REQUIRE(allocator.Allocate(8, 32, x, y));
        REQUIRE(allocator.GetHeight() >= 32);
        REQUIRE(y + 32 <= allocator.GetHeight());
    }

    SECTION("a request larger than the maximum is refused outright")
    {
        AreaAllocator allocator(8, 8, 16, 16);
        int x = 0;
        int y = 0;
        REQUIRE_FALSE(allocator.Allocate(64, 64, x, y));
    }
}

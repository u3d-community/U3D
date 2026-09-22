#include "TestUtils.h"
#include <Urho3D/Resource/XMLFile.h>
#include <Urho3D/Urho2D/TileMapDefs2D.h>

using namespace Urho3D;
using namespace U3DTest;

TEST_CASE("TileMapInfo2D converts orthogonal tile coordinates", "[Urho2D]")
{
    TileMapInfo2D info{O_ORTHOGONAL, 10, 5, 2.0f, 3.0f};
    REQUIRE_EQ_F(info.GetMapWidth(), 20.0f);
    REQUIRE_EQ_F(info.GetMapHeight(), 15.0f);
    for (int y = 0; y < info.height_; ++y)
    {
        for (int x = 0; x < info.width_; ++x)
        {
            const Vector2 position = info.TileIndexToPosition(x, y) + Vector2(0.5f, 0.5f);
            int restoredX = -1;
            int restoredY = -1;
            REQUIRE(info.PositionToTileIndex(restoredX, restoredY, position));
            REQUIRE(restoredX == x);
            REQUIRE(restoredY == y);
        }
    }
    int x = 0;
    int y = 0;
    REQUIRE_FALSE(info.PositionToTileIndex(x, y, Vector2(-1.0f, -1.0f)));
}

TEST_CASE("TileMapInfo2D reports orientation-specific dimensions", "[Urho2D]")
{
    TileMapInfo2D staggered{O_STAGGERED, 4, 6, 10.0f, 8.0f};
    REQUIRE_EQ_F(staggered.GetMapHeight(), 28.0f);
    REQUIRE(staggered.TileIndexToPosition(1, 1) == Vector2(15.0f, 16.0f));
    TileMapInfo2D hexagonal{O_HEXAGONAL, 4, 6, 10.0f, 8.0f};
    REQUIRE_EQ_F(hexagonal.GetMapHeight(), 36.0f);
    TileMapInfo2D isometric{O_ISOMETRIC, 4, 6, 10.0f, 8.0f};
    REQUIRE_EQ_F(isometric.GetMapWidth(), 40.0f);
    REQUIRE_EQ_F(isometric.GetMapHeight(), 48.0f);

    for (TileMapInfo2D* info : {&staggered, &hexagonal})
    {
        for (int tileY = 0; tileY < info->height_; ++tileY)
        {
            for (int tileX = 0; tileX < info->width_; ++tileX)
            {
                const Vector2 position = info->TileIndexToPosition(tileX, tileY);
                int restoredX = -1;
                int restoredY = -1;
                REQUIRE(info->PositionToTileIndex(restoredX, restoredY, position));
                REQUIRE(restoredX == tileX);
                REQUIRE(restoredY == tileY);
            }
        }
    }

    REQUIRE(staggered.ConvertPosition(Vector2(100.0f, 50.0f)).y_ < staggered.GetMapHeight());
    REQUIRE(hexagonal.ConvertPosition(Vector2(100.0f, 50.0f)).y_ < hexagonal.GetMapHeight());
}

TEST_CASE("PropertySet2D loads TMX properties", "[Urho2D]")
{
    TestContext context;
    XMLFile xml(context);
    REQUIRE(xml.FromString("<properties><property name='solid' value='true'/><property name='label'>hello</property></properties>"));
    SharedPtr<PropertySet2D> properties(new PropertySet2D());
    properties->Load(xml.GetRoot());
    REQUIRE(properties->HasProperty("solid"));
    REQUIRE(properties->GetProperty("solid") == "true");
    REQUIRE(properties->GetProperty("label") == "hello");
    REQUIRE_FALSE(properties->HasProperty("missing"));
    REQUIRE(properties->GetProperty("missing").Empty());
}

TEST_CASE("FrameSet2D advances and wraps animation frames", "[Urho2D]")
{
    TestContext context;
    XMLFile xml(context);
    REQUIRE(xml.FromString("<animation><frame tileid='3' duration='100'/><frame tileid='7' duration='200'/></animation>"));
    SharedPtr<FrameSet2D> frames(new FrameSet2D());
    frames->Load(xml.GetRoot());
    REQUIRE(frames->GetNumFrames() == 2);
    REQUIRE(frames->GetCurrentFrameGid() == 4);
    frames->UpdateTimer(0.15f);
    REQUIRE(frames->GetCurrentFrameGid() == 8);
    frames->UpdateTimer(0.20f);
    REQUIRE(frames->GetCurrentFrameGid() == 4);
    frames->UpdateTimer(1.0f);
    REQUIRE(frames->GetCurrentFrameGid() == 8);
}

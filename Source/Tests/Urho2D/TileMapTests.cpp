#include "TestUtils.h"

#include <Urho3D/Graphics/Texture2D.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Urho2D/Drawable2D.h>
#include <Urho3D/Urho2D/Renderer2D.h>
#include <Urho3D/Urho2D/TileMap2D.h>
#include <Urho3D/Urho2D/TileMapLayer2D.h>
#include <Urho3D/Urho2D/TmxFile2D.h>
#include <Urho3D/Urho2D/Urho2D.h>

using namespace Urho3D;
using namespace U3DTest;

namespace
{

TmxFile2D* LoadMap(const String& name)
{
    return HeadlessContext()->GetSubsystem<ResourceCache>()->GetResource<TmxFile2D>(name);
}

struct MapFixture
{
    explicit MapFixture(const String& name) :
        scene_(new Scene(HeadlessContext()))
    {
        scene_->CreateComponent<Renderer2D>();
        file_ = LoadMap(name);
        map_ = scene_->CreateChild("Map")->CreateComponent<TileMap2D>();
        map_->SetTmxFile(file_);
    }

    SharedPtr<Scene> scene_;
    TmxFile2D* file_{};
    TileMap2D* map_{};
};

}

TEST_CASE("TmxFile2D loads an orthogonal map", "[Urho2D]")
{
    TmxFile2D* file = LoadMap("Urho2D/Tilesets/Ortho.tmx");
    REQUIRE(file != nullptr);

    const TileMapInfo2D& info = file->GetInfo();
    REQUIRE(info.orientation_ == O_ORTHOGONAL);
    REQUIRE(info.width_ > 0);
    REQUIRE(info.height_ > 0);
    REQUIRE(info.tileWidth_ > 0.0f);
    REQUIRE(info.tileHeight_ > 0.0f);
    REQUIRE(info.GetMapWidth() > 0.0f);

    REQUIRE(file->GetNumLayers() > 0);
    REQUIRE(file->GetLayer(0) != nullptr);
    REQUIRE(file->GetLayer(file->GetNumLayers()) == nullptr);

    const TmxLayer2D* layer = file->GetLayer(0);
    REQUIRE(layer->GetTmxFile() == file);
    REQUIRE_FALSE(layer->GetName().Empty());
    REQUIRE(layer->GetWidth() == info.width_);
    REQUIRE(layer->GetHeight() == info.height_);
    REQUIRE(layer->IsVisible());
    REQUIRE(layer->GetProperty("NoSuchProperty").Empty());
    REQUIRE_FALSE(layer->HasProperty("NoSuchProperty"));

    SECTION("a tile layer hands out its tiles by index")
    {
        REQUIRE(layer->GetType() == LT_TILE_LAYER);
        const auto* tileLayer = static_cast<const TmxTileLayer2D*>(layer);

        bool anyTile = false;
        for (int y = 0; y < tileLayer->GetHeight() && !anyTile; ++y)
            for (int x = 0; x < tileLayer->GetWidth() && !anyTile; ++x)
                anyTile = tileLayer->GetTile(x, y) != nullptr;
        REQUIRE(anyTile);

        REQUIRE(tileLayer->GetTile(-1, 0) == nullptr);
        REQUIRE(tileLayer->GetTile(0, -1) == nullptr);
        REQUIRE(tileLayer->GetTile(tileLayer->GetWidth(), 0) == nullptr);
        REQUIRE(tileLayer->GetTile(0, tileLayer->GetHeight()) == nullptr);
    }

    SECTION("every tile in the map has a sprite and a gid")
    {
        for (unsigned i = 0; i < file->GetNumLayers(); ++i)
        {
            const TmxLayer2D* current = file->GetLayer(i);
            if (current->GetType() != LT_TILE_LAYER)
                continue;

            const auto* tileLayer = static_cast<const TmxTileLayer2D*>(current);
            for (int y = 0; y < tileLayer->GetHeight(); ++y)
            {
                for (int x = 0; x < tileLayer->GetWidth(); ++x)
                {
                    Tile2D* tile = tileLayer->GetTile(x, y);
                    if (!tile)
                        continue;
                    REQUIRE(tile->GetGid() > 0);
                    REQUIRE(tile->GetSprite() != nullptr);
                }
            }
        }
    }
}

TEST_CASE("TmxFile2D loads an isometric map with object groups", "[Urho2D]")
{
    TmxFile2D* file = LoadMap("Urho2D/isometric_grass_and_water.tmx");
    REQUIRE(file != nullptr);
    REQUIRE(file->GetInfo().orientation_ == O_ISOMETRIC);
    REQUIRE(file->GetNumLayers() > 0);

    bool sawTileLayer = false;
    for (unsigned i = 0; i < file->GetNumLayers(); ++i)
    {
        const TmxLayer2D* layer = file->GetLayer(i);
        sawTileLayer = sawTileLayer || layer->GetType() == LT_TILE_LAYER;
    }
    REQUIRE(sawTileLayer);
}

TEST_CASE("TmxFile2D reads objects out of an object group", "[Urho2D]")
{
    TmxFile2D* file = LoadMap("Urho2D/Tilesets/atrium.tmx");
    REQUIRE(file != nullptr);

    const TmxObjectGroup2D* group = nullptr;
    for (unsigned i = 0; i < file->GetNumLayers() && !group; ++i)
    {
        const TmxLayer2D* layer = file->GetLayer(i);
        if (layer->GetType() == LT_OBJECT_GROUP)
            group = static_cast<const TmxObjectGroup2D*>(layer);
    }

    REQUIRE(group != nullptr);
    REQUIRE(group->GetNumObjects() > 0);
    REQUIRE(group->GetObject(group->GetNumObjects()) == nullptr);

    for (unsigned i = 0; i < group->GetNumObjects(); ++i)
    {
        TileMapObject2D* object = group->GetObject(i);
        REQUIRE(object != nullptr);
        REQUIRE(object->GetObjectType() != OT_INVALID);

        if (object->GetObjectType() == OT_POLYGON || object->GetObjectType() == OT_POLYLINE)
        {
            REQUIRE(object->GetNumPoints() > 1);
            REQUIRE(object->GetPoint(object->GetNumPoints()) == Vector2::ZERO);
        }
    }
}

TEST_CASE("TmxFile2D can be built by hand", "[Urho2D]")
{
    TestContext context;
    SharedPtr<TmxFile2D> file(new TmxFile2D(context));

    REQUIRE(file->SetInfo(O_ORTHOGONAL, 8, 6, 16.0f, 16.0f));
    REQUIRE(file->GetInfo().width_ == 8);
    REQUIRE(file->GetInfo().height_ == 6);
    REQUIRE_EQ_F(file->GetInfo().tileWidth_, 16.0f * PIXEL_SIZE);

    auto* layer = new TmxTileLayer2D(file);
    file->AddLayer(layer);
    REQUIRE(file->GetNumLayers() == 1);
    REQUIRE(file->GetLayer(0) == layer);
    REQUIRE_FALSE(file->SetInfo(O_ISOMETRIC, 2, 2, 8.0f, 8.0f));

    SECTION("a layer can be inserted at a chosen index")
    {
        auto* second = new TmxTileLayer2D(file);
        file->AddLayer(0, second);
        REQUIRE(file->GetNumLayers() == 2);
        REQUIRE(file->GetLayer(0) == second);
        REQUIRE(file->GetLayer(1) == layer);
    }

    SECTION("an out of range insert appends instead")
    {
        auto* third = new TmxTileLayer2D(file);
        file->AddLayer(99, third);
        REQUIRE(file->GetNumLayers() == 2);
        REQUIRE(file->GetLayer(1) == third);
    }
}

TEST_CASE("TmxFile2D rejects a document that is not a tmx map", "[Urho2D]")
{
    Context* context = HeadlessContext();
    auto* cache = context->GetSubsystem<ResourceCache>();

    REQUIRE(cache->GetResource<TmxFile2D>("UI/DefaultStyle.xml", false) == nullptr);
    REQUIRE(cache->GetResource<TmxFile2D>("Urho2D/NoSuchMap.tmx", false) == nullptr);
}

TEST_CASE("TileMap2D builds a layer component per tmx layer", "[Urho2D]")
{
    MapFixture fixture("Urho2D/Tilesets/Ortho.tmx");
    REQUIRE(fixture.file_ != nullptr);
    REQUIRE(fixture.map_->GetTmxFile() == fixture.file_);
    REQUIRE(fixture.map_->GetNumLayers() == fixture.file_->GetNumLayers());
    REQUIRE(fixture.map_->GetLayer(0) != nullptr);
    REQUIRE(fixture.map_->GetLayer(fixture.map_->GetNumLayers()) == nullptr);

    const TileMapInfo2D& info = fixture.map_->GetInfo();
    REQUIRE(info.width_ == fixture.file_->GetInfo().width_);

    TileMapLayer2D* layer = fixture.map_->GetLayer(0);
    REQUIRE(layer->GetTileMap() == fixture.map_);
    REQUIRE(layer->GetTmxLayer() == fixture.file_->GetLayer(0));
    REQUIRE(layer->GetWidth() == info.width_);
    REQUIRE(layer->GetHeight() == info.height_);
    REQUIRE(layer->IsVisible());

    SECTION("tile index and position round trip through the map")
    {
        for (int y = 0; y < info.height_; ++y)
        {
            for (int x = 0; x < info.width_; ++x)
            {
                const Vector2 position = fixture.map_->TileIndexToPosition(x, y)
                    + Vector2(info.tileWidth_, info.tileHeight_) * 0.5f;
                int restoredX = -1;
                int restoredY = -1;
                REQUIRE(fixture.map_->PositionToTileIndex(restoredX, restoredY, position));
                REQUIRE(restoredX == x);
                REQUIRE(restoredY == y);
            }
        }
    }

    SECTION("a position outside the map is rejected")
    {
        int x = 0;
        int y = 0;
        REQUIRE_FALSE(fixture.map_->PositionToTileIndex(x, y, Vector2(-100.0f, -100.0f)));
        REQUIRE_FALSE(fixture.map_->PositionToTileIndex(x, y, Vector2(10000.0f, 10000.0f)));
    }

    SECTION("a layer exposes the node and drawable for each tile it drew")
    {
        bool anyNode = false;
        for (int y = 0; y < layer->GetHeight() && !anyNode; ++y)
        {
            for (int x = 0; x < layer->GetWidth() && !anyNode; ++x)
            {
                if (layer->GetTileNode(x, y))
                {
                    anyNode = true;
                    REQUIRE(layer->GetTile(x, y) != nullptr);
                }
            }
        }
        REQUIRE(anyNode);

        REQUIRE(layer->GetTileNode(-1, 0) == nullptr);
        REQUIRE(layer->GetTile(layer->GetWidth(), 0) == nullptr);
    }

    SECTION("hiding a layer hides its tile nodes")
    {
        layer->SetVisible(false);
        REQUIRE_FALSE(layer->IsVisible());
        layer->SetVisible(true);
        REQUIRE(layer->IsVisible());
    }

    SECTION("the draw order is settable")
    {
        layer->SetDrawOrder(7);
        REQUIRE(layer->GetDrawOrder() == 7);
    }

    SECTION("replacing the tmx file rebuilds the layers")
    {
        fixture.map_->SetTmxFile(nullptr);
        REQUIRE(fixture.map_->GetTmxFile() == nullptr);
        REQUIRE(fixture.map_->GetNumLayers() == 0);

        fixture.map_->SetTmxFile(fixture.file_);
        REQUIRE(fixture.map_->GetNumLayers() == fixture.file_->GetNumLayers());
    }

    SECTION("the tmx file round trips through its resource ref")
    {
        const ResourceRef reference = fixture.map_->GetTmxFileAttr();
        REQUIRE(reference.type_ == TmxFile2D::GetTypeStatic());
        REQUIRE_FALSE(reference.name_.Empty());

        fixture.map_->SetTmxFile(nullptr);
        fixture.map_->SetTmxFileAttr(reference);
        REQUIRE(fixture.map_->GetTmxFile() == fixture.file_);
    }
}

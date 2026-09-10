#include "TestUtils.h"
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Urho2D/Sprite2D.h>

using namespace Urho3D;
using namespace U3DTest;

TEST_CASE("Sprite2D stores geometry settings without a texture", "[Urho2D]")
{
    SharedPtr<Sprite2D> sprite(new Sprite2D(HeadlessContext()));
    sprite->SetRectangle(IntRect(10, 20, 30, 50));
    sprite->SetHotSpot(Vector2(0.25f, 0.75f));
    sprite->SetOffset(IntVector2(2, 3));
    sprite->SetTextureEdgeOffset(0.5f);
    REQUIRE(sprite->GetRectangle() == IntRect(10, 20, 30, 50));
    REQUIRE(sprite->GetHotSpot() == Vector2(0.25f, 0.75f));
    REQUIRE(sprite->GetOffset() == IntVector2(2, 3));
    REQUIRE_EQ_F(sprite->GetTextureEdgeOffset(), 0.5f);
    Rect rect;
    REQUIRE(sprite->GetDrawRectangle(rect));
    REQUIRE(rect.Size().x_ > 0.0f);
    REQUIRE_FALSE(sprite->GetTextureRectangle(rect));
    REQUIRE(Sprite2D::SaveToResourceRef(nullptr).name_.Empty());
}

TEST_CASE("Sprite2D resource references resolve their declared type", "[Urho2D]")
{
    Context* context = HeadlessContext();
    ResourceCache* cache = context->GetSubsystem<ResourceCache>();
    SharedPtr<Sprite2D> sprite(new Sprite2D(context));
    sprite->SetName("Generated/MutationSprite.png");
    REQUIRE(cache->AddManualResource(sprite));

    const ResourceRef reference(Sprite2D::GetTypeStatic(), sprite->GetName());
    REQUIRE(Sprite2D::LoadFromResourceRef(sprite, reference) == sprite);
    cache->ReleaseResource<Sprite2D>(sprite->GetName(), true);
}

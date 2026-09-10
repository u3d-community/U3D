#include "TestUtils.h"

#include <Urho3D/Graphics/Texture2D.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Urho2D/AnimatedSprite2D.h>
#include <Urho3D/Urho2D/AnimationSet2D.h>
#include <Urho3D/Urho2D/ParticleEffect2D.h>
#include <Urho3D/Urho2D/ParticleEmitter2D.h>
#include <Urho3D/Urho2D/Renderer2D.h>
#include <Urho3D/Urho2D/Sprite2D.h>
#include <Urho3D/Urho2D/SpriteSheet2D.h>
#include <Urho3D/Urho2D/StaticSprite2D.h>
#include <Urho3D/Urho2D/StretchableSprite2D.h>

using namespace Urho3D;
using namespace U3DTest;

namespace
{

template <class T> T* LoadAsset(const String& name)
{
    return HeadlessContext()->GetSubsystem<ResourceCache>()->GetResource<T>(name);
}

unsigned LiveParticles(ParticleEmitter2D* emitter)
{
    unsigned vertices = 0;
    const Vector<SourceBatch2D>& batches = emitter->GetSourceBatches();
    for (unsigned i = 0; i < batches.Size(); ++i)
        vertices += batches[i].vertices_.Size();
    return vertices / 4;
}

SharedPtr<Sprite2D> MakeSprite(Context* context, const IntVector2& size)
{
    SharedPtr<Texture2D> texture(new Texture2D(context));
    SharedPtr<Sprite2D> sprite(new Sprite2D(context));
    sprite->SetTexture(texture);
    sprite->SetRectangle(IntRect(0, 0, size.x_, size.y_));
    return sprite;
}

}

TEST_CASE("Sprite2D derives its draw and texture rectangles", "[Urho2D]")
{
    Context* context = HeadlessContext();
    SharedPtr<Sprite2D> sprite = MakeSprite(context, IntVector2(64, 32));
    sprite->SetHotSpot(Vector2(0.5f, 0.5f));

    REQUIRE(sprite->GetTexture() != nullptr);
    REQUIRE(sprite->GetRectangle() == IntRect(0, 0, 64, 32));

    Rect draw;
    REQUIRE(sprite->GetDrawRectangle(draw));
    REQUIRE_NEAR(draw.Size().x_, 0.64f, 0.001f);
    REQUIRE_NEAR(draw.Size().y_, 0.32f, 0.001f);
    REQUIRE_NEAR(draw.Center().x_, 0.0f, 0.001f);
    REQUIRE_NEAR(draw.Center().y_, 0.0f, 0.001f);

    SECTION("a hot spot in the corner puts the quad entirely on one side")
    {
        Rect corner;
        REQUIRE(sprite->GetDrawRectangle(corner, Vector2::ZERO, false, false));
        REQUIRE_NEAR(corner.min_.x_, 0.0f, 0.001f);
        REQUIRE_NEAR(corner.min_.y_, 0.0f, 0.001f);
        REQUIRE_NEAR(corner.max_.x_, 0.64f, 0.001f);
    }

    SECTION("flipping mirrors the quad about the hot spot")
    {
        Rect flipped;
        REQUIRE(sprite->GetDrawRectangle(flipped, Vector2::ZERO, true, true));
        REQUIRE_NEAR(flipped.Size().x_, 0.64f, 0.001f);
        REQUIRE_NEAR(flipped.max_.x_, 0.0f, 0.001f);
        REQUIRE_NEAR(flipped.max_.y_, 0.0f, 0.001f);
    }

    SECTION("a zero sized rectangle has no quad to draw")
    {
        SharedPtr<Sprite2D> empty(new Sprite2D(context));
        Rect none;
        REQUIRE_FALSE(empty->GetDrawRectangle(none));
        REQUIRE_FALSE(empty->GetTextureRectangle(none));
    }

    SECTION("the sprite remembers a manual offset")
    {
        sprite->SetOffset(IntVector2(4, 8));
        REQUIRE(sprite->GetOffset() == IntVector2(4, 8));
    }
}

TEST_CASE("SpriteSheet2D indexes its sprites by name", "[Urho2D]")
{
    auto* sheet = LoadAsset<SpriteSheet2D>("Urho2D/Orc/Orc.xml");
    REQUIRE(sheet != nullptr);
    REQUIRE_FALSE(sheet->GetSpriteMapping().Empty());

    const String first = sheet->GetSpriteMapping().Begin()->first_;
    Sprite2D* sprite = sheet->GetSprite(first);
    REQUIRE(sprite != nullptr);
    REQUIRE(sprite->GetSpriteSheet() == sheet);
    REQUIRE(sprite->GetRectangle().Width() > 0);

    REQUIRE(sheet->GetSprite("NoSuchSprite") == nullptr);

    SECTION("every sprite in the sheet shares the sheet texture")
    {
        for (auto i = sheet->GetSpriteMapping().Begin(); i != sheet->GetSpriteMapping().End(); ++i)
            REQUIRE(i->second_->GetTexture() == sheet->GetTexture());
    }

    SECTION("a sprite resolves back through a resource ref")
    {
        const ResourceRef reference = Sprite2D::SaveToResourceRef(sprite);
        REQUIRE(reference.type_ == SpriteSheet2D::GetTypeStatic());
        REQUIRE(Sprite2D::LoadFromResourceRef(sheet, reference) != nullptr);
    }
}

TEST_CASE("StaticSprite2D exposes its drawing state", "[Urho2D]")
{
    Context* context = HeadlessContext();
    SharedPtr<Scene> scene(new Scene(context));
    scene->CreateComponent<Renderer2D>();

    Node* node = scene->CreateChild("Sprite");
    auto* drawable = node->CreateComponent<StaticSprite2D>();
    SharedPtr<Sprite2D> sprite = MakeSprite(context, IntVector2(100, 100));
    drawable->SetSprite(sprite);

    REQUIRE(drawable->GetSprite() == sprite);
    REQUIRE(drawable->GetWorldBoundingBox().Defined());

    drawable->SetColor(Color::RED);
    REQUIRE(drawable->GetColor() == Color::RED);
    drawable->SetAlpha(0.25f);
    REQUIRE_NEAR(drawable->GetAlpha(), 0.25f, 0.001f);
    REQUIRE_NEAR(drawable->GetColor().a_, 0.25f, 0.001f);

    drawable->SetBlendMode(BLEND_ADDALPHA);
    REQUIRE(drawable->GetBlendMode() == BLEND_ADDALPHA);

    drawable->SetFlip(true, true, true);
    REQUIRE(drawable->GetFlipX());
    REQUIRE(drawable->GetFlipY());
    REQUIRE(drawable->GetSwapXY());
    drawable->SetFlipX(false);
    REQUIRE_FALSE(drawable->GetFlipX());

    drawable->SetLayer(5);
    drawable->SetOrderInLayer(2);
    REQUIRE(drawable->GetLayer() == 5);
    REQUIRE(drawable->GetOrderInLayer() == 2);

    SECTION("an explicit draw rectangle overrides the one from the sprite")
    {
        const Rect fromSprite = drawable->GetDrawRect();
        drawable->SetUseDrawRect(true);
        drawable->SetDrawRect(Rect(-2.0f, -2.0f, 2.0f, 2.0f));
        REQUIRE(drawable->GetUseDrawRect());
        REQUIRE(drawable->GetDrawRect() == Rect(-2.0f, -2.0f, 2.0f, 2.0f));
        REQUIRE_FALSE(drawable->GetDrawRect() == fromSprite);
    }

    SECTION("a custom hot spot moves the quad off the sprite's own centre")
    {
        drawable->SetFlip(false, false, false);
        const Rect centred = drawable->GetDrawRect();
        REQUIRE_NEAR(centred.Center().x_, 0.0f, 0.001f);

        drawable->SetUseHotSpot(true);
        drawable->SetHotSpot(Vector2::ZERO);
        REQUIRE(drawable->GetUseHotSpot());
        REQUIRE(drawable->GetHotSpot() == Vector2::ZERO);
        REQUIRE_NEAR(drawable->GetDrawRect().min_.x_, 0.0f, 0.001f);
        REQUIRE_NEAR(drawable->GetDrawRect().min_.y_, 0.0f, 0.001f);
    }

    SECTION("the bounding box follows the node")
    {
        node->SetPosition(Vector3(10.0f, 0.0f, 0.0f));
        REQUIRE_NEAR(drawable->GetWorldBoundingBox().Center().x_, 10.0f, 0.001f);
    }

    SECTION("clearing the sprite empties the geometry")
    {
        drawable->SetSprite(nullptr);
        REQUIRE(drawable->GetSprite() == nullptr);
    }
}

TEST_CASE("StretchableSprite2D keeps its borders unscaled", "[Urho2D]")
{
    Context* context = HeadlessContext();
    SharedPtr<Scene> scene(new Scene(context));
    scene->CreateComponent<Renderer2D>();

    auto* drawable = scene->CreateChild("Panel")->CreateComponent<StretchableSprite2D>();
    drawable->SetSprite(MakeSprite(context, IntVector2(64, 64)));

    drawable->SetBorder(IntRect(4, 5, 6, 7));
    REQUIRE(drawable->GetBorder() == IntRect(4, 5, 6, 7));

    SECTION("an out of bounds border is stored as given and only clamped when it is used")
    {
        drawable->SetBorder(IntRect(-1, -1, 10000, 10000));
        REQUIRE(drawable->GetBorder() == IntRect(-1, -1, 10000, 10000));
    }
}

TEST_CASE("ParticleEffect2D loads a pex file and clones cleanly", "[Urho2D]")
{
    auto* effect = LoadAsset<ParticleEffect2D>("Urho2D/sun.pex");
    REQUIRE(effect != nullptr);

    REQUIRE(effect->GetMaxParticles() > 0);
    REQUIRE(effect->GetParticleLifeSpan() > 0.0f);
    REQUIRE(effect->GetDuration() != 0.0f);
    REQUIRE(effect->GetSprite() != nullptr);

    SharedPtr<ParticleEffect2D> clone = effect->Clone("ClonedEffect");
    REQUIRE(clone.NotNull());
    REQUIRE(clone->GetMaxParticles() == effect->GetMaxParticles());

    clone->SetMaxParticles(7);
    clone->SetParticleLifeSpan(2.5f);
    clone->SetEmitterType(EMITTER_TYPE_RADIAL);
    clone->SetStartColor(Color::RED);
    clone->SetFinishColor(Color::BLUE);
    clone->SetGravity(Vector2(0.0f, -5.0f));

    REQUIRE(clone->GetMaxParticles() == 7);
    REQUIRE_EQ_F(clone->GetParticleLifeSpan(), 2.5f);
    REQUIRE(clone->GetEmitterType() == EMITTER_TYPE_RADIAL);
    REQUIRE(clone->GetStartColor() == Color::RED);
    REQUIRE(clone->GetFinishColor() == Color::BLUE);
    REQUIRE(clone->GetGravity() == Vector2(0.0f, -5.0f));

    REQUIRE(effect->GetMaxParticles() != 7);
}

TEST_CASE("ParticleEmitter2D emits and retires particles over time", "[Urho2D]")
{
    Context* context = HeadlessContext();
    SharedPtr<Scene> scene(new Scene(context));
    scene->CreateComponent<Renderer2D>();

    auto* effect = LoadAsset<ParticleEffect2D>("Urho2D/sun.pex");
    REQUIRE(effect != nullptr);

    Node* node = scene->CreateChild("Emitter");
    auto* emitter = node->CreateComponent<ParticleEmitter2D>();
    emitter->SetEffect(effect);

    REQUIRE(emitter->GetEffect() == effect);
    REQUIRE(emitter->GetSprite() != nullptr);
    REQUIRE(emitter->IsEmitting());

    scene->Update(0.1f);
    REQUIRE(LiveParticles(emitter) > 0);

    SECTION("stopping the emitter lets the existing particles die out")
    {
        emitter->SetEmitting(false);
        REQUIRE_FALSE(emitter->IsEmitting());

        for (int i = 0; i < 200; ++i)
            scene->Update(0.1f);

        REQUIRE(LiveParticles(emitter) == 0);
    }

    SECTION("restarting the emitter refills the buffer")
    {
        emitter->SetEmitting(false);
        emitter->SetEmitting(true);
        scene->Update(0.1f);
        REQUIRE(LiveParticles(emitter) > 0);
    }

    SECTION("the particle count never exceeds the effect's maximum")
    {
        for (int i = 0; i < 100; ++i)
            scene->Update(0.05f);
        REQUIRE(LiveParticles(emitter) <= (unsigned)effect->GetMaxParticles());
    }
}

TEST_CASE("AnimationSet2D loads a spriter file", "[Urho2D]")
{
    auto* set = LoadAsset<AnimationSet2D>("Urho2D/imp/imp.scml");
    REQUIRE(set != nullptr);
    REQUIRE(set->GetNumAnimations() > 0);

    const String first = set->GetAnimation(0);
    REQUIRE_FALSE(first.Empty());
    REQUIRE(set->HasAnimation(first));
    REQUIRE_FALSE(set->HasAnimation("NoSuchAnimation"));
    REQUIRE(set->GetAnimation(set->GetNumAnimations()).Empty());
}

TEST_CASE("AnimatedSprite2D plays a spriter animation", "[Urho2D]")
{
    Context* context = HeadlessContext();
    SharedPtr<Scene> scene(new Scene(context));
    scene->CreateComponent<Renderer2D>();

    auto* set = LoadAsset<AnimationSet2D>("Urho2D/imp/imp.scml");
    REQUIRE(set != nullptr);

    Node* node = scene->CreateChild("Imp");
    auto* animated = node->CreateComponent<AnimatedSprite2D>();
    animated->SetAnimationSet(set);

    const String animation = set->GetAnimation(0);
    animated->SetAnimation(animation, LM_FORCE_LOOPED);

    REQUIRE(animated->GetAnimationSet() == set);
    REQUIRE(animated->GetAnimation() == animation);
    REQUIRE(animated->GetLoopMode() == LM_FORCE_LOOPED);

    animated->SetSpeed(2.0f);
    REQUIRE_EQ_F(animated->GetSpeed(), 2.0f);

    scene->Update(0.1f);
    REQUIRE_FALSE(animated->GetSourceBatches().Empty());
    REQUIRE_FALSE(animated->GetSourceBatches()[0].vertices_.Empty());
    REQUIRE(animated->GetWorldBoundingBox().Defined());

    SECTION("switching animation replaces the previous one")
    {
        REQUIRE(set->GetNumAnimations() > 1);

        const String second = set->GetAnimation(1);
        animated->SetAnimation(second);
        REQUIRE(animated->GetAnimation() == second);
    }

    SECTION("clearing the animation set stops playback but leaves the last frame drawn")
    {
        animated->SetAnimationSet(nullptr);
        REQUIRE(animated->GetAnimationSet() == nullptr);

        scene->Update(0.1f);
        const Vector<Vertex2D> frozen = animated->GetSourceBatches()[0].vertices_;
        REQUIRE_FALSE(frozen.Empty());

        scene->Update(0.5f);
        const Vector<Vertex2D>& after = animated->GetSourceBatches()[0].vertices_;
        REQUIRE(after.Size() == frozen.Size());
        for (unsigned i = 0; i < frozen.Size(); ++i)
            REQUIRE(after[i].position_ == frozen[i].position_);
    }
}

TEST_CASE("AnimatedSprite2D forgets its animation when the set is cleared", "[Urho2D]")
{
    Context* context = HeadlessContext();
    SharedPtr<Scene> scene(new Scene(context));
    scene->CreateComponent<Renderer2D>();

    auto* set = LoadAsset<AnimationSet2D>("Urho2D/imp/imp.scml");
    REQUIRE(set != nullptr);

    auto* animated = scene->CreateChild("Imp")->CreateComponent<AnimatedSprite2D>();
    animated->SetAnimationSet(set);
    animated->SetAnimation(set->GetAnimation(0), LM_FORCE_LOOPED);

    REQUIRE_FALSE(animated->GetAnimation().Empty());
    REQUIRE(animated->GetLoopMode() == LM_FORCE_LOOPED);

    animated->SetAnimationSet(nullptr);

    REQUIRE(animated->GetAnimationSet() == nullptr);
    REQUIRE(animated->GetAnimation().Empty());
    REQUIRE(animated->GetLoopMode() == LM_DEFAULT);

    SECTION("an unresolvable resource ref clears it through the attribute path too")
    {
        animated->SetAnimationSet(set);
        animated->SetAnimation(set->GetAnimation(0), LM_FORCE_LOOPED);
        REQUIRE_FALSE(animated->GetAnimation().Empty());

        animated->SetAnimationSetAttr(ResourceRef(AnimationSet2D::GetTypeStatic(), "Urho2D/NoSuchSet.scml"));

        REQUIRE(animated->GetAnimationSet() == nullptr);
        REQUIRE(animated->GetAnimation().Empty());
        REQUIRE(animated->GetLoopMode() == LM_DEFAULT);
    }
}

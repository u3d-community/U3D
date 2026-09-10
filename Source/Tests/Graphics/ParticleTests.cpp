#include "TestUtils.h"

#include <Urho3D/Graphics/BillboardSet.h>
#include <Urho3D/Graphics/Drawable.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Graphics/ParticleEffect.h>
#include <Urho3D/Graphics/ParticleEmitter.h>
#include <Urho3D/IO/VectorBuffer.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Resource/XMLFile.h>
#include <Urho3D/Scene/Scene.h>

using namespace Urho3D;
using namespace U3DTest;

namespace
{

unsigned LiveParticles(ParticleEmitter* emitter)
{
    unsigned count = 0;
    const PODVector<Billboard>& billboards = emitter->GetBillboards();
    for (unsigned i = 0; i < billboards.Size(); ++i)
        count += billboards[i].enabled_ ? 1 : 0;
    return count;
}

ParticleEffect* LoadEffect(const String& name)
{
    return HeadlessContext()->GetSubsystem<ResourceCache>()->GetResource<ParticleEffect>(name);
}

struct EmitterFixture
{
    explicit EmitterFixture(ParticleEffect* effect) :
        scene_(new Scene(HeadlessContext()))
    {
        scene_->CreateComponent<Octree>();
        node_ = scene_->CreateChild("Emitter");
        emitter_ = node_->CreateComponent<ParticleEmitter>();
        emitter_->SetViewMask(0);
        emitter_->SetEffect(effect);
    }

    void Step(int frames, float timeStep = 0.05f)
    {
        FrameInfo frame{};
        frame.timeStep_ = timeStep;
        for (int i = 0; i < frames; ++i)
        {
            scene_->Update(timeStep);
            emitter_->Update(frame);
        }
    }

    SharedPtr<Scene> scene_;
    Node* node_{};
    ParticleEmitter* emitter_{};
};

}

TEST_CASE("ParticleEffect loads from XML and reports its ranges", "[Graphics]")
{
    ParticleEffect* effect = LoadEffect("Particle/Fire.xml");
    REQUIRE(effect != nullptr);

    REQUIRE(effect->GetNumParticles() > 0);
    REQUIRE(effect->GetMinTimeToLive() > 0.0f);
    REQUIRE(effect->GetMaxTimeToLive() >= effect->GetMinTimeToLive());
    REQUIRE(effect->GetMaxEmissionRate() >= effect->GetMinEmissionRate());
    REQUIRE(effect->GetMaxParticleSize().x_ >= effect->GetMinParticleSize().x_);
    REQUIRE(effect->GetMaterial() != nullptr);

    SECTION("a colour ramp is ordered by time")
    {
        const Vector<ColorFrame>& colors = effect->GetColorFrames();
        for (unsigned i = 1; i < colors.Size(); ++i)
            REQUIRE(colors[i].time_ >= colors[i - 1].time_);

        if (!colors.Empty())
        {
            REQUIRE(effect->GetColorFrame(0) != nullptr);
            REQUIRE(effect->GetColorFrame(colors.Size()) == nullptr);
        }
    }

    SECTION("a texture animation ramp is ordered by time")
    {
        const Vector<TextureFrame>& frames = effect->GetTextureFrames();
        REQUIRE(effect->GetNumTextureFrames() == frames.Size());

        for (unsigned i = 1; i < frames.Size(); ++i)
            REQUIRE(frames[i].time_ >= frames[i - 1].time_);
    }
}

TEST_CASE("ParticleEffect clamps and round trips its settings", "[Graphics]")
{
    TestContext context;
    SharedPtr<ParticleEffect> effect(new ParticleEffect(context));

    effect->SetNumParticles(0);
    REQUIRE(effect->GetNumParticles() == 0);
    effect->SetNumParticles(50);
    REQUIRE(effect->GetNumParticles() == 50);

    effect->SetMinTimeToLive(-1.0f);
    REQUIRE(effect->GetMinTimeToLive() >= 0.0f);
    effect->SetMaxTimeToLive(-1.0f);
    REQUIRE(effect->GetMaxTimeToLive() >= 0.0f);

    effect->SetMinTimeToLive(2.0f);
    effect->SetMaxTimeToLive(2.0f);
    REQUIRE_EQ_F(effect->GetMinTimeToLive(), 2.0f);
    REQUIRE_EQ_F(effect->GetMaxTimeToLive(), 2.0f);

    effect->SetMinEmissionRate(-5.0f);
    REQUIRE(effect->GetMinEmissionRate() >= 0.0f);
    effect->SetMinEmissionRate(10.0f);
    effect->SetMaxEmissionRate(10.0f);
    REQUIRE_EQ_F(effect->GetMinEmissionRate(), 10.0f);
    REQUIRE_EQ_F(effect->GetMaxEmissionRate(), 10.0f);

    effect->SetMinParticleSize(Vector2(2.0f, 3.0f));
    effect->SetMaxParticleSize(Vector2(2.0f, 3.0f));
    REQUIRE(effect->GetMinParticleSize() == Vector2(2.0f, 3.0f));
    REQUIRE(effect->GetMaxParticleSize() == Vector2(2.0f, 3.0f));

    effect->SetMinDirection(Vector3(-1.0f, 0.0f, -1.0f));
    effect->SetMaxDirection(Vector3(1.0f, 1.0f, 1.0f));
    REQUIRE(effect->GetMinDirection() == Vector3(-1.0f, 0.0f, -1.0f));
    REQUIRE(effect->GetMaxDirection() == Vector3(1.0f, 1.0f, 1.0f));

    effect->SetMinVelocity(5.0f);
    effect->SetMaxVelocity(5.0f);
    REQUIRE_EQ_F(effect->GetMinVelocity(), 5.0f);
    REQUIRE_EQ_F(effect->GetMaxVelocity(), 5.0f);

    effect->SetMinRotation(45.0f);
    effect->SetMinRotationSpeed(90.0f);
    REQUIRE_EQ_F(effect->GetMinRotation(), 45.0f);
    REQUIRE_EQ_F(effect->GetMinRotationSpeed(), 90.0f);

    effect->SetEmitterType(EMITTER_SPHERE);
    effect->SetEmitterSize(Vector3::ONE);
    REQUIRE(effect->GetEmitterType() == EMITTER_SPHERE);
    REQUIRE(effect->GetEmitterSize() == Vector3::ONE);

    effect->SetConstantForce(Vector3(0.0f, -9.8f, 0.0f));
    effect->SetDampingForce(0.5f);
    effect->SetSizeAdd(0.1f);
    effect->SetSizeMul(1.5f);
    REQUIRE(effect->GetConstantForce() == Vector3(0.0f, -9.8f, 0.0f));
    REQUIRE_EQ_F(effect->GetDampingForce(), 0.5f);
    REQUIRE_EQ_F(effect->GetSizeAdd(), 0.1f);
    REQUIRE_EQ_F(effect->GetSizeMul(), 1.5f);

    effect->SetActiveTime(-1.0f);
    REQUIRE_EQ_F(effect->GetActiveTime(), -1.0f);
    effect->SetActiveTime(3.0f);
    REQUIRE_EQ_F(effect->GetActiveTime(), 3.0f);
    effect->SetInactiveTime(1.5f);
    REQUIRE_EQ_F(effect->GetInactiveTime(), 1.5f);

    effect->SetRelative(false);
    effect->SetScaled(false);
    effect->SetSorted(true);
    effect->SetUpdateInvisible(true);
    effect->SetFixedScreenSize(true);
    REQUIRE_FALSE(effect->IsRelative());
    REQUIRE_FALSE(effect->IsScaled());
    REQUIRE(effect->IsSorted());
    REQUIRE(effect->GetUpdateInvisible());
    REQUIRE(effect->IsFixedScreenSize());

    effect->SetFaceCameraMode(FC_ROTATE_Y);
    REQUIRE(effect->GetFaceCameraMode() == FC_ROTATE_Y);

    SECTION("the two ends of a range are independent, so a caller can invert one")
    {
        effect->SetMinTimeToLive(5.0f);
        effect->SetMaxTimeToLive(1.0f);
        REQUIRE_EQ_F(effect->GetMinTimeToLive(), 5.0f);
        REQUIRE_EQ_F(effect->GetMaxTimeToLive(), 1.0f);
    }

    SECTION("colour frames can be added, edited, and sorted")
    {
        effect->SetNumColorFrames(3);
        REQUIRE(effect->GetNumColorFrames() == 3);

        effect->SetColorFrame(0, ColorFrame(Color::RED, 0.0f));
        effect->SetColorFrame(1, ColorFrame(Color::BLUE, 2.0f));
        effect->SetColorFrame(2, ColorFrame(Color::GREEN, 1.0f));

        REQUIRE(effect->GetColorFrame(1)->color_ == Color::BLUE);

        effect->SortColorFrames();
        REQUIRE_EQ_F(effect->GetColorFrame(0)->time_, 0.0f);
        REQUIRE_EQ_F(effect->GetColorFrame(1)->time_, 1.0f);
        REQUIRE_EQ_F(effect->GetColorFrame(2)->time_, 2.0f);

        effect->RemoveColorFrame(1);
        REQUIRE(effect->GetNumColorFrames() == 2);

        effect->AddColorTime(Color::WHITE, 5.0f);
        REQUIRE(effect->GetNumColorFrames() == 3);

        effect->SetNumColorFrames(1);
        effect->SetColorFrame(0, ColorFrame(Color::YELLOW, 0.0f));
        REQUIRE(effect->GetNumColorFrames() == 1);
        REQUIRE(effect->GetColorFrame(0)->color_ == Color::YELLOW);
    }

    SECTION("texture frames can be added, edited, and sorted")
    {
        effect->SetNumTextureFrames(2);
        REQUIRE(effect->GetNumTextureFrames() == 2);

        effect->SetTextureFrame(0, TextureFrame());
        effect->SetTextureFrame(1, TextureFrame());
        REQUIRE(effect->GetTextureFrame(0) != nullptr);
        REQUIRE(effect->GetTextureFrame(2) == nullptr);

        effect->SortTextureFrames();
        effect->RemoveTextureFrame(0);
        REQUIRE(effect->GetNumTextureFrames() == 1);
    }
}

TEST_CASE("ParticleEffect saves and reloads through XML", "[Graphics]")
{
    Context* context = HeadlessContext();
    ParticleEffect* source = LoadEffect("Particle/Fire.xml");
    REQUIRE(source != nullptr);

    SharedPtr<XMLFile> document(new XMLFile(context));
    XMLElement root = document->CreateRoot("particleeffect");
    REQUIRE(source->Save(root));

    SharedPtr<ParticleEffect> restored(new ParticleEffect(context));
    REQUIRE(restored->Load(root));

    REQUIRE(restored->GetNumParticles() == source->GetNumParticles());
    REQUIRE_EQ_F(restored->GetMinTimeToLive(), source->GetMinTimeToLive());
    REQUIRE(restored->GetEmitterType() == source->GetEmitterType());
    REQUIRE(restored->GetNumColorFrames() == source->GetNumColorFrames());

    SECTION("a clone matches the original without sharing it")
    {
        SharedPtr<ParticleEffect> clone = source->Clone("ClonedEffect");
        REQUIRE(clone.NotNull());
        REQUIRE(clone->GetNumParticles() == source->GetNumParticles());

        clone->SetNumParticles(3);
        REQUIRE(source->GetNumParticles() != 3);
    }

    SECTION("a document with no recognised children loads as a default effect")
    {
        SharedPtr<XMLFile> wrong(new XMLFile(context));
        XMLElement wrongRoot = wrong->CreateRoot("something-else");

        SharedPtr<ParticleEffect> bare(new ParticleEffect(context));
        REQUIRE(bare->Load(wrongRoot));
        REQUIRE(bare->GetEmitterType() == EMITTER_SPHERE);
        REQUIRE(bare->GetMaterial() == nullptr);
        REQUIRE(bare->IsRelative());
    }
}

TEST_CASE("ParticleEmitter spawns and retires particles", "[Graphics]")
{
    ParticleEffect* effect = LoadEffect("Particle/Fire.xml");
    REQUIRE(effect != nullptr);

    SharedPtr<ParticleEffect> owned = effect->Clone("EmitterTestEffect");
    owned->SetNumParticles(32);
    owned->SetMinEmissionRate(50.0f);
    owned->SetMaxEmissionRate(50.0f);
    owned->SetMinTimeToLive(0.5f);
    owned->SetMaxTimeToLive(0.5f);
    owned->SetActiveTime(0.0f);
    owned->SetInactiveTime(0.0f);
    owned->SetUpdateInvisible(true);

    EmitterFixture fixture(owned);
    REQUIRE(fixture.emitter_->GetEffect() == owned);
    REQUIRE(fixture.emitter_->GetNumParticles() == 32);
    REQUIRE(fixture.emitter_->IsEmitting());
    REQUIRE(LiveParticles(fixture.emitter_) == 0);

    fixture.Step(4);
    REQUIRE(LiveParticles(fixture.emitter_) > 0);
    REQUIRE(fixture.emitter_->GetWorldBoundingBox().Defined());

    SECTION("particles are retired once they outlive their time to live")
    {
        fixture.emitter_->SetEmitting(false);
        REQUIRE_FALSE(fixture.emitter_->IsEmitting());

        fixture.Step(40);
        REQUIRE(LiveParticles(fixture.emitter_) == 0);
    }

    SECTION("the active count never exceeds the effect's budget")
    {
        fixture.Step(100);
        REQUIRE(LiveParticles(fixture.emitter_) <= 32);
    }

    SECTION("resetting the emitter clears the live particles")
    {
        fixture.emitter_->RemoveAllParticles();
        REQUIRE(LiveParticles(fixture.emitter_) == 0);
    }

    SECTION("a smaller budget shrinks the particle pool")
    {
        fixture.emitter_->SetNumParticles(8);
        REQUIRE(fixture.emitter_->GetNumParticles() == 8);
        fixture.Step(20);
        REQUIRE(LiveParticles(fixture.emitter_) <= 8);
    }

    SECTION("the emitter can be reset to its serialised state")
    {
        fixture.emitter_->Reset();
        REQUIRE(fixture.emitter_->IsEmitting());
    }

    SECTION("clearing the effect stops emission")
    {
        fixture.emitter_->SetEffect(nullptr);
        REQUIRE(fixture.emitter_->GetEffect() == nullptr);
        fixture.Step(5);
        REQUIRE(LiveParticles(fixture.emitter_) == 0);
    }

    SECTION("the effect round trips through its resource ref")
    {
        fixture.emitter_->SetEffect(effect);
        const ResourceRef reference = fixture.emitter_->GetEffectAttr();
        REQUIRE(reference.type_ == ParticleEffect::GetTypeStatic());
        REQUIRE(reference.name_ == effect->GetName());

        fixture.emitter_->SetEffect(nullptr);
        fixture.emitter_->SetEffectAttr(reference);
        REQUIRE(fixture.emitter_->GetEffect() == effect);
    }
}

TEST_CASE("A particle emitter with a spherical emitter fills a volume", "[Graphics]")
{
    Context* context = HeadlessContext();
    SharedPtr<ParticleEffect> effect(new ParticleEffect(context));
    effect->SetNumParticles(64);
    effect->SetMinEmissionRate(200.0f);
    effect->SetMaxEmissionRate(200.0f);
    effect->SetMinTimeToLive(2.0f);
    effect->SetMaxTimeToLive(2.0f);
    effect->SetEmitterType(EMITTER_SPHERE);
    effect->SetEmitterSize(Vector3(4.0f, 4.0f, 4.0f));
    effect->SetMinVelocity(0.0f);
    effect->SetMaxVelocity(0.0f);
    effect->SetConstantForce(Vector3::ZERO);
    effect->SetUpdateInvisible(true);
    effect->SetMinParticleSize(Vector2(0.1f, 0.1f));
    effect->SetMaxParticleSize(Vector2(0.1f, 0.1f));

    EmitterFixture fixture(effect);
    fixture.Step(20);

    REQUIRE(LiveParticles(fixture.emitter_) > 0);

    const BoundingBox bounds = fixture.emitter_->GetWorldBoundingBox();
    REQUIRE(bounds.Size().x_ > 0.5f);
    REQUIRE(bounds.Size().y_ > 0.5f);

    SECTION("a box emitter also spreads the particles out")
    {
        effect->SetEmitterType(EMITTER_BOX);
        fixture.emitter_->Reset();
        fixture.Step(20);
        REQUIRE(fixture.emitter_->GetWorldBoundingBox().Size().x_ > 0.5f);
    }

    SECTION("a constant force moves the cloud")
    {
        effect->SetConstantForce(Vector3(0.0f, -20.0f, 0.0f));
        fixture.emitter_->Reset();
        fixture.Step(10);
        const float low = fixture.emitter_->GetWorldBoundingBox().min_.y_;
        fixture.Step(20);
        REQUIRE(fixture.emitter_->GetWorldBoundingBox().min_.y_ < low);
    }
}

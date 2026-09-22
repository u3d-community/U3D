#include "TestUtils.h"

#include <Urho3D/Audio/Audio.h>
#include <Urho3D/Audio/BufferedSoundStream.h>
#include <Urho3D/Audio/Sound.h>
#include <Urho3D/Audio/SoundListener.h>
#include <Urho3D/Audio/SoundSource.h>
#include <Urho3D/Audio/SoundSource3D.h>
#include <Urho3D/IO/File.h>
#include <Urho3D/IO/VectorBuffer.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/Scene.h>

using namespace Urho3D;
using namespace U3DTest;

namespace
{

SharedPtr<Sound> MakeSound(Context* context, unsigned frequency = 22050, bool sixteenBit = true, bool stereo = false)
{
    SharedPtr<Sound> sound(new Sound(context));
    const unsigned sampleSize = (sixteenBit ? 2u : 1u) * (stereo ? 2u : 1u);
    sound->SetSize(frequency * sampleSize);
    sound->SetFormat(frequency, sixteenBit, stereo);
    return sound;
}

}

TEST_CASE("Sound describes its own format", "[Audio]")
{
    TestContext context;
    SharedPtr<Sound> sound = MakeSound(context);

    REQUIRE(sound->GetIntFrequency() == 22050);
    REQUIRE_EQ_F(sound->GetFrequency(), 22050.0f);
    REQUIRE(sound->IsSixteenBit());
    REQUIRE_FALSE(sound->IsStereo());
    REQUIRE_FALSE(sound->IsCompressed());
    REQUIRE(sound->GetSampleSize() == 2);
    REQUIRE_NEAR(sound->GetLength(), 1.0f, 0.001f);
    REQUIRE(sound->GetStart() != nullptr);
    REQUIRE(sound->GetEnd() == sound->GetStart() + sound->GetDataSize());

    SECTION("stereo and eight bit change the sample size and the length")
    {
        SharedPtr<Sound> stereo = MakeSound(context, 44100, true, true);
        REQUIRE(stereo->GetSampleSize() == 4);
        REQUIRE_NEAR(stereo->GetLength(), 1.0f, 0.001f);

        SharedPtr<Sound> eightBit = MakeSound(context, 11025, false, false);
        REQUIRE(eightBit->GetSampleSize() == 1);
        REQUIRE_FALSE(eightBit->IsSixteenBit());
    }

    SECTION("a sound with no data has no length")
    {
        SharedPtr<Sound> empty(new Sound(context));
        REQUIRE_EQ_F(empty->GetLength(), 0.0f);
        REQUIRE(empty->GetDataSize() == 0);
    }
}

TEST_CASE("Sound loop points bracket the data", "[Audio]")
{
    TestContext context;
    SharedPtr<Sound> sound = MakeSound(context);

    REQUIRE_FALSE(sound->IsLooped());
    REQUIRE(sound->GetRepeat() == nullptr);
    REQUIRE(sound->GetEnd() == sound->GetStart() + sound->GetDataSize());

    sound->SetLooped(true);
    REQUIRE(sound->IsLooped());
    REQUIRE(sound->GetRepeat() == sound->GetStart());
    REQUIRE(sound->GetEnd() == sound->GetStart() + sound->GetDataSize());

    sound->SetLoop(1000, 2000);
    REQUIRE(sound->IsLooped());
    REQUIRE(sound->GetRepeat() == sound->GetStart() + 1000);
    REQUIRE(sound->GetEnd() == sound->GetStart() + 2000);

    SECTION("out of range loop points are clamped into the data")
    {
        sound->SetLoop(0, sound->GetDataSize() * 4);
        REQUIRE(sound->GetEnd() == sound->GetStart() + sound->GetDataSize());
        REQUIRE(sound->GetRepeat() == sound->GetStart());
    }

    SECTION("loop offsets are aligned down onto a sample boundary")
    {
        sound->SetLoop(1001, 2003);
        REQUIRE(sound->GetRepeat() == sound->GetStart() + 1000);
        REQUIRE(sound->GetEnd() == sound->GetStart() + 2002);
    }

    SECTION("clearing the loop puts the end back at the end of the data")
    {
        sound->SetLooped(false);
        REQUIRE_FALSE(sound->IsLooped());
        REQUIRE(sound->GetEnd() == sound->GetStart() + sound->GetDataSize());
    }
}

TEST_CASE("Sound loads a wav file from the data directory", "[Audio]")
{
    Context* context = HeadlessContext();
    auto* sound = context->GetSubsystem<ResourceCache>()->GetResource<Sound>("Sounds/PlayerFist.wav");

    REQUIRE(sound != nullptr);
    REQUIRE(sound->GetDataSize() > 0);
    REQUIRE(sound->GetIntFrequency() > 0);
    REQUIRE(sound->GetLength() > 0.0f);
    REQUIRE_FALSE(sound->IsCompressed());
    REQUIRE(sound->GetMemoryUse() >= sound->GetDataSize());
}

TEST_CASE("Sound rejects data in a format it does not recognise", "[Audio]")
{
    Context* context = HeadlessContext();
    const String garbage("not a sound file at all");

    Sound wav(context);
    VectorBuffer wavSource(garbage.CString(), garbage.Length());
    REQUIRE_FALSE(wav.LoadWav(wavSource));

    Sound ogg(context);
    VectorBuffer oggSource(garbage.CString(), garbage.Length());
    REQUIRE_FALSE(ogg.LoadOggVorbis(oggSource));

    SECTION("raw loading accepts any payload as untyped samples")
    {
        Sound raw(context);
        VectorBuffer rawSource(garbage.CString(), garbage.Length());
        REQUIRE(raw.LoadRaw(rawSource));
        REQUIRE(raw.GetDataSize() == garbage.Length());
        REQUIRE_FALSE(raw.IsCompressed());
    }

    SECTION("an empty payload leaves the sound empty")
    {
        Sound empty(context);
        VectorBuffer emptySource;
        REQUIRE(empty.LoadRaw(emptySource));
        REQUIRE(empty.GetDataSize() == 0);
    }
}

TEST_CASE("Sound decompresses an ogg vorbis stream", "[Audio]")
{
    Context* context = HeadlessContext();
    auto* sound = context->GetSubsystem<ResourceCache>()->GetResource<Sound>("Music/Ninja Gods.ogg");

    REQUIRE(sound != nullptr);
    REQUIRE(sound->IsCompressed());
    REQUIRE(sound->GetLength() > 0.0f);

    SharedPtr<SoundStream> stream = sound->GetDecoderStream();
    REQUIRE(stream.NotNull());
    REQUIRE(stream->GetIntFrequency() > 0);

    PODVector<signed char> decoded(4096);
    const unsigned produced = stream->GetData(decoded.Buffer(), decoded.Size());
    REQUIRE(produced > 0);
    REQUIRE(produced <= decoded.Size());
}

TEST_CASE("BufferedSoundStream queues and drains data", "[Audio]")
{
    SharedPtr<BufferedSoundStream> stream(new BufferedSoundStream());
    stream->SetFormat(22050, true, false);

    REQUIRE(stream->GetIntFrequency() == 22050);
    REQUIRE(stream->IsSixteenBit());
    REQUIRE_FALSE(stream->IsStereo());
    REQUIRE(stream->GetSampleSize() == 2);
    REQUIRE(stream->GetBufferNumBytes() == 0);
    REQUIRE_EQ_F(stream->GetBufferLength(), 0.0f);

    signed char payload[441 * 2]{};
    for (unsigned i = 0; i < sizeof payload; ++i)
        payload[i] = static_cast<signed char>(i);

    stream->AddData(payload, sizeof payload);
    REQUIRE(stream->GetBufferNumBytes() == sizeof payload);
    REQUIRE_NEAR(stream->GetBufferLength(), 0.02f, 0.001f);

    signed char destination[sizeof payload]{};
    REQUIRE(stream->GetData(destination, sizeof destination) == sizeof destination);
    REQUIRE(destination[0] == payload[0]);
    REQUIRE(destination[sizeof payload - 1] == payload[sizeof payload - 1]);
    REQUIRE(stream->GetBufferNumBytes() == 0);

    SECTION("a request larger than the buffer is only partly satisfied")
    {
        stream->AddData(payload, 16);
        signed char oversized[64];
        REQUIRE(stream->GetData(oversized, sizeof oversized) == 16);
    }

    SECTION("draining an empty stream produces nothing")
    {
        signed char nothing[8];
        REQUIRE(stream->GetData(nothing, sizeof nothing) == 0);
    }

    SECTION("Clear discards everything that was queued")
    {
        stream->AddData(payload, sizeof payload);
        stream->Clear();
        REQUIRE(stream->GetBufferNumBytes() == 0);
    }

    SECTION("the stop at end flag round trips")
    {
        REQUIRE_FALSE(stream->GetStopAtEnd());
        stream->SetStopAtEnd(true);
        REQUIRE(stream->GetStopAtEnd());
    }
}

TEST_CASE("Audio subsystem manages gains and sound types", "[Audio]")
{
    Audio* audio = HeadlessContext()->GetSubsystem<Audio>();
    REQUIRE(audio != nullptr);

    REQUIRE_FALSE(audio->IsInitialized());

    audio->SetMasterGain(SOUND_MUSIC, 0.5f);
    REQUIRE_NEAR(audio->GetSoundSourceMasterGain(SOUND_MUSIC), 0.5f, 0.001f);

    audio->SetMasterGain(SOUND_EFFECT, 2.0f);
    REQUIRE_NEAR(audio->GetSoundSourceMasterGain(SOUND_EFFECT), 1.0f, 0.001f);

    audio->SetMasterGain(SOUND_EFFECT, -1.0f);
    REQUIRE_NEAR(audio->GetSoundSourceMasterGain(SOUND_EFFECT), 0.0f, 0.001f);
    audio->SetMasterGain(SOUND_EFFECT, 1.0f);

    SECTION("an unknown sound type reports full gain")
    {
        REQUIRE_NEAR(audio->GetSoundSourceMasterGain("NeverRegistered"), 1.0f, 0.001f);
    }

    SECTION("a sound type can be paused and resumed")
    {
        audio->PauseSoundType(SOUND_MUSIC);
        REQUIRE(audio->IsSoundTypePaused(SOUND_MUSIC));
        audio->ResumeSoundType(SOUND_MUSIC);
        REQUIRE_FALSE(audio->IsSoundTypePaused(SOUND_MUSIC));

        audio->PauseSoundType(SOUND_MUSIC);
        audio->ResumeAll();
        REQUIRE_FALSE(audio->IsSoundTypePaused(SOUND_MUSIC));
    }
}

TEST_CASE("SoundSource plays, clamps its settings, and stops", "[Audio]")
{
    Context* context = HeadlessContext();
    SharedPtr<Scene> scene(new Scene(context));
    Node* node = scene->CreateChild("Speaker");
    auto* source = node->CreateComponent<SoundSource>();

    REQUIRE_FALSE(source->IsPlaying());
    REQUIRE(source->GetSound() == nullptr);
    REQUIRE(source->GetSoundType() == SOUND_EFFECT);

    source->SetGain(2.0f);
    REQUIRE_NEAR(source->GetGain(), 2.0f, 0.001f);
    source->SetGain(-1.0f);
    REQUIRE_NEAR(source->GetGain(), 0.0f, 0.001f);
    source->SetGain(0.75f);
    REQUIRE_NEAR(source->GetGain(), 0.75f, 0.001f);

    source->SetPanning(-5.0f);
    REQUIRE_NEAR(source->GetPanning(), -1.0f, 0.001f);
    source->SetPanning(5.0f);
    REQUIRE_NEAR(source->GetPanning(), 1.0f, 0.001f);

    source->SetAttenuation(2.0f);
    REQUIRE_NEAR(source->GetAttenuation(), 1.0f, 0.001f);

    source->SetSoundType(SOUND_MUSIC);
    REQUIRE(source->GetSoundType() == SOUND_MUSIC);

    source->SetAutoRemoveMode(REMOVE_COMPONENT);
    REQUIRE(source->GetAutoRemoveMode() == REMOVE_COMPONENT);
    source->SetAutoRemoveMode(REMOVE_DISABLED);

    SharedPtr<Sound> sound = MakeSound(context);
    source->Play(sound);
    REQUIRE(source->IsPlaying());
    REQUIRE(source->GetSound() == sound);
    REQUIRE(source->GetPlayPosition() != nullptr);

    SECTION("the play position advances with the mix and can be sought")
    {
        source->Seek(0.5f);
        REQUIRE_NEAR(source->GetTimePosition(), 0.5f, 0.05f);

        source->SetPlayPosition(sound->GetStart());
        REQUIRE(source->GetPlayPosition() == sound->GetStart());
    }

    SECTION("the frequency is clamped to the mixer's supported range")
    {
        source->Play(sound, 44100.0f);
        REQUIRE_NEAR(source->GetFrequency(), 44100.0f, 1.0f);
        source->Play(sound, -100.0f);
        REQUIRE(source->GetFrequency() >= 0.0f);
    }

    SECTION("Stop releases the sound")
    {
        source->Stop();
        REQUIRE_FALSE(source->IsPlaying());
        REQUIRE(source->GetPlayPosition() == nullptr);
    }

    SECTION("playing a null sound stops any current playback")
    {
        source->Play(static_cast<Sound*>(nullptr));
        REQUIRE_FALSE(source->IsPlaying());
    }

    SECTION("a sound with no data never starts")
    {
        SharedPtr<Sound> empty(new Sound(context));
        source->Play(empty);
        REQUIRE_FALSE(source->IsPlaying());
    }
}

TEST_CASE("SoundSource3D clamps its attenuation parameters", "[Audio]")
{
    Context* context = HeadlessContext();
    SharedPtr<Scene> scene(new Scene(context));
    Node* node = scene->CreateChild("Emitter");
    auto* source = node->CreateComponent<SoundSource3D>();

    source->SetDistanceAttenuation(5.0f, 50.0f, 2.0f);
    REQUIRE_NEAR(source->GetNearDistance(), 5.0f, 0.001f);
    REQUIRE_NEAR(source->GetFarDistance(), 50.0f, 0.001f);
    REQUIRE_NEAR(source->RollAngleoffFactor(), 2.0f, 0.001f);

    source->SetNearDistance(-1.0f);
    REQUIRE_NEAR(source->GetNearDistance(), 0.0f, 0.001f);

    source->SetFarDistance(-1.0f);
    REQUIRE_NEAR(source->GetFarDistance(), 0.0f, 0.001f);

    source->SetAngleAttenuation(30.0f, 90.0f);
    REQUIRE_NEAR(source->GetInnerAngle(), 30.0f, 0.001f);
    REQUIRE_NEAR(source->GetOuterAngle(), 90.0f, 0.001f);

    source->SetInnerAngle(-10.0f);
    REQUIRE(source->GetInnerAngle() >= 0.0f);
    source->SetOuterAngle(500.0f);
    REQUIRE(source->GetOuterAngle() <= 360.0f);

    SECTION("attenuation drops off with distance from the listener")
    {
        Node* listenerNode = scene->CreateChild("Listener");
        auto* listener = listenerNode->CreateComponent<SoundListener>();
        HeadlessContext()->GetSubsystem<Audio>()->SetListener(listener);
        REQUIRE(HeadlessContext()->GetSubsystem<Audio>()->GetListener() == listener);

        source->SetDistanceAttenuation(1.0f, 100.0f, 1.0f);

        node->SetWorldPosition(Vector3::ZERO);
        source->CalculateAttenuation();
        const float atOrigin = source->GetAttenuation();

        node->SetWorldPosition(Vector3(0.0f, 0.0f, 50.0f));
        source->CalculateAttenuation();
        REQUIRE(source->GetAttenuation() < atOrigin);

        node->SetWorldPosition(Vector3(0.0f, 0.0f, 1000.0f));
        source->CalculateAttenuation();
        REQUIRE_NEAR(source->GetAttenuation(), 0.0f, 0.001f);

        HeadlessContext()->GetSubsystem<Audio>()->SetListener(nullptr);
    }
}

#include "TestUtils.h"

#include <Urho3D/Audio/Audio.h>
#include <Urho3D/Audio/BufferedSoundStream.h>
#include <Urho3D/Audio/Sound.h>
#include <Urho3D/Audio/SoundSource.h>
#include <Urho3D/Scene/Scene.h>

using namespace Urho3D;
using namespace U3DTest;

namespace
{

SharedPtr<Sound> MakeSound16(Context* context, const PODVector<short>& samples, unsigned frequency = 44100)
{
    SharedPtr<Sound> sound(new Sound(context));
    sound->SetSize(samples.Size() * sizeof(short));
    memcpy(sound->GetStart(), samples.Buffer(), samples.Size() * sizeof(short));
    sound->SetFormat(frequency, true, false);
    return sound;
}

SharedPtr<Sound> MakeSound8(Context* context, const PODVector<signed char>& samples, unsigned frequency = 44100)
{
    SharedPtr<Sound> sound(new Sound(context));
    sound->SetSize(samples.Size());
    memcpy(sound->GetStart(), samples.Buffer(), samples.Size());
    sound->SetFormat(frequency, false, false);
    return sound;
}

SharedPtr<Sound> MakeSoundStereo16(Context* context, const PODVector<short>& samples, unsigned frequency = 44100)
{
    SharedPtr<Sound> sound(new Sound(context));
    sound->SetSize(samples.Size() * sizeof(short));
    memcpy(sound->GetStart(), samples.Buffer(), samples.Size() * sizeof(short));
    sound->SetFormat(frequency, true, true);
    return sound;
}

PODVector<short> Ramp(unsigned count, short step = 1000)
{
    PODVector<short> samples;
    for (unsigned i = 0; i < count; ++i)
        samples.Push(static_cast<short>((i + 1) * step));
    return samples;
}

struct MixFixture
{
    MixFixture() :
        scene_(new Scene(HeadlessContext()))
    {
        source_ = scene_->CreateChild("Speaker")->CreateComponent<SoundSource>();
    }

    SharedPtr<Scene> scene_;
    SoundSource* source_{};
};

}

TEST_CASE("SoundSource mixes 16 bit mono into a mono buffer", "[Audio]")
{
    MixFixture fixture;
    SharedPtr<Sound> sound = MakeSound16(HeadlessContext(), Ramp(8));

    fixture.source_->Play(sound);
    REQUIRE(fixture.source_->IsPlaying());

    int buffer[8]{};
    fixture.source_->Mix(buffer, 8, 44100, false, false);

    for (int i = 0; i < 8; ++i)
        REQUIRE(buffer[i] == (i + 1) * 1000);

    SECTION("the mix accumulates into the destination rather than overwriting it")
    {
        fixture.source_->Play(sound);
        int accumulate[4];
        for (int i = 0; i < 4; ++i)
            accumulate[i] = 5;

        fixture.source_->Mix(accumulate, 4, 44100, false, false);
        for (int i = 0; i < 4; ++i)
            REQUIRE(accumulate[i] == 5 + (i + 1) * 1000);
    }

    SECTION("half gain halves the samples")
    {
        fixture.source_->Play(sound);
        fixture.source_->SetGain(0.5f);

        int halved[4]{};
        fixture.source_->Mix(halved, 4, 44100, false, false);
        for (int i = 0; i < 4; ++i)
            REQUIRE(halved[i] == ((i + 1) * 1000 * 128) / 256);
    }

    SECTION("zero gain still advances the play position without writing anything")
    {
        fixture.source_->Play(sound);
        fixture.source_->SetGain(0.0f);

        int silent[4];
        for (int i = 0; i < 4; ++i)
            silent[i] = 7;

        const signed char* before = const_cast<const signed char*>(fixture.source_->GetPlayPosition());
        fixture.source_->Mix(silent, 4, 44100, false, false);

        for (int i = 0; i < 4; ++i)
            REQUIRE(silent[i] == 7);
        REQUIRE(const_cast<const signed char*>(fixture.source_->GetPlayPosition()) == before + 4 * sizeof(short));
    }

    SECTION("attenuation scales the output alongside the gain")
    {
        fixture.source_->Play(sound);
        fixture.source_->SetAttenuation(0.5f);

        int attenuated[4]{};
        fixture.source_->Mix(attenuated, 4, 44100, false, false);
        for (int i = 0; i < 4; ++i)
            REQUIRE(attenuated[i] == ((i + 1) * 1000 * 128) / 256);
    }

    SECTION("a disabled source contributes nothing")
    {
        fixture.source_->Play(sound);
        fixture.source_->SetEnabled(false);

        int untouched[4]{};
        fixture.source_->Mix(untouched, 4, 44100, false, false);
        for (int i = 0; i < 4; ++i)
            REQUIRE(untouched[i] == 0);
    }

    SECTION("a source that was never played contributes nothing")
    {
        fixture.source_->Stop();
        int untouched[4]{};
        fixture.source_->Mix(untouched, 4, 44100, false, false);
        for (int i = 0; i < 4; ++i)
            REQUIRE(untouched[i] == 0);
    }
}

TEST_CASE("SoundSource stops a one shot at the end of the data", "[Audio]")
{
    MixFixture fixture;
    SharedPtr<Sound> sound = MakeSound16(HeadlessContext(), Ramp(4));

    fixture.source_->Play(sound);

    int buffer[16]{};
    fixture.source_->Mix(buffer, 16, 44100, false, false);

    for (int i = 0; i < 4; ++i)
        REQUIRE(buffer[i] == (i + 1) * 1000);
    for (int i = 8; i < 16; ++i)
        REQUIRE(buffer[i] == 0);

    REQUIRE_FALSE(fixture.source_->IsPlaying());
    REQUIRE(fixture.source_->GetPlayPosition() == nullptr);
}

TEST_CASE("A looped sound wraps back to its repeat point while mixing", "[Audio]")
{
    MixFixture fixture;
    SharedPtr<Sound> sound = MakeSound16(HeadlessContext(), Ramp(4));
    sound->SetLooped(true);

    fixture.source_->Play(sound);

    int buffer[12]{};
    fixture.source_->Mix(buffer, 12, 44100, false, false);

    for (int i = 0; i < 12; ++i)
        REQUIRE(buffer[i] == ((i % 4) + 1) * 1000);

    REQUIRE(fixture.source_->IsPlaying());

    SECTION("a partial loop only repeats the tail of the data")
    {
        SharedPtr<Sound> partial = MakeSound16(HeadlessContext(), Ramp(4));
        partial->SetLoop(2 * sizeof(short), 4 * sizeof(short));
        fixture.source_->Play(partial);

        int looped[8]{};
        fixture.source_->Mix(looped, 8, 44100, false, false);

        const int expected[8] = {1000, 2000, 3000, 4000, 3000, 4000, 3000, 4000};
        for (int i = 0; i < 8; ++i)
            REQUIRE(looped[i] == expected[i]);
    }
}

TEST_CASE("SoundSource mixes mono into a stereo buffer with panning", "[Audio]")
{
    MixFixture fixture;
    SharedPtr<Sound> sound = MakeSound16(HeadlessContext(), Ramp(4));

    fixture.source_->Play(sound);
    fixture.source_->SetPanning(0.0f);

    int centred[8]{};
    fixture.source_->Mix(centred, 4, 44100, true, false);

    for (int i = 0; i < 4; ++i)
        REQUIRE(centred[i * 2] == centred[i * 2 + 1]);
    REQUIRE(centred[0] > 0);

    SECTION("hard left silences the right channel")
    {
        fixture.source_->Play(sound);
        fixture.source_->SetPanning(-1.0f);

        int left[8]{};
        fixture.source_->Mix(left, 4, 44100, true, false);

        for (int i = 0; i < 4; ++i)
        {
            REQUIRE(left[i * 2] > 0);
            REQUIRE(left[i * 2 + 1] == 0);
        }
    }

    SECTION("hard right silences the left channel")
    {
        fixture.source_->Play(sound);
        fixture.source_->SetPanning(1.0f);

        int right[8]{};
        fixture.source_->Mix(right, 4, 44100, true, false);

        for (int i = 0; i < 4; ++i)
        {
            REQUIRE(right[i * 2] == 0);
            REQUIRE(right[i * 2 + 1] > 0);
        }
    }
}

TEST_CASE("SoundSource mixes stereo material to both buffer layouts", "[Audio]")
{
    MixFixture fixture;

    PODVector<short> interleaved;
    interleaved.Push(100);
    interleaved.Push(200);
    interleaved.Push(300);
    interleaved.Push(400);
    SharedPtr<Sound> sound = MakeSoundStereo16(HeadlessContext(), interleaved);
    REQUIRE(sound->IsStereo());
    REQUIRE(sound->GetSampleSize() == 4);

    fixture.source_->Play(sound);

    int stereo[4]{};
    fixture.source_->Mix(stereo, 2, 44100, true, false);
    REQUIRE(stereo[0] == 100);
    REQUIRE(stereo[1] == 200);
    REQUIRE(stereo[2] == 300);
    REQUIRE(stereo[3] == 400);

    SECTION("mixing stereo down to mono averages the two channels")
    {
        fixture.source_->Play(sound);

        int mono[2]{};
        fixture.source_->Mix(mono, 2, 44100, false, false);
        REQUIRE(mono[0] == (100 + 200) / 2);
        REQUIRE(mono[1] == (300 + 400) / 2);
    }
}

TEST_CASE("SoundSource mixes eight bit material", "[Audio]")
{
    MixFixture fixture;

    PODVector<signed char> samples;
    samples.Push(10);
    samples.Push(-20);
    samples.Push(30);
    samples.Push(-40);
    SharedPtr<Sound> sound = MakeSound8(HeadlessContext(), samples);
    REQUIRE_FALSE(sound->IsSixteenBit());
    REQUIRE(sound->GetSampleSize() == 1);

    fixture.source_->Play(sound);

    int buffer[4]{};
    fixture.source_->Mix(buffer, 4, 44100, false, false);
    REQUIRE(buffer[0] == 10 * 256);
    REQUIRE(buffer[1] == -20 * 256);
    REQUIRE(buffer[2] == 30 * 256);
    REQUIRE(buffer[3] == -40 * 256);
}

TEST_CASE("SoundSource resamples when the rates differ", "[Audio]")
{
    MixFixture fixture;
    SharedPtr<Sound> sound = MakeSound16(HeadlessContext(), Ramp(8));

    SECTION("playing at half the mixer rate repeats each sample")
    {
        fixture.source_->Play(sound, 22050.0f);
        REQUIRE_NEAR(fixture.source_->GetFrequency(), 22050.0f, 1.0f);

        int buffer[8]{};
        fixture.source_->Mix(buffer, 8, 44100, false, false);

        const int expected[8] = {1000, 1000, 2000, 2000, 3000, 3000, 4000, 4000};
        for (int i = 0; i < 8; ++i)
            REQUIRE(buffer[i] == expected[i]);
    }

    SECTION("playing at twice the mixer rate skips every other sample")
    {
        fixture.source_->Play(sound, 88200.0f);

        int buffer[4]{};
        fixture.source_->Mix(buffer, 4, 44100, false, false);

        const int expected[4] = {1000, 3000, 5000, 7000};
        for (int i = 0; i < 4; ++i)
            REQUIRE(buffer[i] == expected[i]);
    }

    SECTION("interpolation blends between neighbouring samples")
    {
        fixture.source_->Play(sound, 22050.0f);

        int buffer[4]{};
        fixture.source_->Mix(buffer, 4, 44100, false, true);

        REQUIRE(buffer[0] == 1000);
        REQUIRE(buffer[1] > 1000);
        REQUIRE(buffer[1] < 2000);
        REQUIRE(buffer[2] == 2000);
        REQUIRE(buffer[3] > 2000);
        REQUIRE(buffer[3] < 3000);
    }

    SECTION("interpolated stereo output keeps both channels in step")
    {
        fixture.source_->Play(sound, 22050.0f);
        fixture.source_->SetPanning(0.0f);

        int buffer[8]{};
        fixture.source_->Mix(buffer, 4, 44100, true, true);
        for (int i = 0; i < 4; ++i)
            REQUIRE(buffer[i * 2] == buffer[i * 2 + 1]);
    }
}

TEST_CASE("SoundSource mixes from a buffered stream", "[Audio]")
{
    MixFixture fixture;
    SharedPtr<BufferedSoundStream> stream(new BufferedSoundStream());
    stream->SetFormat(44100, true, false);

    const PODVector<short> samples = Ramp(64);
    stream->AddData((void*)samples.Buffer(), samples.Size() * sizeof(short));

    fixture.source_->Play(stream);
    REQUIRE(fixture.source_->IsPlaying());

    int buffer[16]{};
    fixture.source_->Mix(buffer, 16, 44100, false, false);

    REQUIRE(buffer[0] != 0);
    REQUIRE(buffer[15] != 0);

    SECTION("a stream that runs dry mixes silence rather than stopping")
    {
        stream->Clear();
        int drained[16]{};
        for (int pass = 0; pass < 8; ++pass)
        {
            for (int i = 0; i < 16; ++i)
                drained[i] = 0;
            fixture.source_->Mix(drained, 16, 44100, false, false);
        }

        for (int i = 0; i < 16; ++i)
            REQUIRE(drained[i] == 0);
        REQUIRE(fixture.source_->IsPlaying());
    }

    SECTION("a stream set to stop at the end finishes the source")
    {
        stream->SetStopAtEnd(true);
        stream->Clear();
        int drained[16]{};
        for (int i = 0; i < 8; ++i)
            fixture.source_->Mix(drained, 16, 44100, false, false);
        REQUIRE_FALSE(fixture.source_->IsPlaying());
    }
}

TEST_CASE("SoundSource reports and seeks its time position", "[Audio]")
{
    MixFixture fixture;
    SharedPtr<Sound> sound = MakeSound16(HeadlessContext(), Ramp(44100));

    fixture.source_->Play(sound);
    REQUIRE_NEAR(fixture.source_->GetTimePosition(), 0.0f, 0.001f);

    PODVector<int> buffer(11025);
    fixture.source_->Mix(buffer.Buffer(), buffer.Size(), 44100, false, false);

    REQUIRE_NEAR(fixture.source_->GetTimePosition(), 0.25f, 0.01f);

    SECTION("seeking moves the play position within the data")
    {
        fixture.source_->Seek(0.5f);
        REQUIRE_NEAR(fixture.source_->GetTimePosition(), 0.5f, 0.01f);

        int next[4]{};
        fixture.source_->Mix(next, 4, 44100, false, false);
        REQUIRE(next[0] != 0);
    }

    SECTION("seeking past the end is clamped inside the sound")
    {
        fixture.source_->Seek(10.0f);
        REQUIRE(fixture.source_->GetTimePosition() <= sound->GetLength());
    }
}

TEST_CASE("Audio mixes every registered source into one output buffer", "[Audio]")
{
    Context* context = HeadlessContext();
    Audio* audio = context->GetSubsystem<Audio>();

    SharedPtr<Scene> scene(new Scene(context));
    auto* first = scene->CreateChild("A")->CreateComponent<SoundSource>();
    auto* second = scene->CreateChild("B")->CreateComponent<SoundSource>();

    SharedPtr<Sound> sound = MakeSound16(context, Ramp(16));
    first->Play(sound);
    second->Play(sound);

    REQUIRE(audio->GetSoundSources().Contains(first));
    REQUIRE(audio->GetSoundSources().Contains(second));

    int buffer[8]{};
    first->Mix(buffer, 8, 44100, false, false);
    second->Mix(buffer, 8, 44100, false, false);

    for (int i = 0; i < 8; ++i)
        REQUIRE(buffer[i] == 2 * (i + 1) * 1000);

    SECTION("a paused sound type mutes only its own sources")
    {
        second->SetSoundType(SOUND_MUSIC);
        audio->PauseSoundType(SOUND_MUSIC);
        REQUIRE(audio->IsSoundTypePaused(SOUND_MUSIC));
        REQUIRE_FALSE(audio->IsSoundTypePaused(SOUND_EFFECT));
        audio->ResumeSoundType(SOUND_MUSIC);
    }

    SECTION("the master gain scales what a source contributes")
    {
        audio->SetMasterGain(SOUND_EFFECT, 0.5f);
        first->UpdateMasterGain();
        first->Play(sound);

        int quiet[4]{};
        first->Mix(quiet, 4, 44100, false, false);
        for (int i = 0; i < 4; ++i)
            REQUIRE(quiet[i] == ((i + 1) * 1000 * 128) / 256);

        audio->SetMasterGain(SOUND_EFFECT, 1.0f);
        first->UpdateMasterGain();
    }
}

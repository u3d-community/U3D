#include "TestUtils.h"

#include <Urho3D/Graphics/AnimatedModel.h>
#include <Urho3D/Graphics/Animation.h>
#include <Urho3D/Graphics/AnimationController.h>
#include <Urho3D/Graphics/AnimationState.h>
#include <Urho3D/Graphics/Geometry.h>
#include <Urho3D/Graphics/IndexBuffer.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Graphics/Skeleton.h>
#include <Urho3D/Graphics/VertexBuffer.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/Scene.h>

using namespace Urho3D;
using namespace U3DTest;

namespace
{

Model* LoadModel(const String& name)
{
    return HeadlessContext()->GetSubsystem<ResourceCache>()->GetResource<Model>(name);
}

Animation* LoadAnimation(const String& name)
{
    return HeadlessContext()->GetSubsystem<ResourceCache>()->GetResource<Animation>(name);
}

}

TEST_CASE("Model loads its geometry, bounds, and buffers", "[Graphics]")
{
    Model* model = LoadModel("Models/Box.mdl");
    REQUIRE(model != nullptr);

    REQUIRE(model->GetNumGeometries() == 1);
    REQUIRE(model->GetNumGeometryLodLevels(0) >= 1);
    REQUIRE(model->GetGeometry(0, 0) != nullptr);
    REQUIRE(model->GetGeometry(99, 0) == nullptr);

    const BoundingBox bounds = model->GetBoundingBox();
    REQUIRE(bounds.Defined());
    REQUIRE(bounds.Size().x_ > 0.0f);

    REQUIRE_FALSE(model->GetVertexBuffers().Empty());
    REQUIRE_FALSE(model->GetIndexBuffers().Empty());

    Geometry* geometry = model->GetGeometry(0, 0);
    REQUIRE(geometry->GetVertexCount() > 0);
    REQUIRE(geometry->GetIndexCount() > 0);
    REQUIRE(geometry->GetPrimitiveType() == TRIANGLE_LIST);

    SECTION("the shadow copies of the buffers are readable without a GPU")
    {
        VertexBuffer* vertices = model->GetVertexBuffers()[0];
        REQUIRE(vertices->GetVertexCount() > 0);
        REQUIRE(vertices->GetVertexSize() > 0);
        REQUIRE(vertices->GetShadowData() != nullptr);

        IndexBuffer* indices = model->GetIndexBuffers()[0];
        REQUIRE(indices->GetIndexCount() > 0);
        REQUIRE(indices->GetShadowData() != nullptr);
    }

    SECTION("the raw geometry data can be pulled out for a ray test")
    {
        const unsigned char* vertexData = nullptr;
        const unsigned char* indexData = nullptr;
        unsigned vertexSize = 0;
        unsigned indexSize = 0;
        const PODVector<VertexElement>* elements = nullptr;

        geometry->GetRawData(vertexData, vertexSize, indexData, indexSize, elements);
        REQUIRE(vertexData != nullptr);
        REQUIRE(indexData != nullptr);
        REQUIRE(vertexSize > 0);
        REQUIRE((indexSize == sizeof(unsigned short) || indexSize == sizeof(unsigned)));

        const Ray ray(Vector3(0.0f, 0.0f, -10.0f), Vector3::FORWARD);
        REQUIRE(geometry->GetHitDistance(ray) < M_INFINITY);
        REQUIRE(Ray(Vector3(100.0f, 0.0f, -10.0f), Vector3::FORWARD).HitDistance(
            vertexData, vertexSize, indexData, indexSize, geometry->GetIndexStart(), geometry->GetIndexCount())
            == M_INFINITY);
    }

    SECTION("a clone is an independent copy")
    {
        SharedPtr<Model> clone = model->Clone("Cloned");
        REQUIRE(clone.NotNull());
        REQUIRE(clone != model);
        REQUIRE(clone->GetNumGeometries() == model->GetNumGeometries());
        REQUIRE(clone->GetBoundingBox() == model->GetBoundingBox());

        clone->SetBoundingBox(BoundingBox(-100.0f, 100.0f));
        REQUIRE_FALSE(model->GetBoundingBox() == clone->GetBoundingBox());
    }
}

TEST_CASE("A skinned model carries a skeleton", "[Graphics]")
{
    Model* model = LoadModel("Models/Kachujin/Kachujin.mdl");
    REQUIRE(model != nullptr);

    const Skeleton& skeleton = model->GetSkeleton();
    REQUIRE(skeleton.GetNumBones() > 0);

    Skeleton& bones = const_cast<Skeleton&>(skeleton);
    Bone* root = bones.GetRootBone();
    REQUIRE(root != nullptr);
    REQUIRE_FALSE(root->name_.Empty());

    const unsigned rootIndex = bones.GetBoneIndex(root->name_);
    REQUIRE(rootIndex < skeleton.GetNumBones());
    REQUIRE(bones.GetBone(rootIndex) == root);
    REQUIRE(bones.GetBone(root->name_) == root);
    REQUIRE(bones.GetBoneIndex(String("NotABone")) == M_MAX_UNSIGNED);
    REQUIRE(bones.GetBone("NotABone") == nullptr);

    SECTION("every non root bone points at a parent inside the skeleton")
    {
        const Vector<Bone>& all = skeleton.GetBones();
        for (unsigned i = 0; i < all.Size(); ++i)
        {
            const Bone& bone = all[i];
            if (&bone == root)
                continue;
            REQUIRE(bone.parentIndex_ < all.Size());
            REQUIRE(bone.parentIndex_ != i);
        }
    }

    SECTION("a skeleton defined from another one matches it")
    {
        Skeleton copy;
        copy.Define(skeleton);
        REQUIRE(copy.GetNumBones() == skeleton.GetNumBones());
        REQUIRE(copy.GetRootBone()->name_ == root->name_);

        copy.ClearBones();
        REQUIRE(copy.GetNumBones() == 0);
        REQUIRE(copy.GetRootBone() == nullptr);
    }
}

TEST_CASE("Animation loads tracks and reports keyframes by time", "[Graphics]")
{
    Animation* animation = LoadAnimation("Models/Kachujin/Kachujin_Walk.ani");
    REQUIRE(animation != nullptr);

    REQUIRE(animation->GetLength() > 0.0f);
    REQUIRE(animation->GetNumTracks() > 0);
    REQUIRE_FALSE(animation->GetAnimationName().Empty());
    REQUIRE(animation->GetAnimationNameHash() == StringHash(animation->GetAnimationName()));

    AnimationTrack* track = animation->GetTrack(0u);
    REQUIRE(track != nullptr);
    REQUIRE(track->GetNumKeyFrames() > 1);
    REQUIRE(animation->GetTrack(track->name_) == track);
    REQUIRE(animation->GetTrack(String("NoSuchBone")) == nullptr);

    unsigned index = 0;
    REQUIRE(track->GetKeyFrameIndex(0.0f, index));
    REQUIRE(index == 0);

    REQUIRE(track->GetKeyFrameIndex(animation->GetLength(), index));
    REQUIRE(index == track->GetNumKeyFrames() - 1);

    SECTION("times before and after the track clamp onto the end keyframes")
    {
        unsigned before = 99;
        REQUIRE(track->GetKeyFrameIndex(-10.0f, before));
        REQUIRE(before == 0);

        unsigned after = 0;
        REQUIRE(track->GetKeyFrameIndex(animation->GetLength() * 10.0f, after));
        REQUIRE(after == track->GetNumKeyFrames() - 1);
    }

    SECTION("keyframe times increase along the track")
    {
        for (unsigned i = 1; i < track->GetNumKeyFrames(); ++i)
            REQUIRE(track->GetKeyFrame(i)->time_ >= track->GetKeyFrame(i - 1)->time_);
    }

    SECTION("a clone carries the tracks across")
    {
        SharedPtr<Animation> clone = animation->Clone("CloneName");
        REQUIRE(clone.NotNull());
        REQUIRE(clone->GetNumTracks() == animation->GetNumTracks());
        REQUIRE_EQ_F(clone->GetLength(), animation->GetLength());
        REQUIRE(clone->GetName() == "CloneName");

        clone->RemoveAllTracks();
        REQUIRE(clone->GetNumTracks() == 0);
        REQUIRE(animation->GetNumTracks() > 0);
    }
}

TEST_CASE("Animation tracks and triggers can be built by hand", "[Graphics]")
{
    TestContext context;
    Animation animation(context);

    animation.SetAnimationName("Handmade");
    animation.SetLength(2.0f);
    REQUIRE(animation.GetAnimationName() == "Handmade");
    REQUIRE_EQ_F(animation.GetLength(), 2.0f);

    AnimationTrack* track = animation.CreateTrack("Root");
    REQUIRE(track != nullptr);
    REQUIRE(animation.CreateTrack("Root") == track);
    track->channelMask_ = CHANNEL_POSITION;

    AnimationKeyFrame first;
    first.time_ = 0.0f;
    first.position_ = Vector3::ZERO;
    track->AddKeyFrame(first);

    AnimationKeyFrame second;
    second.time_ = 2.0f;
    second.position_ = Vector3(10.0f, 0.0f, 0.0f);
    track->AddKeyFrame(second);

    REQUIRE(track->GetNumKeyFrames() == 2);

    unsigned index = 99;
    REQUIRE(track->GetKeyFrameIndex(1.0f, index));
    REQUIRE(index == 0);

    animation.AddTrigger(0.5f, false, Variant("half"));
    animation.AddTrigger(1.0f, true, Variant("normalised"));
    REQUIRE(animation.GetNumTriggers() == 2);
    REQUIRE(animation.GetTrigger(0)->data_.GetString() == "half");
    REQUIRE_EQ_F(animation.GetTrigger(1)->time_, 2.0f);

    SECTION("triggers and tracks can be removed again")
    {
        animation.RemoveTrigger(0);
        REQUIRE(animation.GetNumTriggers() == 1);
        animation.RemoveAllTriggers();
        REQUIRE(animation.GetNumTriggers() == 0);

        REQUIRE(animation.RemoveTrack("Root"));
        REQUIRE_FALSE(animation.RemoveTrack("Root"));
        REQUIRE(animation.GetNumTracks() == 0);
    }

    SECTION("keyframes can be inserted and dropped mid track")
    {
        AnimationKeyFrame middle;
        middle.time_ = 1.0f;
        middle.position_ = Vector3(5.0f, 0.0f, 0.0f);
        track->InsertKeyFrame(1, middle);
        REQUIRE(track->GetNumKeyFrames() == 3);
        REQUIRE_EQ_F(track->GetKeyFrame(1)->time_, 1.0f);

        track->RemoveKeyFrame(1);
        REQUIRE(track->GetNumKeyFrames() == 2);
        REQUIRE_EQ_F(track->GetKeyFrame(1)->time_, 2.0f);

        track->RemoveAllKeyFrames();
        REQUIRE(track->GetNumKeyFrames() == 0);
        REQUIRE_FALSE(track->GetKeyFrameIndex(0.0f, index));
    }
}

TEST_CASE("AnimatedModel builds a bone hierarchy and plays an animation", "[Graphics]")
{
    Context* context = HeadlessContext();
    SharedPtr<Scene> scene(new Scene(context));
    scene->CreateComponent<Octree>();

    Model* model = LoadModel("Models/Kachujin/Kachujin.mdl");
    Animation* walk = LoadAnimation("Models/Kachujin/Kachujin_Walk.ani");
    REQUIRE(model != nullptr);
    REQUIRE(walk != nullptr);

    Node* node = scene->CreateChild("Actor");
    auto* animated = node->CreateComponent<AnimatedModel>();
    animated->SetModel(model);

    REQUIRE(animated->GetModel() == model);
    REQUIRE(animated->GetSkeleton().GetNumBones() == model->GetSkeleton().GetNumBones());
    REQUIRE(node->GetNumChildren(true) >= model->GetSkeleton().GetNumBones());

    AnimationState* state = animated->AddAnimationState(walk);
    REQUIRE(state != nullptr);
    REQUIRE(animated->GetNumAnimationStates() == 1);
    REQUIRE(animated->GetAnimationState(walk) == state);

    state->SetWeight(1.0f);
    state->SetLooped(true);
    REQUIRE(state->IsEnabled());
    REQUIRE(state->IsLooped());
    REQUIRE_EQ_F(state->GetLength(), walk->GetLength());

    Node* boneNode = animated->GetSkeleton().GetBone(1u)->node_;
    REQUIRE(boneNode != nullptr);
    const Vector3 rest = boneNode->GetPosition();
    const Quaternion restRotation = boneNode->GetRotation();

    state->SetTime(walk->GetLength() * 0.5f);
    state->Apply();
    REQUIRE_NEAR(state->GetTime(), walk->GetLength() * 0.5f, 0.001f);
    const bool unchanged = boneNode->GetPosition().Equals(rest) && boneNode->GetRotation().Equals(restRotation);
    REQUIRE_FALSE(unchanged);

    SECTION("the time wraps around on a looped state and clamps otherwise")
    {
        state->SetTime(0.0f);
        state->AddTime(walk->GetLength() * 1.25f);
        REQUIRE(state->GetTime() < walk->GetLength());

        state->SetLooped(false);
        state->SetTime(0.0f);
        state->AddTime(walk->GetLength() * 4.0f);
        REQUIRE_EQ_F(state->GetTime(), walk->GetLength());
    }

    SECTION("a zero weight state stops contributing")
    {
        state->SetWeight(0.0f);
        REQUIRE_FALSE(state->IsEnabled());
    }

    SECTION("removing the state leaves the model without animation")
    {
        animated->RemoveAnimationState(walk);
        REQUIRE(animated->GetNumAnimationStates() == 0);
        REQUIRE(animated->GetAnimationState(walk) == nullptr);
    }

    SECTION("the world bounding box follows the node")
    {
        node->SetPosition(Vector3(50.0f, 0.0f, 0.0f));
        REQUIRE(animated->GetWorldBoundingBox().Center().x_ > 40.0f);
    }
}

TEST_CASE("AnimationController drives states by name", "[Graphics]")
{
    Context* context = HeadlessContext();
    SharedPtr<Scene> scene(new Scene(context));
    scene->CreateComponent<Octree>();

    Node* node = scene->CreateChild("Actor");
    auto* animated = node->CreateComponent<AnimatedModel>();
    animated->SetModel(LoadModel("Models/Kachujin/Kachujin.mdl"));
    auto* controller = node->CreateComponent<AnimationController>();

    const String walk("Models/Kachujin/Kachujin_Walk.ani");

    REQUIRE_FALSE(controller->IsPlaying(walk));
    REQUIRE(controller->Play(walk, 0, true, 0.0f));
    REQUIRE(controller->IsPlaying(walk));
    REQUIRE(controller->GetAnimationState(walk) != nullptr);
    REQUIRE(controller->IsAtEnd(walk) == false);

    controller->SetSpeed(walk, 2.0f);
    REQUIRE_EQ_F(controller->GetSpeed(walk), 2.0f);

    controller->SetWeight(walk, 0.25f);
    REQUIRE_NEAR(controller->GetWeight(walk), 0.25f, 0.001f);

    controller->SetTime(walk, 0.1f);
    REQUIRE_NEAR(controller->GetTime(walk), 0.1f, 0.001f);

    SECTION("the controller advances the animation time")
    {
        controller->SetSpeed(walk, 1.0f);
        controller->SetTime(walk, 0.0f);
        controller->Update(0.1f);
        REQUIRE(controller->GetTime(walk) > 0.0f);
    }

    SECTION("stopping fades the state out and the next update removes it")
    {
        REQUIRE(controller->Stop(walk, 0.0f));
        controller->Update(0.0f);
        REQUIRE_FALSE(controller->IsPlaying(walk));
        REQUIRE(controller->GetAnimationState(walk) == nullptr);

        REQUIRE_FALSE(controller->Stop(walk, 0.0f));
    }

    SECTION("StopAll fades everything out, and the update that follows drops the states")
    {
        controller->StopAll(0.0f);
        REQUIRE(controller->IsPlaying(walk));
        controller->Update(0.0f);
        REQUIRE_FALSE(controller->IsPlaying(walk));
    }

    SECTION("an animation that does not exist is refused")
    {
        REQUIRE_FALSE(controller->Play("Models/NoSuchAnimation.ani", 0, false, 0.0f));
    }

    SECTION("playing exclusively on a layer stops the others on it")
    {
        REQUIRE(controller->PlayExclusive(walk, 0, true, 0.0f));
        REQUIRE(controller->IsPlaying(walk));
    }
}

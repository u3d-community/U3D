#include "TestUtils.h"

#include <Urho3D/Graphics/AnimatedModel.h>
#include <Urho3D/Graphics/Animation.h>
#include <Urho3D/Graphics/AnimationController.h>
#include <Urho3D/Graphics/AnimationState.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Graphics/Technique.h>
#include <Urho3D/Graphics/Texture2D.h>
#include <Urho3D/Resource/JSONFile.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Resource/XMLFile.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/ValueAnimation.h>

using namespace Urho3D;
using namespace U3DTest;

namespace
{

template <class T> T* Load(const String& name)
{
    return HeadlessContext()->GetSubsystem<ResourceCache>()->GetResource<T>(name);
}

}

TEST_CASE("Material holds textures per unit", "[Graphics]")
{
    Context* context = HeadlessContext();
    SharedPtr<Material> material(new Material(context));

    REQUIRE(material->GetTexture(TU_DIFFUSE) == nullptr);
    REQUIRE(material->GetTextures().Empty());

    auto* texture = Load<Texture2D>("Textures/Logo.png");
    REQUIRE(texture != nullptr);

    material->SetTexture(TU_DIFFUSE, texture);
    REQUIRE(material->GetTexture(TU_DIFFUSE) == texture);
    REQUIRE(material->GetTextures().Size() == 1);

    material->SetTexture(TU_NORMAL, texture);
    REQUIRE(material->GetTextures().Size() == 2);

    material->SetTexture(TU_DIFFUSE, nullptr);
    REQUIRE(material->GetTexture(TU_DIFFUSE) == nullptr);
    REQUIRE(material->GetTextures().Size() == 1);

    SECTION("the texture survives an XML round trip")
    {
        SharedPtr<XMLFile> document(new XMLFile(context));
        XMLElement root = document->CreateRoot("material");
        REQUIRE(material->Save(root));

        SharedPtr<Material> restored(new Material(context));
        REQUIRE(restored->Load(root));
        REQUIRE(restored->GetTexture(TU_NORMAL) == texture);
        REQUIRE(restored->GetTexture(TU_DIFFUSE) == nullptr);
    }

    SECTION("a clone carries the textures across")
    {
        SharedPtr<Material> clone = material->Clone("TexturedClone");
        REQUIRE(clone->GetTexture(TU_NORMAL) == texture);

        clone->SetTexture(TU_NORMAL, nullptr);
        REQUIRE(material->GetTexture(TU_NORMAL) == texture);
    }
}

TEST_CASE("Material animates a shader parameter over time", "[Graphics]")
{
    Context* context = HeadlessContext();
    SharedPtr<Scene> scene(new Scene(context));

    SharedPtr<Material> material(new Material(context));
    material->SetScene(scene);
    material->SetShaderParameter("MatDiffColor", Variant(Color::BLACK));

    SharedPtr<ValueAnimation> animation(new ValueAnimation(context));
    animation->SetKeyFrame(0.0f, Variant(Color::BLACK));
    animation->SetKeyFrame(1.0f, Variant(Color::WHITE));

    material->SetShaderParameterAnimation("MatDiffColor", animation, WM_CLAMP, 1.0f);
    REQUIRE(material->GetShaderParameterAnimation("MatDiffColor") == animation);
    REQUIRE(material->GetShaderParameterAnimationWrapMode("MatDiffColor") == WM_CLAMP);
    REQUIRE_EQ_F(material->GetShaderParameterAnimationSpeed("MatDiffColor"), 1.0f);

    scene->Update(0.5f);
    const Color midway = material->GetShaderParameter("MatDiffColor").GetColor();
    REQUIRE(midway.r_ > 0.0f);
    REQUIRE(midway.r_ < 1.0f);

    scene->Update(1.0f);
    REQUIRE_NEAR(material->GetShaderParameter("MatDiffColor").GetColor().r_, 1.0f, 0.01f);

    SECTION("the animation speed scales how fast it runs")
    {
        material->SetShaderParameterAnimation("MatDiffColor", animation, WM_CLAMP, 1.0f);
        material->SetShaderParameterAnimationSpeed("MatDiffColor", 4.0f);
        REQUIRE_EQ_F(material->GetShaderParameterAnimationSpeed("MatDiffColor"), 4.0f);

        scene->Update(0.5f);
        REQUIRE_NEAR(material->GetShaderParameter("MatDiffColor").GetColor().r_, 1.0f, 0.01f);
    }

    SECTION("clearing the animation leaves the last value in place")
    {
        material->SetShaderParameterAnimation("MatDiffColor", nullptr);
        REQUIRE(material->GetShaderParameterAnimation("MatDiffColor") == nullptr);

        const Color held = material->GetShaderParameter("MatDiffColor").GetColor();
        scene->Update(1.0f);
        REQUIRE(material->GetShaderParameter("MatDiffColor").GetColor() == held);
    }
}

TEST_CASE("Material round trips through JSON", "[Graphics]")
{
    Context* context = HeadlessContext();
    auto* source = Load<Material>("Materials/Stone.xml");
    REQUIRE(source != nullptr);

    JSONValue json;
    REQUIRE(source->Save(json));

    SharedPtr<Material> restored(new Material(context));
    REQUIRE(restored->Load(json));
    REQUIRE(restored->GetNumTechniques() == source->GetNumTechniques());
    REQUIRE(restored->GetCullMode() == source->GetCullMode());
}

TEST_CASE("AnimationState blends bone tracks by weight", "[Graphics]")
{
    Context* context = HeadlessContext();
    SharedPtr<Scene> scene(new Scene(context));
    scene->CreateComponent<Octree>();

    auto* model = Load<Model>("Models/Kachujin/Kachujin.mdl");
    auto* walk = Load<Animation>("Models/Kachujin/Kachujin_Walk.ani");
    REQUIRE(model != nullptr);
    REQUIRE(walk != nullptr);

    Node* node = scene->CreateChild("Actor");
    auto* animated = node->CreateComponent<AnimatedModel>();
    animated->SetModel(model);

    AnimationState* state = animated->AddAnimationState(walk);
    REQUIRE(state != nullptr);
    REQUIRE(state->GetAnimation() == walk);
    REQUIRE(state->GetModel() == animated);

    REQUIRE_EQ_F(state->GetWeight(), 0.0f);
    REQUIRE_FALSE(state->IsEnabled());

    Bone* bone = animated->GetSkeleton().GetBone(1u);
    REQUIRE(bone != nullptr);
    REQUIRE(bone->node_ != nullptr);

    animated->GetSkeleton().Reset();
    const Vector3 bind = bone->node_->GetPosition();
    const Quaternion bindRotation = bone->node_->GetRotation();

    state->SetWeight(1.0f);
    state->SetTime(walk->GetLength() * 0.5f);
    state->Apply();
    const Quaternion full = bone->node_->GetRotation();

    animated->GetSkeleton().Reset();
    state->SetWeight(0.5f);
    state->Apply();
    const Quaternion half = bone->node_->GetRotation();

    REQUIRE_FALSE(half.Equals(full));
    REQUIRE_FALSE(half.Equals(bindRotation));

    SECTION("the layer decides which state wins")
    {
        state->SetLayer(3);
        REQUIRE(state->GetLayer() == 3);
    }

    SECTION("a start bone limits the animation to a subtree")
    {
        Bone* root = animated->GetSkeleton().GetRootBone();
        state->SetWeight(1.0f);
        state->SetStartBone(root);
        REQUIRE(state->GetStartBone() == root);
        REQUIRE(state->IsEnabled());
    }

    SECTION("the weight is clamped to the unit range")
    {
        state->SetWeight(5.0f);
        REQUIRE_NEAR(state->GetWeight(), 1.0f, 0.001f);
        state->SetWeight(-5.0f);
        REQUIRE_NEAR(state->GetWeight(), 0.0f, 0.001f);

        state->SetWeight(0.5f);
        state->AddWeight(0.25f);
        REQUIRE_NEAR(state->GetWeight(), 0.75f, 0.001f);
        state->AddWeight(1.0f);
        REQUIRE_NEAR(state->GetWeight(), 1.0f, 0.001f);
    }

    SECTION("the time is clamped to the animation length")
    {
        state->SetTime(-10.0f);
        REQUIRE_EQ_F(state->GetTime(), 0.0f);
        state->SetTime(walk->GetLength() * 10.0f);
        REQUIRE_EQ_F(state->GetTime(), walk->GetLength());
    }

    SECTION("bone weights can be set individually and in bulk")
    {
        Skeleton& skeleton = animated->GetSkeleton();
        const unsigned index = skeleton.GetBoneIndex(bone->name_);
        state->SetBoneWeight(index, 0.25f);
        REQUIRE_NEAR(state->GetBoneWeight(index), 0.25f, 0.001f);

        state->SetBoneWeight(bone->name_, 0.75f);
        REQUIRE_NEAR(state->GetBoneWeight(bone->name_), 0.75f, 0.001f);

        Bone* root = skeleton.GetRootBone();
        const unsigned rootIndex = skeleton.GetBoneIndex(root->name_);
        const Vector<Bone>& bones = skeleton.GetBones();

        unsigned childIndex = M_MAX_UNSIGNED;
        for (unsigned i = 0; i < bones.Size(); ++i)
        {
            if (i != rootIndex && bones[i].parentIndex_ == rootIndex)
            {
                childIndex = i;
                break;
            }
        }
        REQUIRE(childIndex != M_MAX_UNSIGNED);

        state->SetBoneWeight(rootIndex, 0.0f);
        state->SetBoneWeight(childIndex, 0.0f);
        state->SetBoneWeight(root->name_, 1.0f, true);
        REQUIRE_NEAR(state->GetBoneWeight(rootIndex), 1.0f, 0.001f);
        REQUIRE_NEAR(state->GetBoneWeight(childIndex), 1.0f, 0.001f);
    }

    SECTION("two states on the same model both contribute")
    {
        AnimationState* second = animated->AddAnimationState(walk->Clone("SecondWalk"));
        REQUIRE(second != nullptr);
        REQUIRE(animated->GetNumAnimationStates() == 2);
        REQUIRE(animated->GetAnimationState(0u) != animated->GetAnimationState(1u));

        animated->RemoveAllAnimationStates();
        REQUIRE(animated->GetNumAnimationStates() == 0);
    }
}

TEST_CASE("AnimatedModel exposes morphs and bone bounding boxes", "[Graphics]")
{
    Context* context = HeadlessContext();
    SharedPtr<Scene> scene(new Scene(context));
    scene->CreateComponent<Octree>();

    Node* node = scene->CreateChild("Actor");
    auto* animated = node->CreateComponent<AnimatedModel>();
    animated->SetModel(Load<Model>("Models/Kachujin/Kachujin.mdl"));

    REQUIRE(animated->GetNumMorphs() == animated->GetMorphs().Size());
    REQUIRE_EQ_F(animated->GetMorphWeight(animated->GetNumMorphs()), 0.0f);
    REQUIRE_EQ_F(animated->GetMorphWeight(String("NoSuchMorph")), 0.0f);
    REQUIRE(animated->GetMorphsAttr().Size() == animated->GetNumMorphs());

    animated->SetAnimationLodBias(2.0f);
    REQUIRE_EQ_F(animated->GetAnimationLodBias(), 2.0f);

    animated->SetUpdateInvisible(true);
    REQUIRE(animated->GetUpdateInvisible());

    SECTION("the skeleton drives the world bounding box")
    {
        const BoundingBox before = animated->GetWorldBoundingBox();
        REQUIRE(before.Defined());

        node->SetScale(2.0f);
        REQUIRE(animated->GetWorldBoundingBox().Size().y_ > before.Size().y_);
    }

    SECTION("cloning a node copies the animated model and its own skeleton")
    {
        Node* clone = node->Clone();
        auto* clonedModel = clone->GetComponent<AnimatedModel>();
        REQUIRE(clonedModel != nullptr);
        REQUIRE(clonedModel != animated);
        REQUIRE(clonedModel->GetSkeleton().GetNumBones() == animated->GetSkeleton().GetNumBones());

        REQUIRE(clonedModel->GetSkeleton().GetBone(1u)->node_ != animated->GetSkeleton().GetBone(1u)->node_);
    }

    SECTION("the model can be swapped out and cleared")
    {
        animated->SetModel(Load<Model>("Models/Box.mdl"));
        REQUIRE(animated->GetSkeleton().GetNumBones() <= 1);

        animated->SetModel(nullptr);
        REQUIRE(animated->GetModel() == nullptr);
        REQUIRE(animated->GetNumAnimationStates() == 0);
    }
}

TEST_CASE("AnimationController layers, fades, and looks up by name", "[Graphics]")
{
    Context* context = HeadlessContext();
    SharedPtr<Scene> scene(new Scene(context));
    scene->CreateComponent<Octree>();

    Node* node = scene->CreateChild("Actor");
    auto* animated = node->CreateComponent<AnimatedModel>();
    animated->SetModel(Load<Model>("Models/Kachujin/Kachujin.mdl"));
    auto* controller = node->CreateComponent<AnimationController>();

    const String walk("Models/Kachujin/Kachujin_Walk.ani");

    REQUIRE(controller->Play(walk, 1, true, 0.0f));
    REQUIRE(controller->GetLayer(walk) == 1);
    REQUIRE(controller->IsPlaying(1));
    REQUIRE_FALSE(controller->IsPlaying(0));

    controller->SetLayer(walk, 2);
    REQUIRE(controller->GetLayer(walk) == 2);

    controller->SetLooped(walk, false);
    REQUIRE_FALSE(controller->IsLooped(walk));
    controller->SetLooped(walk, true);
    REQUIRE(controller->IsLooped(walk));

    REQUIRE_EQ_F(controller->GetLength(walk), controller->GetAnimationState(walk)->GetLength());
    REQUIRE(controller->GetAnimations().Size() == 1);

    SECTION("fading moves the weight towards a target over time")
    {
        controller->SetWeight(walk, 0.0f);
        REQUIRE(controller->Fade(walk, 1.0f, 1.0f));

        controller->Update(0.5f);
        const float halfway = controller->GetWeight(walk);
        REQUIRE(halfway > 0.0f);
        REQUIRE(halfway < 1.0f);

        controller->Update(1.0f);
        REQUIRE_NEAR(controller->GetWeight(walk), 1.0f, 0.01f);

        REQUIRE_FALSE(controller->Fade("Models/NoSuchAnimation.ani", 1.0f, 1.0f));
    }

    SECTION("fading others leaves the named animation alone")
    {
        REQUIRE(controller->PlayExclusive(walk, 2, true, 0.0f));
        REQUIRE(controller->FadeOthers(walk, 0.0f, 0.0f));
        controller->Update(0.1f);
        REQUIRE(controller->IsPlaying(walk));
    }

    SECTION("the autofade time is remembered")
    {
        controller->SetAutoFade(walk, 0.5f);
        REQUIRE_EQ_F(controller->GetAutoFade(walk), 0.5f);
    }

    SECTION("removal on completion can be requested")
    {
        controller->SetRemoveOnCompletion(walk, true);
        REQUIRE(controller->GetRemoveOnCompletion(walk));
    }

    SECTION("a start bone can be nominated by name")
    {
        Bone* root = animated->GetSkeleton().GetRootBone();
        REQUIRE(controller->SetStartBone(walk, root->name_));
        REQUIRE(controller->GetStartBone(walk) == root);
        REQUIRE(controller->GetStartBoneName(walk) == root->name_);
    }

    SECTION("stopping a whole layer clears just that layer")
    {
        REQUIRE(controller->Play(walk, 2, true, 0.0f));
        controller->StopLayer(2, 0.0f);
        controller->Update(0.0f);
        REQUIRE_FALSE(controller->IsPlaying(2));
    }

    SECTION("a non looped animation reaching its end reports as at end")
    {
        controller->SetLooped(walk, false);
        controller->SetTime(walk, controller->GetLength(walk));
        REQUIRE(controller->IsAtEnd(walk));
    }
}

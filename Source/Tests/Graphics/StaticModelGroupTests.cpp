#include "TestUtils.h"

#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/Drawable.h>
#include <Urho3D/Graphics/Geometry.h>
#include <Urho3D/Graphics/Light.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Graphics/OctreeQuery.h>
#include <Urho3D/Graphics/StaticModelGroup.h>
#include <Urho3D/IO/VectorBuffer.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/Scene.h>

using namespace Urho3D;
using namespace U3DTest;

namespace
{

Model* BoxModel()
{
    return HeadlessContext()->GetSubsystem<ResourceCache>()->GetResource<Model>("Models/Box.mdl");
}

struct GroupFixture
{
    GroupFixture() :
        scene_(new Scene(HeadlessContext()))
    {
        octree_ = scene_->CreateComponent<Octree>();
        group_ = scene_->CreateChild("Group")->CreateComponent<StaticModelGroup>();
        group_->SetModel(BoxModel());
    }

    Node* AddInstance(const Vector3& position)
    {
        Node* node = scene_->CreateChild("Instance");
        node->SetPosition(position);
        group_->AddInstanceNode(node);
        return node;
    }

    SharedPtr<Scene> scene_;
    Octree* octree_{};
    StaticModelGroup* group_{};
};

}

TEST_CASE("StaticModelGroup tracks its instance nodes", "[Graphics]")
{
    GroupFixture fixture;
    StaticModelGroup* group = fixture.group_;

    REQUIRE(group->GetModel() == BoxModel());
    REQUIRE(group->GetNumInstanceNodes() == 0);
    REQUIRE(group->GetInstanceNode(0) == nullptr);

    Node* first = fixture.AddInstance(Vector3::ZERO);
    Node* second = fixture.AddInstance(Vector3(10.0f, 0.0f, 0.0f));

    REQUIRE(group->GetNumInstanceNodes() == 2);
    REQUIRE(group->GetInstanceNode(0) == first);
    REQUIRE(group->GetInstanceNode(1) == second);
    REQUIRE(group->GetInstanceNode(2) == nullptr);

    const BoundingBox bounds = group->GetWorldBoundingBox();
    REQUIRE(bounds.Defined());
    REQUIRE(bounds.min_.x_ < 0.0f);
    REQUIRE(bounds.max_.x_ > 9.0f);

    SECTION("adding the same node twice is ignored")
    {
        group->AddInstanceNode(first);
        REQUIRE(group->GetNumInstanceNodes() == 2);
    }

    SECTION("a null node is ignored")
    {
        group->AddInstanceNode(nullptr);
        REQUIRE(group->GetNumInstanceNodes() == 2);
    }

    SECTION("removing a node shrinks the group and its bounds")
    {
        group->RemoveInstanceNode(second);
        REQUIRE(group->GetNumInstanceNodes() == 1);
        REQUIRE(group->GetInstanceNode(0) == first);
        REQUIRE(group->GetWorldBoundingBox().max_.x_ < 9.0f);

        group->RemoveInstanceNode(second);
        REQUIRE(group->GetNumInstanceNodes() == 1);
        group->RemoveInstanceNode(nullptr);
        REQUIRE(group->GetNumInstanceNodes() == 1);
    }

    SECTION("clearing removes every instance")
    {
        group->RemoveAllInstanceNodes();
        REQUIRE(group->GetNumInstanceNodes() == 0);
        REQUIRE(group->GetInstanceNode(0) == nullptr);
    }

    SECTION("deleting an instance node leaves an expired slot behind")
    {
        second->Remove();
        REQUIRE(group->GetNumInstanceNodes() == 2);
        REQUIRE(group->GetInstanceNode(0) == first);
        REQUIRE(group->GetInstanceNode(1) == nullptr);

        REQUIRE(group->GetWorldBoundingBox().max_.x_ < 9.0f);
    }

    SECTION("moving an instance moves the group bounds")
    {
        second->SetPosition(Vector3(100.0f, 0.0f, 0.0f));
        REQUIRE(group->GetWorldBoundingBox().max_.x_ > 99.0f);
    }

    SECTION("the instances round trip through the node id attribute")
    {
        const VariantVector ids = group->GetNodeIDsAttr();
        REQUIRE(ids.Size() >= 2);

        group->RemoveAllInstanceNodes();
        REQUIRE(group->GetNumInstanceNodes() == 0);

        group->SetNodeIDsAttr(ids);
        REQUIRE(group->GetNumInstanceNodes() == 0);

        group->ApplyAttributes();
        REQUIRE(group->GetNumInstanceNodes() == 2);
        REQUIRE(group->GetInstanceNode(0) == first);
    }

    SECTION("a raycast hits an instance away from the group's own node")
    {
        FrameInfo frame{};
        fixture.octree_->Update(frame);

        PODVector<RayQueryResult> results;
        RayOctreeQuery query(results, Ray(Vector3(10.0f, 0.0f, -20.0f), Vector3::FORWARD),
            RAY_TRIANGLE, 100.0f);
        fixture.octree_->Raycast(query);

        REQUIRE_FALSE(results.Empty());
        REQUIRE(results[0].drawable_ == group);
    }

    SECTION("clearing the model leaves the instances in place")
    {
        group->SetModel(nullptr);
        REQUIRE(group->GetModel() == nullptr);
        REQUIRE(group->GetNumInstanceNodes() == 2);
    }
}

TEST_CASE("Model saves and reloads its geometry", "[Graphics]")
{
    Context* context = HeadlessContext();
    Model* source = BoxModel();
    REQUIRE(source != nullptr);

    VectorBuffer buffer;
    REQUIRE(source->Save(buffer));
    REQUIRE(buffer.GetSize() > 0);

    buffer.Seek(0);
    SharedPtr<Model> restored(new Model(context));
    REQUIRE(restored->Load(buffer));

    REQUIRE(restored->GetNumGeometries() == source->GetNumGeometries());
    REQUIRE(restored->GetBoundingBox() == source->GetBoundingBox());
    REQUIRE(restored->GetVertexBuffers().Size() == source->GetVertexBuffers().Size());
    REQUIRE(restored->GetIndexBuffers().Size() == source->GetIndexBuffers().Size());

    Geometry* geometry = restored->GetGeometry(0, 0);
    REQUIRE(geometry != nullptr);
    REQUIRE(geometry->GetVertexCount() == source->GetGeometry(0, 0)->GetVertexCount());
    REQUIRE(geometry->GetIndexCount() == source->GetGeometry(0, 0)->GetIndexCount());

    SECTION("a skinned model keeps its skeleton across the round trip")
    {
        auto* skinned = HeadlessContext()->GetSubsystem<ResourceCache>()
            ->GetResource<Model>("Models/Kachujin/Kachujin.mdl");
        REQUIRE(skinned != nullptr);

        VectorBuffer skinnedBuffer;
        REQUIRE(skinned->Save(skinnedBuffer));
        skinnedBuffer.Seek(0);

        SharedPtr<Model> reloaded(new Model(context));
        REQUIRE(reloaded->Load(skinnedBuffer));
        REQUIRE(reloaded->GetSkeleton().GetNumBones() == skinned->GetSkeleton().GetNumBones());
        REQUIRE(reloaded->GetSkeleton().GetRootBone()->name_ == skinned->GetSkeleton().GetRootBone()->name_);
    }

    SECTION("the geometry lod distances are kept")
    {
        REQUIRE(restored->GetNumGeometryLodLevels(0) == source->GetNumGeometryLodLevels(0));
        REQUIRE(restored->GetNumGeometryLodLevels(99) == 0);

        REQUIRE(restored->GetGeometry(0, 99) == restored->GetGeometry(0, restored->GetNumGeometryLodLevels(0) - 1));
        REQUIRE(restored->GetGeometry(99, 0) == nullptr);
    }

    SECTION("rubbish is refused")
    {
        const String rubbish("not a model at all");
        VectorBuffer bad(rubbish.CString(), rubbish.Length());
        SharedPtr<Model> broken(new Model(context));
        REQUIRE_FALSE(broken->Load(bad));
    }
}

TEST_CASE("Light builds its shadow split and projection data", "[Graphics]")
{
    SharedPtr<Scene> scene(new Scene(HeadlessContext()));
    scene->CreateComponent<Octree>();

    Node* cameraNode = scene->CreateChild("Camera");
    cameraNode->SetPosition(Vector3(0.0f, 0.0f, -10.0f));
    auto* camera = cameraNode->CreateComponent<Camera>();
    camera->SetAspectRatio(1.0f);
    camera->SetFarClip(100.0f);

    Node* lightNode = scene->CreateChild("Light");
    auto* light = lightNode->CreateComponent<Light>();

    SECTION("cascade parameters are stored and clamped")
    {
        CascadeParameters cascade(10.0f, 20.0f, 40.0f, 80.0f, 0.9f);
        light->SetShadowCascade(cascade);
        REQUIRE_EQ_F(light->GetShadowCascade().splits_[0], 10.0f);
        REQUIRE_EQ_F(light->GetShadowCascade().splits_[3], 80.0f);
        REQUIRE_EQ_F(light->GetShadowCascade().fadeStart_, 0.9f);

        REQUIRE_EQ_F(cascade.GetShadowRange(), 80.0f);
    }

    SECTION("focus parameters round trip")
    {
        FocusParameters focus(true, true, true, 2.0f, 4.0f);
        light->SetShadowFocus(focus);
        REQUIRE(light->GetShadowFocus().focus_);
        REQUIRE(light->GetShadowFocus().nonUniform_);
        REQUIRE_EQ_F(light->GetShadowFocus().quantize_, 2.0f);

        FocusParameters clamped(true, true, true, 0.0f, 0.0f);
        clamped.Validate();
        REQUIRE(clamped.quantize_ > 0.0f);
        REQUIRE(clamped.minView_ > 0.0f);
    }

    SECTION("the view space frustum is the world frustum seen from the camera")
    {
        light->SetLightType(LIGHT_SPOT);
        light->SetRange(50.0f);
        light->SetFov(60.0f);
        lightNode->SetPosition(Vector3(3.0f, 5.0f, -2.0f));
        lightNode->SetRotation(Quaternion(25.0f, Vector3::UP));

        const Frustum world = light->GetFrustum();
        const Frustum viewSpace = light->GetViewSpaceFrustum(camera->GetView());

        for (unsigned i = 0; i < NUM_FRUSTUM_VERTICES; ++i)
            REQUIRE_NEAR((camera->GetView() * world.vertices_[i] - viewSpace.vertices_[i]).Length(), 0.0f, 0.001f);
    }

    SECTION("a spot light exposes the matrix used to project its texture")
    {
        light->SetLightType(LIGHT_SPOT);
        light->SetRange(50.0f);
        light->SetFov(60.0f);

        const Matrix3x4 projection = light->GetFullscreenQuadTransform(camera);
        REQUIRE_FALSE(projection.Equals(Matrix3x4::ZERO));
    }

    SECTION("the per vertex flag and light mask round trip")
    {
        light->SetPerVertex(true);
        REQUIRE(light->GetPerVertex());
        light->SetPerVertex(false);

        light->SetLightMask(0x7);
        REQUIRE(light->GetLightMask() == 0x7);
    }

    SECTION("a light with zero brightness still reports its colour")
    {
        light->SetColor(Color::RED);
        light->SetBrightness(0.0f);
        REQUIRE(light->GetColor() == Color::RED);
        REQUIRE(light->GetEffectiveColor().r_ <= Color::RED.r_);
        REQUIRE_EQ_F(light->GetEffectiveSpecularIntensity(), 0.0f);
    }
}

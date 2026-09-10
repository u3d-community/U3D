#include "TestUtils.h"

#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Math/Frustum.h>
#include <Urho3D/Scene/Node.h>
#include <Urho3D/Scene/Scene.h>

using namespace Urho3D;
using namespace U3DTest;

namespace
{

Camera* MakeCamera(SharedPtr<Scene>& scene, Node*& node)
{
    scene = new Scene(HeadlessContext());
    node = scene->CreateChild("Camera");
    auto* camera = node->CreateComponent<Camera>();
    camera->SetAspectRatio(1.0f);
    return camera;
}

}

TEST_CASE("Camera clamps and reports its lens settings", "[Graphics]")
{
    SharedPtr<Scene> scene;
    Node* node = nullptr;
    Camera* camera = MakeCamera(scene, node);

    REQUIRE_EQ_F(camera->GetFov(), DEFAULT_CAMERA_FOV);
    REQUIRE_EQ_F(camera->GetNearClip(), DEFAULT_NEARCLIP);
    REQUIRE_EQ_F(camera->GetFarClip(), DEFAULT_FARCLIP);
    REQUIRE_FALSE(camera->IsOrthographic());

    camera->SetFov(500.0f);
    REQUIRE(camera->GetFov() <= 180.0f);
    camera->SetFov(-10.0f);
    REQUIRE(camera->GetFov() >= 0.0f);
    camera->SetFov(60.0f);
    REQUIRE_EQ_F(camera->GetFov(), 60.0f);

    camera->SetNearClip(-5.0f);
    REQUIRE(camera->GetNearClip() > 0.0f);

    camera->SetFarClip(-5.0f);
    REQUIRE(camera->GetFarClip() >= 0.0f);

    camera->SetNearClip(1.0f);
    camera->SetFarClip(100.0f);
    REQUIRE_EQ_F(camera->GetNearClip(), 1.0f);
    REQUIRE_EQ_F(camera->GetFarClip(), 100.0f);

    camera->SetZoom(0.0f);
    REQUIRE(camera->GetZoom() > 0.0f);
    camera->SetZoom(2.0f);
    REQUIRE_EQ_F(camera->GetZoom(), 2.0f);
    camera->SetZoom(1.0f);

    camera->SetLodBias(0.0f);
    REQUIRE(camera->GetLodBias() > 0.0f);

    camera->SetViewMask(0x5);
    REQUIRE(camera->GetViewMask() == 0x5);

    camera->SetFillMode(FILL_WIREFRAME);
    REQUIRE(camera->GetFillMode() == FILL_WIREFRAME);

    REQUIRE(camera->IsProjectionValid());

    SECTION("a far clip inside the near clip makes the projection invalid")
    {
        camera->SetNearClip(10.0f);
        camera->SetFarClip(5.0f);
        REQUIRE_FALSE(camera->IsProjectionValid());
    }
}

TEST_CASE("Camera builds a perspective frustum around its node", "[Graphics]")
{
    SharedPtr<Scene> scene;
    Node* node = nullptr;
    Camera* camera = MakeCamera(scene, node);
    camera->SetFov(90.0f);
    camera->SetNearClip(1.0f);
    camera->SetFarClip(100.0f);

    const Frustum& frustum = camera->GetFrustum();

    REQUIRE(frustum.IsInside(Vector3(0.0f, 0.0f, 50.0f)) != OUTSIDE);
    REQUIRE(frustum.IsInside(Vector3(0.0f, 0.0f, -50.0f)) == OUTSIDE);
    REQUIRE(frustum.IsInside(Vector3(0.0f, 0.0f, 500.0f)) == OUTSIDE);

    SECTION("moving the node moves the frustum with it")
    {
        node->SetPosition(Vector3(0.0f, 0.0f, 100.0f));
        REQUIRE(camera->GetFrustum().IsInside(Vector3(0.0f, 0.0f, 150.0f)) != OUTSIDE);
        REQUIRE(camera->GetFrustum().IsInside(Vector3(0.0f, 0.0f, 50.0f)) == OUTSIDE);
    }

    SECTION("rotating the node turns the frustum")
    {
        node->SetRotation(Quaternion(180.0f, Vector3::UP));
        REQUIRE(camera->GetFrustum().IsInside(Vector3(0.0f, 0.0f, -50.0f)) != OUTSIDE);
        REQUIRE(camera->GetFrustum().IsInside(Vector3(0.0f, 0.0f, 50.0f)) == OUTSIDE);
    }

    SECTION("the frustum size grows towards the far plane")
    {
        Vector3 nearSize;
        Vector3 farSize;
        camera->GetFrustumSize(nearSize, farSize);
        REQUIRE(farSize.x_ > nearSize.x_);
        REQUIRE(farSize.y_ > nearSize.y_);
        REQUIRE_EQ_F(nearSize.z_, camera->GetNearClip());
        REQUIRE_EQ_F(farSize.z_, camera->GetFarClip());
    }

    SECTION("a split frustum covers only the requested depth range")
    {
        const Frustum split = camera->GetSplitFrustum(10.0f, 20.0f);
        REQUIRE(split.IsInside(Vector3(0.0f, 0.0f, 15.0f)) != OUTSIDE);
        REQUIRE(split.IsInside(Vector3(0.0f, 0.0f, 5.0f)) == OUTSIDE);
        REQUIRE(split.IsInside(Vector3(0.0f, 0.0f, 25.0f)) == OUTSIDE);
    }

    SECTION("the view space frustum sits at the origin regardless of the node")
    {
        node->SetPosition(Vector3(1000.0f, 0.0f, 0.0f));
        const Frustum viewSpace = camera->GetViewSpaceFrustum();
        REQUIRE(viewSpace.IsInside(Vector3(0.0f, 0.0f, 50.0f)) != OUTSIDE);
    }
}

TEST_CASE("Camera projects between world and screen space", "[Graphics]")
{
    SharedPtr<Scene> scene;
    Node* node = nullptr;
    Camera* camera = MakeCamera(scene, node);
    camera->SetFov(90.0f);
    camera->SetNearClip(1.0f);
    camera->SetFarClip(100.0f);

    const Vector2 centre = camera->WorldToScreenPoint(Vector3(0.0f, 0.0f, 10.0f));
    REQUIRE_NEAR(centre.x_, 0.5f, 0.001f);
    REQUIRE_NEAR(centre.y_, 0.5f, 0.001f);

    REQUIRE(camera->WorldToScreenPoint(Vector3(0.0f, 5.0f, 10.0f)).y_ < 0.5f);
    REQUIRE(camera->WorldToScreenPoint(Vector3(5.0f, 0.0f, 10.0f)).x_ > 0.5f);

    SECTION("a screen ray from the centre starts on the near plane and points straight ahead")
    {
        const Ray ray = camera->GetScreenRay(0.5f, 0.5f);
        REQUIRE(ray.direction_.Equals(Vector3::FORWARD));
        REQUIRE(ray.origin_.Equals(node->GetWorldPosition() + Vector3::FORWARD * camera->GetNearClip()));
    }

    SECTION("screen rays away from the centre lean the right way")
    {
        REQUIRE(camera->GetScreenRay(1.0f, 0.5f).direction_.x_ > 0.0f);
        REQUIRE(camera->GetScreenRay(0.0f, 0.5f).direction_.x_ < 0.0f);
        REQUIRE(camera->GetScreenRay(0.5f, 0.0f).direction_.y_ > 0.0f);
        REQUIRE(camera->GetScreenRay(0.5f, 1.0f).direction_.y_ < 0.0f);
    }

    SECTION("the screen to world round trip returns the original point")
    {
        const Vector3 world(2.0f, -3.0f, 20.0f);
        const Vector2 screen = camera->WorldToScreenPoint(world);
        const Vector3 restored = camera->ScreenToWorldPoint(Vector3(screen.x_, screen.y_, world.z_));
        REQUIRE_NEAR((restored - world).Length(), 0.0f, 0.01f);
    }

    SECTION("distance is measured along the view direction")
    {
        REQUIRE_NEAR(camera->GetDistance(Vector3(0.0f, 0.0f, 10.0f)), 10.0f, 0.001f);
        REQUIRE_NEAR(camera->GetDistanceSquared(Vector3(0.0f, 0.0f, 10.0f)), 100.0f, 0.01f);
    }
}

TEST_CASE("An orthographic camera keeps its size independent of depth", "[Graphics]")
{
    SharedPtr<Scene> scene;
    Node* node = nullptr;
    Camera* camera = MakeCamera(scene, node);

    camera->SetOrthographic(true);
    camera->SetOrthoSize(20.0f);
    camera->SetNearClip(0.0f);
    camera->SetFarClip(100.0f);

    REQUIRE(camera->IsOrthographic());
    REQUIRE_EQ_F(camera->GetOrthoSize(), 20.0f);
    REQUIRE_EQ_F(camera->GetNearClip(), 0.0f);

    Vector3 nearSize;
    Vector3 farSize;
    camera->GetFrustumSize(nearSize, farSize);
    REQUIRE_NEAR(nearSize.x_, farSize.x_, 0.001f);
    REQUIRE_NEAR(nearSize.y_, farSize.y_, 0.001f);

    SECTION("the same world offset maps to the same screen offset at any depth")
    {
        const Vector2 nearPoint = camera->WorldToScreenPoint(Vector3(5.0f, 0.0f, 10.0f));
        const Vector2 farPoint = camera->WorldToScreenPoint(Vector3(5.0f, 0.0f, 90.0f));
        REQUIRE_NEAR(nearPoint.x_, farPoint.x_, 0.001f);
    }

    SECTION("every screen ray runs parallel to the view direction")
    {
        REQUIRE(camera->GetScreenRay(0.0f, 0.0f).direction_.Equals(Vector3::FORWARD));
        REQUIRE(camera->GetScreenRay(1.0f, 1.0f).direction_.Equals(Vector3::FORWARD));
        REQUIRE_FALSE(camera->GetScreenRay(0.0f, 0.0f).origin_.Equals(camera->GetScreenRay(1.0f, 1.0f).origin_));
    }

    SECTION("a non square ortho size is taken as width and height")
    {
        camera->SetOrthoSize(Vector2(40.0f, 10.0f));
        REQUIRE_EQ_F(camera->GetAspectRatio(), 4.0f);
        REQUIRE_FALSE(camera->GetAutoAspectRatio());
    }
}

TEST_CASE("Camera reflection and clipping planes alter the view", "[Graphics]")
{
    SharedPtr<Scene> scene;
    Node* node = nullptr;
    Camera* camera = MakeCamera(scene, node);
    node->SetPosition(Vector3(0.0f, 10.0f, 0.0f));

    const Matrix3x4 plainView = camera->GetView();

    const Plane water(Vector3::UP, Vector3::ZERO);
    camera->SetReflectionPlane(water);
    camera->SetUseReflection(true);

    REQUIRE(camera->GetUseReflection());
    REQUIRE(camera->GetReflectionPlane().normal_.Equals(water.normal_));
    REQUIRE_FALSE(camera->GetView().Equals(plainView));

    REQUIRE_NEAR(camera->GetEffectiveWorldTransform().Translation().y_, -10.0f, 0.001f);
    REQUIRE(camera->GetReverseCulling());

    camera->SetUseReflection(false);
    REQUIRE(camera->GetView().Equals(plainView));

    SECTION("the clip plane is remembered and can be turned off again")
    {
        const Plane clip(Vector3::FORWARD, Vector3(0.0f, 0.0f, 5.0f));
        camera->SetClipPlane(clip);
        camera->SetUseClipping(true);
        REQUIRE(camera->GetUseClipping());
        REQUIRE(camera->GetClipPlane().normal_.Equals(clip.normal_));
        camera->SetUseClipping(false);
        REQUIRE_FALSE(camera->GetUseClipping());
    }

    SECTION("flipping vertically reverses the culling order on its own")
    {
        REQUIRE_FALSE(camera->GetReverseCulling());
        camera->SetFlipVertical(true);
        REQUIRE(camera->GetFlipVertical());
        REQUIRE(camera->GetReverseCulling());
        camera->SetFlipVertical(false);
    }

    SECTION("a projection offset shifts the projected point")
    {
        const Vector2 before = camera->WorldToScreenPoint(Vector3(0.0f, 10.0f, 10.0f));
        camera->SetProjectionOffset(Vector2(0.25f, 0.0f));
        REQUIRE(camera->GetProjectionOffset() == Vector2(0.25f, 0.0f));
        REQUIRE(camera->WorldToScreenPoint(Vector3(0.0f, 10.0f, 10.0f)).x_ != before.x_);
    }
}

TEST_CASE("Camera face camera rotations turn towards the viewer", "[Graphics]")
{
    SharedPtr<Scene> scene;
    Node* node = nullptr;
    Camera* camera = MakeCamera(scene, node);
    node->SetPosition(Vector3(0.0f, 0.0f, -10.0f));

    const Quaternion identity = Quaternion::IDENTITY;
    const Vector3 target(0.0f, 0.0f, 10.0f);

    REQUIRE(camera->GetFaceCameraRotation(target, identity, FC_NONE).Equals(identity));

    node->SetRotation(Quaternion(30.0f, Vector3::UP));
    REQUIRE(camera->GetFaceCameraRotation(target, identity, FC_ROTATE_XYZ).Equals(node->GetWorldRotation()));

    const Quaternion rotateY = camera->GetFaceCameraRotation(target, identity, FC_ROTATE_Y);
    REQUIRE_NEAR(rotateY.EulerAngles().y_, 30.0f, 0.01f);
    REQUIRE_NEAR(rotateY.EulerAngles().x_, 0.0f, 0.01f);

    SECTION("look at aligns with the vector from the camera to the target")
    {
        node->SetRotation(Quaternion::IDENTITY);
        node->SetPosition(Vector3(10.0f, 0.0f, 0.0f));

        const Quaternion lookAtXYZ = camera->GetFaceCameraRotation(Vector3::ZERO, identity, FC_LOOKAT_XYZ);
        REQUIRE((lookAtXYZ * Vector3::FORWARD).Normalized().Equals(Vector3(-1.0f, 0.0f, 0.0f)));

        const Quaternion lookAtY = camera->GetFaceCameraRotation(Vector3(0.0f, 50.0f, 0.0f), identity, FC_LOOKAT_Y);
        REQUIRE_NEAR((lookAtY * Vector3::UP).Normalized().y_, 1.0f, 0.01f);
    }

    SECTION("mixed mode matches the Y only form until a minimum angle is asked for")
    {
        node->SetRotation(Quaternion::IDENTITY);
        const Quaternion yOnly = camera->GetFaceCameraRotation(target, identity, FC_LOOKAT_Y);
        REQUIRE(camera->GetFaceCameraRotation(target, identity, FC_LOOKAT_MIXED, 0.0f).Equals(yOnly));

        const Quaternion tilted = camera->GetFaceCameraRotation(target, identity, FC_LOOKAT_MIXED, 120.0f);
        REQUIRE_FALSE(tilted.Equals(yOnly));
        REQUIRE_NEAR(tilted.EulerAngles().x_, 30.0f, 0.01f);
    }
}

TEST_CASE("An explicitly set projection overrides the lens parameters", "[Graphics]")
{
    SharedPtr<Scene> scene;
    Node* node = nullptr;
    Camera* camera = MakeCamera(scene, node);

    const Matrix4 custom = Matrix4::IDENTITY;
    camera->SetProjection(custom);
    REQUIRE(camera->GetProjection().Equals(custom));
    REQUIRE_FALSE(camera->GetAutoAspectRatio());

    camera->SetFov(30.0f);
    REQUIRE_FALSE(camera->GetProjection().Equals(custom));
    REQUIRE_EQ_F(camera->GetFov(), 30.0f);
}

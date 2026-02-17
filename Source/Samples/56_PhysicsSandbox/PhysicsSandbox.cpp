//
// Copyright (c) 2008-2022 the Urho3D project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Engine/Engine.h>
#include <Urho3D/Engine/EngineDefs.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/DebugRenderer.h>
#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Graphics/Light.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Graphics/StaticModel.h>
#include <Urho3D/Graphics/Zone.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/Input/InputEvents.h>
#include <Urho3D/Physics/CollisionShape.h>
#include <Urho3D/Physics/PhysicsWorld.h>
#include <Urho3D/Physics/RigidBody.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/UI/Font.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/UI/UI.h>

#include "PhysicsSandbox.h"

#include <Urho3D/DebugNew.h>

static const float FLICK_FORCE_MULTIPLIER = 0.08f;
static const float SETTLE_VELOCITY_THRESHOLD = 0.3f;
static const float SETTLE_TIME_REQUIRED = 2.0f;
static const Vector3 CAMERA_OFFSET(0.0f, 3.0f, -7.0f);
static const float CAMERA_LERP_SPEED = 3.0f;

URHO3D_DEFINE_APPLICATION_MAIN(PhysicsSandbox)

PhysicsSandbox::PhysicsSandbox(Context* context) :
    Sample(context),
    sphereNode_(nullptr),
    flickActive_(false),
    launched_(false),
    settleTimer_(0.0f),
    drawDebug_(false),
    instructionText_(nullptr),
    initialCameraPos_(0.0f, 4.0f, -6.0f),
    initialCameraTarget_(0.0f, 1.0f, 5.0f)
{
}

void PhysicsSandbox::Setup()
{
    Sample::Setup();
    engineParameters_[EP_WINDOW_WIDTH] = 450;
    engineParameters_[EP_WINDOW_HEIGHT] = 800;
}

void PhysicsSandbox::Start()
{
    Sample::Start();

    CreateScene();
    CreateInstructions();
    SetupViewport();
    SubscribeToEvents();

    // Use free mouse mode so the cursor is visible for click-drag flicking
    Sample::InitMouseMode(MM_FREE);
}

void PhysicsSandbox::CreateScene()
{
    auto* cache = GetSubsystem<ResourceCache>();

    scene_ = new Scene(context_);
    scene_->CreateComponent<Octree>();
    scene_->CreateComponent<PhysicsWorld>();
    scene_->CreateComponent<DebugRenderer>();

    // Zone for ambient lighting
    Node* zoneNode = scene_->CreateChild("Zone");
    auto* zone = zoneNode->CreateComponent<Zone>();
    zone->SetBoundingBox(BoundingBox(-1000.0f, 1000.0f));
    zone->SetAmbientColor(Color(0.2f, 0.2f, 0.2f));
    zone->SetFogColor(Color(0.7f, 0.8f, 1.0f));
    zone->SetFogStart(200.0f);
    zone->SetFogEnd(400.0f);

    // Directional light with shadows
    Node* lightNode = scene_->CreateChild("DirectionalLight");
    lightNode->SetDirection(Vector3(0.6f, -1.0f, 0.8f));
    auto* light = lightNode->CreateComponent<Light>();
    light->SetLightType(LIGHT_DIRECTIONAL);
    light->SetCastShadows(true);
    light->SetShadowBias(BiasParameters(0.00025f, 0.5f));
    light->SetShadowCascade(CascadeParameters(10.0f, 50.0f, 200.0f, 0.0f, 0.8f));

    // Ground plane
    {
        Node* floorNode = scene_->CreateChild("Floor");
        floorNode->SetPosition(Vector3(0.0f, -0.5f, 0.0f));
        floorNode->SetScale(Vector3(100.0f, 1.0f, 100.0f));
        auto* floorObject = floorNode->CreateComponent<StaticModel>();
        floorObject->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
        floorObject->SetMaterial(cache->GetResource<Material>("Materials/StoneTiled.xml"));

        floorNode->CreateComponent<RigidBody>();
        auto* shape = floorNode->CreateComponent<CollisionShape>();
        shape->SetBox(Vector3::ONE);
    }

    // Sphere (the flick-able ball)
    {
        sphereNode_ = scene_->CreateChild("Sphere");
        sphereNode_->SetPosition(Vector3(0.0f, 0.5f, 0.0f));
        auto* sphereObject = sphereNode_->CreateComponent<StaticModel>();
        sphereObject->SetModel(cache->GetResource<Model>("Models/Sphere.mdl"));
        sphereObject->SetMaterial(cache->GetResource<Material>("Materials/StoneSmall.xml"));
        sphereObject->SetCastShadows(true);

        auto* body = sphereNode_->CreateComponent<RigidBody>();
        body->SetMass(2.0f);
        body->SetFriction(0.5f);
        auto* shape = sphereNode_->CreateComponent<CollisionShape>();
        shape->SetSphere(1.0f);
    }

    // Tower of boxes: 4 wide x 6 high, centered at Z=10
    {
        const int towerWidth = 4;
        const int towerHeight = 6;
        const float towerZ = 10.0f;
        const float boxSize = 1.0f;
        float startX = -(towerWidth - 1) * boxSize * 0.5f;

        for (int y = 0; y < towerHeight; ++y)
        {
            for (int x = 0; x < towerWidth; ++x)
            {
                Node* boxNode = scene_->CreateChild("TowerBox");
                boxNode->SetPosition(Vector3(
                    startX + x * boxSize,
                    y * boxSize + boxSize * 0.5f,
                    towerZ));
                auto* boxObject = boxNode->CreateComponent<StaticModel>();
                boxObject->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
                boxObject->SetMaterial(cache->GetResource<Material>("Materials/StoneEnvMapSmall.xml"));
                boxObject->SetCastShadows(true);

                auto* body = boxNode->CreateComponent<RigidBody>();
                body->SetMass(0.5f);
                body->SetFriction(0.75f);
                auto* shape = boxNode->CreateComponent<CollisionShape>();
                shape->SetBox(Vector3::ONE);
            }
        }
    }

    // Camera node (outside scene, unaffected by scene load/save)
    cameraNode_ = new Node(context_);
    auto* camera = cameraNode_->CreateComponent<Camera>();
    camera->SetFarClip(500.0f);
    camera->SetAutoAspectRatio(true);
    cameraNode_->SetPosition(initialCameraPos_);
    cameraNode_->LookAt(initialCameraTarget_);

    // Reset state
    flickActive_ = false;
    launched_ = false;
    settleTimer_ = 0.0f;
}

void PhysicsSandbox::CreateInstructions()
{
    auto* cache = GetSubsystem<ResourceCache>();
    auto* ui = GetSubsystem<UI>();

    instructionText_ = ui->GetRoot()->CreateChild<Text>();
    instructionText_->SetText("Flick the ball to knock down the tower!");
    instructionText_->SetFont(cache->GetResource<Font>("Fonts/Anonymous Pro.ttf"), 15);
    instructionText_->SetTextAlignment(HA_CENTER);
    instructionText_->SetHorizontalAlignment(HA_CENTER);
    instructionText_->SetVerticalAlignment(VA_CENTER);
    instructionText_->SetPosition(0, ui->GetRoot()->GetHeight() / 4);
}

void PhysicsSandbox::SetupViewport()
{
    auto* renderer = GetSubsystem<Renderer>();
    SharedPtr<Viewport> viewport(new Viewport(context_, scene_, cameraNode_->GetComponent<Camera>()));
    renderer->SetViewport(0, viewport);
}

void PhysicsSandbox::SubscribeToEvents()
{
    SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(PhysicsSandbox, HandleUpdate));
    SubscribeToEvent(E_POSTUPDATE, URHO3D_HANDLER(PhysicsSandbox, HandlePostUpdate));
    SubscribeToEvent(E_MOUSEBUTTONDOWN, URHO3D_HANDLER(PhysicsSandbox, HandleMouseButtonDown));
    SubscribeToEvent(E_MOUSEBUTTONUP, URHO3D_HANDLER(PhysicsSandbox, HandleMouseButtonUp));
    SubscribeToEvent(E_TOUCHBEGIN, URHO3D_HANDLER(PhysicsSandbox, HandleTouchBegin));
    SubscribeToEvent(E_TOUCHEND, URHO3D_HANDLER(PhysicsSandbox, HandleTouchEnd));
    SubscribeToEvent(E_POSTRENDERUPDATE, URHO3D_HANDLER(PhysicsSandbox, HandlePostRenderUpdate));
}

void PhysicsSandbox::HandlePress(int screenX, int screenY)
{
    if (launched_)
        return;

    auto* graphics = GetSubsystem<Graphics>();
    auto* camera = cameraNode_->GetComponent<Camera>();

    // Normalized screen coordinates
    float normX = (float)screenX / (float)graphics->GetWidth();
    float normY = (float)screenY / (float)graphics->GetHeight();

    Ray ray = camera->GetScreenRay(normX, normY);

    // Raycast against physics world
    PhysicsRaycastResult result;
    scene_->GetComponent<PhysicsWorld>()->RaycastSingle(result, ray, 100.0f);

    if (result.body_ && result.body_->GetNode() == sphereNode_)
    {
        flickStart_ = IntVector2(screenX, screenY);
        hitPoint_ = result.position_;
        flickActive_ = true;
    }
}

void PhysicsSandbox::HandleRelease(int screenX, int screenY)
{
    if (!flickActive_)
        return;

    flickActive_ = false;

    auto* graphics = GetSubsystem<Graphics>();
    float dx = (float)(screenX - flickStart_.x_);
    float dy = (float)(screenY - flickStart_.y_);

    // Screen Y-up is negative in screen coords, so negate dy
    // Map screen delta to world impulse: X stays X, screen-up maps to Z-forward + some Y-up
    Vector3 impulseDir(dx, -dy * 0.3f, -dy);
    float magnitude = impulseDir.Length();
    if (magnitude < 1.0f)
        return;

    impulseDir /= magnitude;
    float impulseStrength = magnitude * FLICK_FORCE_MULTIPLIER;

    // Convert hit point to local space for ApplyImpulse
    Vector3 localHit = hitPoint_ - sphereNode_->GetWorldPosition();

    auto* body = sphereNode_->GetComponent<RigidBody>();
    body->Activate();
    body->ApplyImpulse(impulseDir * impulseStrength, localHit);

    launched_ = true;

    // Hide instructions
    if (instructionText_)
        instructionText_->SetVisible(false);
}

void PhysicsSandbox::HandleMouseButtonDown(StringHash /*eventType*/, VariantMap& eventData)
{
    using namespace MouseButtonDown;
    if (eventData[P_BUTTON].GetInt() != MOUSEB_LEFT)
        return;

    auto* input = GetSubsystem<Input>();
    IntVector2 pos = input->GetMousePosition();
    HandlePress(pos.x_, pos.y_);
}

void PhysicsSandbox::HandleMouseButtonUp(StringHash /*eventType*/, VariantMap& eventData)
{
    using namespace MouseButtonUp;
    if (eventData[P_BUTTON].GetInt() != MOUSEB_LEFT)
        return;

    auto* input = GetSubsystem<Input>();
    IntVector2 pos = input->GetMousePosition();
    HandleRelease(pos.x_, pos.y_);
}

void PhysicsSandbox::HandleTouchBegin(StringHash /*eventType*/, VariantMap& eventData)
{
    using namespace TouchBegin;
    HandlePress(eventData[P_X].GetInt(), eventData[P_Y].GetInt());
}

void PhysicsSandbox::HandleTouchEnd(StringHash /*eventType*/, VariantMap& eventData)
{
    using namespace TouchEnd;
    HandleRelease(eventData[P_X].GetInt(), eventData[P_Y].GetInt());
}

void PhysicsSandbox::HandleUpdate(StringHash /*eventType*/, VariantMap& eventData)
{
    using namespace Update;
    float timeStep = eventData[P_TIMESTEP].GetFloat();

    // Toggle debug geometry with Space
    auto* input = GetSubsystem<Input>();
    if (input->GetKeyPress(KEY_SPACE))
        drawDebug_ = !drawDebug_;

    // Check if sphere has settled after launch
    if (launched_ && sphereNode_)
    {
        auto* body = sphereNode_->GetComponent<RigidBody>();
        if (body->GetLinearVelocity().Length() < SETTLE_VELOCITY_THRESHOLD)
        {
            settleTimer_ += timeStep;
            if (settleTimer_ >= SETTLE_TIME_REQUIRED)
            {
                // Reset: recreate the scene
                CreateScene();
                SetupViewport();
                if (instructionText_)
                    instructionText_->SetVisible(true);
            }
        }
        else
        {
            settleTimer_ = 0.0f;
        }
    }
}

void PhysicsSandbox::HandlePostUpdate(StringHash /*eventType*/, VariantMap& eventData)
{
    using namespace PostUpdate;
    float timeStep = eventData[P_TIMESTEP].GetFloat();

    if (!cameraNode_ || !sphereNode_)
        return;

    if (launched_)
    {
        // Smoothly follow the sphere
        Vector3 spherePos = sphereNode_->GetWorldPosition();
        Vector3 targetCameraPos = spherePos + CAMERA_OFFSET;
        Vector3 currentPos = cameraNode_->GetPosition();

        float t = 1.0f - expf(-CAMERA_LERP_SPEED * timeStep);
        cameraNode_->SetPosition(currentPos.Lerp(targetCameraPos, t));
        cameraNode_->LookAt(spherePos);
    }
}

void PhysicsSandbox::HandlePostRenderUpdate(StringHash /*eventType*/, VariantMap& eventData)
{
    if (drawDebug_)
        scene_->GetComponent<PhysicsWorld>()->DrawDebugGeometry(true);
}

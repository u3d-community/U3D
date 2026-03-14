#include <cstdio>

#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Core/Main.h>
#include <Urho3D/Engine/Engine.h>
#include <Urho3D/Engine/EngineDefs.h>
#include <Urho3D/Graphics/AnimatedModel.h>
#include <Urho3D/Graphics/Animation.h>
#include <Urho3D/Graphics/AnimationState.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/DebugRenderer.h>
#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Graphics/GraphicsEvents.h>
#include <Urho3D/Graphics/IndexBuffer.h>
#include <Urho3D/Graphics/Light.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Graphics/Skeleton.h>
#include <Urho3D/Graphics/Technique.h>
#include <Urho3D/Graphics/VertexBuffer.h>
#include <Urho3D/Graphics/Viewport.h>
#include <Urho3D/Graphics/Zone.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Resource/XMLFile.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/UI/Button.h>
#include <Urho3D/UI/CheckBox.h>
#include <Urho3D/UI/Font.h>
#include <Urho3D/UI/ListView.h>
#include <Urho3D/UI/Slider.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/UI/UI.h>
#include <Urho3D/UI/UIElement.h>
#include <Urho3D/UI/UIEvents.h>
#include <Urho3D/UI/Window.h>

#include "ModelViewer.h"

#include <Urho3D/DebugNew.h>

URHO3D_DEFINE_APPLICATION_MAIN(ModelViewer);

ModelViewer::ModelViewer(Context* context) :
    Application(context),
    cameraDistance_(5.0f),
    cameraYaw_(0.0f),
    cameraPitch_(20.0f),
    cameraTarget_(Vector3::ZERO),
    currentAnimState_(nullptr),
    animPlaying_(false),
    animSpeed_(1.0f),
    showWireframe_(false),
    showSkeleton_(false),
    showBounds_(false)
{
}

void ModelViewer::Setup()
{
    engineParameters_[EP_WINDOW_TITLE] = "ModelViewer";
    engineParameters_[EP_WINDOW_WIDTH] = 1280;
    engineParameters_[EP_WINDOW_HEIGHT] = 800;
    engineParameters_[EP_FULL_SCREEN] = false;
    engineParameters_[EP_LOG_NAME] = "ModelViewer.log";

    if (!engineParameters_.Contains(EP_RESOURCE_PREFIX_PATHS))
        engineParameters_[EP_RESOURCE_PREFIX_PATHS] = ";../share/Resources;../share/Urho3D/Resources";
}

void ModelViewer::Start()
{
    GetSubsystem<Input>()->SetMouseVisible(true);

    CreateScene();
    CreateUI();

    const Vector<String>& args = GetArguments();
    if (args.Size() && GetExtension(args[0]) == ".mdl")
    {
        auto* cache = GetSubsystem<ResourceCache>();
        auto* fs = GetSubsystem<FileSystem>();
        String fullPath = GetInternalPath(args[0]);
        if (!IsAbsolutePath(fullPath))
            fullPath = fs->GetCurrentDir() + fullPath;

        String dir = GetPath(fullPath);
        String filename = GetFileNameAndExtension(fullPath);
        cache->AddResourceDir(dir);
        LoadModel(filename);
    }
    else
    {
        auto* fs = GetSubsystem<FileSystem>();
        auto* cache = GetSubsystem<ResourceCache>();
        String cwd = fs->GetCurrentDir();

        Vector<String> files;
        fs->ScanDir(files, cwd, "*.mdl", SCAN_FILES, true);

        if (files.Size())
        {
            cache->AddResourceDir(cwd);
            auto* font = cache->GetResource<Font>("Fonts/Anonymous Pro.ttf");
            for (const String& file : files)
            {
                filePaths_.Push(file);
                auto* text = new Text(context_);
                text->SetFont(font, 11);
                text->SetText(file);
                text->SetStyleAuto();
                fileList_->AddItem(text);
            }
        }
    }

    SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(ModelViewer, HandleUpdate));
    SubscribeToEvent(E_POSTRENDERUPDATE, URHO3D_HANDLER(ModelViewer, HandlePostRenderUpdate));
    SubscribeToEvent(E_SCREENMODE, URHO3D_HANDLER(ModelViewer, HandleScreenMode));
}

void ModelViewer::CreateScene()
{
    scene_ = new Scene(context_);
    scene_->CreateComponent<Octree>();
    scene_->CreateComponent<DebugRenderer>();

    auto* zone = scene_->CreateComponent<Zone>();
    zone->SetAmbientColor(Color(0.4f, 0.4f, 0.4f));
    zone->SetBoundingBox(BoundingBox(-1000.0f, 1000.0f));

    Node* lightNode = scene_->CreateChild("DirectionalLight");
    lightNode->SetDirection(Vector3(0.6f, -1.0f, 0.8f));
    auto* light = lightNode->CreateComponent<Light>();
    light->SetLightType(LIGHT_DIRECTIONAL);
    light->SetCastShadows(true);

    cameraNode_ = scene_->CreateChild("Camera");
    cameraNode_->SetPosition(Vector3(0.0f, 2.0f, -5.0f));
    auto* camera = cameraNode_->CreateComponent<Camera>();

    modelNode_ = scene_->CreateChild("Model");

    auto* graphics = GetSubsystem<Graphics>();
    auto* renderer = GetSubsystem<Renderer>();
    SharedPtr<Viewport> viewport(
        new Viewport(context_, scene_, camera, IntRect(PANEL_WIDTH, 0, graphics->GetWidth(), graphics->GetHeight())));
    renderer->SetViewport(0, viewport);
}

void ModelViewer::CreateUI()
{
    auto* cache = GetSubsystem<ResourceCache>();
    auto* ui = GetSubsystem<UI>();
    auto* root = ui->GetRoot();
    auto* style = cache->GetResource<XMLFile>("UI/DefaultStyle.xml");
    root->SetDefaultStyle(style);

    auto* graphics = GetSubsystem<Graphics>();
    auto* font = cache->GetResource<Font>("Fonts/Anonymous Pro.ttf");

    // Main panel
    auto* panel = root->CreateChild<Window>("Panel");
    panel->SetStyleAuto();
    panel->SetLayout(LM_VERTICAL, 4, IntRect(6, 6, 6, 6));
    panel->SetPosition(0, 0);
    panel->SetFixedWidth(PANEL_WIDTH);
    panel->SetFixedHeight(graphics->GetHeight());
    panel->SetMovable(false);
    panel->SetResizable(false);
    panel_ = panel;

    // Files
    auto* filesLabel = panel->CreateChild<Text>();
    filesLabel->SetFont(font, 12);
    filesLabel->SetText("Files");
    filesLabel->SetStyleAuto();

    fileList_ = panel->CreateChild<ListView>();
    fileList_->SetStyleAuto();
    fileList_->SetMinHeight(150);
    fileList_->SetHighlightMode(HM_ALWAYS);
    fileList_->SetSelectOnClickEnd(true);
    SubscribeToEvent(fileList_, E_ITEMSELECTED, URHO3D_HANDLER(ModelViewer, HandleFileSelected));

    // Model Info
    auto* infoLabel = panel->CreateChild<Text>();
    infoLabel->SetFont(font, 12);
    infoLabel->SetText("Model Info");
    infoLabel->SetStyleAuto();

    infoText_ = panel->CreateChild<Text>();
    infoText_->SetFont(font, 11);
    infoText_->SetText("No model loaded");
    infoText_->SetStyleAuto();

    // Animations
    auto* animLabel = panel->CreateChild<Text>();
    animLabel->SetFont(font, 12);
    animLabel->SetText("Animations");
    animLabel->SetStyleAuto();

    animList_ = panel->CreateChild<ListView>();
    animList_->SetStyleAuto();
    animList_->SetMinHeight(80);
    animList_->SetHighlightMode(HM_ALWAYS);
    animList_->SetSelectOnClickEnd(true);
    SubscribeToEvent(animList_, E_ITEMSELECTED, URHO3D_HANDLER(ModelViewer, HandleAnimSelected));

    // Play/speed row
    auto* playRow = panel->CreateChild<UIElement>();
    playRow->SetLayout(LM_HORIZONTAL, 4);
    playRow->SetFixedHeight(24);

    playButton_ = playRow->CreateChild<Button>();
    playButton_->SetStyleAuto();
    playButton_->SetFixedWidth(60);
    playButtonText_ = playButton_->CreateChild<Text>();
    playButtonText_->SetFont(font, 11);
    playButtonText_->SetText("Play");
    playButtonText_->SetAlignment(HA_CENTER, VA_CENTER);
    SubscribeToEvent(playButton_, E_RELEASED, URHO3D_HANDLER(ModelViewer, HandlePlayPause));

    auto* speedLabel = playRow->CreateChild<Text>();
    speedLabel->SetFont(font, 11);
    speedLabel->SetText("Speed:");
    speedLabel->SetStyleAuto();

    speedSlider_ = playRow->CreateChild<Slider>();
    speedSlider_->SetStyleAuto();
    speedSlider_->SetRange(3.0f);
    speedSlider_->SetValue(1.0f);
    speedSlider_->SetFixedHeight(16);
    SubscribeToEvent(speedSlider_, E_SLIDERCHANGED, URHO3D_HANDLER(ModelViewer, HandleSpeedSliderChanged));

    speedText_ = playRow->CreateChild<Text>();
    speedText_->SetFont(font, 11);
    speedText_->SetText("1.0x");
    speedText_->SetStyleAuto();

    // Time display
    timeText_ = panel->CreateChild<Text>();
    timeText_->SetFont(font, 11);
    timeText_->SetText("0.00 / 0.00");
    timeText_->SetStyleAuto();

    // Time slider
    timeSlider_ = panel->CreateChild<Slider>();
    timeSlider_->SetStyleAuto();
    timeSlider_->SetRange(1.0f);
    timeSlider_->SetFixedHeight(16);
    SubscribeToEvent(timeSlider_, E_SLIDERCHANGED, URHO3D_HANDLER(ModelViewer, HandleTimeSliderChanged));

    // Display
    auto* displayLabel = panel->CreateChild<Text>();
    displayLabel->SetFont(font, 12);
    displayLabel->SetText("Display");
    displayLabel->SetStyleAuto();

    // Wireframe
    auto* wireRow = panel->CreateChild<UIElement>();
    wireRow->SetLayout(LM_HORIZONTAL, 6);
    wireRow->SetFixedHeight(20);
    auto* wireCheck = wireRow->CreateChild<CheckBox>();
    wireCheck->SetStyleAuto();
    auto* wireText = wireRow->CreateChild<Text>();
    wireText->SetFont(font, 11);
    wireText->SetText("Wireframe");
    wireText->SetStyleAuto();
    SubscribeToEvent(wireCheck, E_TOGGLED, URHO3D_HANDLER(ModelViewer, HandleWireframeToggled));

    // Skeleton
    auto* skelRow = panel->CreateChild<UIElement>();
    skelRow->SetLayout(LM_HORIZONTAL, 6);
    skelRow->SetFixedHeight(20);
    auto* skelCheck = skelRow->CreateChild<CheckBox>();
    skelCheck->SetStyleAuto();
    auto* skelText = skelRow->CreateChild<Text>();
    skelText->SetFont(font, 11);
    skelText->SetText("Skeleton");
    skelText->SetStyleAuto();
    SubscribeToEvent(skelCheck, E_TOGGLED, URHO3D_HANDLER(ModelViewer, HandleSkeletonToggled));

    // Bounding Box
    auto* boundsRow = panel->CreateChild<UIElement>();
    boundsRow->SetLayout(LM_HORIZONTAL, 6);
    boundsRow->SetFixedHeight(20);
    auto* boundsCheck = boundsRow->CreateChild<CheckBox>();
    boundsCheck->SetStyleAuto();
    auto* boundsText = boundsRow->CreateChild<Text>();
    boundsText->SetFont(font, 11);
    boundsText->SetText("Bounding Box");
    boundsText->SetStyleAuto();
    SubscribeToEvent(boundsCheck, E_TOGGLED, URHO3D_HANDLER(ModelViewer, HandleBoundsToggled));
}

void ModelViewer::LoadModel(const String& resourcePath)
{
    modelNode_->RemoveAllChildren();
    currentAnimState_ = nullptr;
    animPlaying_ = false;
    playButtonText_->SetText("Play");

    auto* cache = GetSubsystem<ResourceCache>();
    auto* model = cache->GetResource<Model>(resourcePath);
    if (!model)
    {
        URHO3D_LOGERROR("Failed to load model: " + resourcePath);
        infoText_->SetText("Failed to load:\n" + resourcePath);
        return;
    }

    currentModelPath_ = resourcePath;

    Node* child = modelNode_->CreateChild("LoadedModel");
    auto* animModel = child->CreateComponent<AnimatedModel>();
    animModel->SetModel(model);
    animModel->SetCastShadows(true);

    // Default grey material
    auto* defaultMat = new Material(context_);
    defaultMat->SetTechnique(0, cache->GetResource<Technique>("Techniques/NoTexture.xml"));
    defaultMat->SetShaderParameter("MatDiffColor", Color(0.6f, 0.6f, 0.6f));
    if (showWireframe_)
        defaultMat->SetFillMode(FILL_WIREFRAME);
    animModel->SetMaterial(defaultMat);

    FitCamera();
    DiscoverAnimations();
    UpdateInfoText();
}

void ModelViewer::FitCamera()
{
    auto* animModel = modelNode_->GetComponent<AnimatedModel>(true);
    if (!animModel)
        return;

    BoundingBox bbox = animModel->GetWorldBoundingBox();
    cameraTarget_ = bbox.Center();
    cameraDistance_ = bbox.Size().Length() * 1.5f;
    if (cameraDistance_ < 0.1f)
        cameraDistance_ = 5.0f;
    cameraYaw_ = 0.0f;
    cameraPitch_ = 20.0f;
    UpdateCameraPosition();
}

void ModelViewer::DiscoverAnimations()
{
    animList_->RemoveAllItems();
    animPaths_.Clear();
    currentAnimState_ = nullptr;
    animPlaying_ = false;

    auto* cache = GetSubsystem<ResourceCache>();
    auto* fs = GetSubsystem<FileSystem>();

    String fullPath = cache->GetResourceFileName(currentModelPath_);
    if (fullPath.Empty())
        return;

    String dir = GetPath(fullPath);
    Vector<String> files;
    fs->ScanDir(files, dir, "*.ani", SCAN_FILES, false);

    auto* font = cache->GetResource<Font>("Fonts/Anonymous Pro.ttf");
    String modelDir = GetPath(currentModelPath_);

    for (const String& file : files)
    {
        String resourcePath = modelDir + file;
        animPaths_.Push(resourcePath);

        auto* text = new Text(context_);
        text->SetFont(font, 11);
        text->SetText(file);
        text->SetStyleAuto();
        animList_->AddItem(text);
    }
}

void ModelViewer::UpdateInfoText()
{
    auto* animModel = modelNode_->GetComponent<AnimatedModel>(true);
    if (!animModel || !animModel->GetModel())
    {
        infoText_->SetText("No model loaded");
        return;
    }

    auto* model = animModel->GetModel();

    unsigned totalVertices = 0;
    unsigned totalIndices = 0;

    const auto& vbs = model->GetVertexBuffers();
    for (unsigned i = 0; i < vbs.Size(); ++i)
        totalVertices += vbs[i]->GetVertexCount();

    const auto& ibs = model->GetIndexBuffers();
    for (unsigned i = 0; i < ibs.Size(); ++i)
        totalIndices += ibs[i]->GetIndexCount();

    BoundingBox bbox = model->GetBoundingBox();
    unsigned numBones = model->GetSkeleton().GetNumBones();

    String info;
    info += "Geometries: " + String(model->GetNumGeometries()) + "\n";
    info += "Vertices: " + String(totalVertices) + "\n";
    info += "Indices: " + String(totalIndices) + "\n";
    info += "Bones: " + String(numBones) + "\n";
    char bboxBuf[128];
    snprintf(bboxBuf, sizeof(bboxBuf), "BBox: (%.1f, %.1f, %.1f)\n  to (%.1f, %.1f, %.1f)", bbox.min_.x_, bbox.min_.y_,
             bbox.min_.z_, bbox.max_.x_, bbox.max_.y_, bbox.max_.z_);
    info += String(bboxBuf);

    infoText_->SetText(info);
}

void ModelViewer::UpdateCameraPosition()
{
    Quaternion rot(cameraPitch_, cameraYaw_, 0.0f);
    Vector3 pos = cameraTarget_ + rot * Vector3(0.0f, 0.0f, -cameraDistance_);
    cameraNode_->SetPosition(pos);
    cameraNode_->LookAt(cameraTarget_);
}

void ModelViewer::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
    using namespace Update;
    float timeStep = eventData[P_TIMESTEP].GetFloat();

    auto* input = GetSubsystem<Input>();

    if (input->GetKeyPress(KEY_ESCAPE))
    {
        engine_->Exit();
        return;
    }

    // Camera controls (only when mouse is not over UI elements)
    auto* ui = GetSubsystem<UI>();
    UIElement* hovered = ui->GetElementAt(input->GetMousePosition(), true);
    bool overUI = hovered != nullptr && hovered != ui->GetRoot();

    if (!overUI)
    {
        // Left drag: orbit
        if (input->GetMouseButtonDown(MOUSEB_LEFT))
        {
            IntVector2 delta = input->GetMouseMove();
            cameraYaw_ += delta.x_ * 0.3f;
            cameraPitch_ += delta.y_ * 0.3f;
            cameraPitch_ = Clamp(cameraPitch_, -89.0f, 89.0f);
        }

        // Middle drag: pan
        if (input->GetMouseButtonDown(MOUSEB_MIDDLE))
        {
            IntVector2 delta = input->GetMouseMove();
            float panSpeed = cameraDistance_ * 0.002f;
            cameraTarget_ += cameraNode_->GetRight() * (-delta.x_ * panSpeed);
            cameraTarget_ += cameraNode_->GetUp() * (delta.y_ * panSpeed);
        }

        // Scroll: zoom
        int wheel = input->GetMouseMoveWheel();
        if (wheel != 0)
        {
            cameraDistance_ *= 1.0f - wheel * 0.1f;
            cameraDistance_ = Max(cameraDistance_, 0.1f);
        }
    }

    UpdateCameraPosition();

    // Animation playback
    if (animPlaying_ && currentAnimState_)
    {
        currentAnimState_->AddTime(timeStep * animSpeed_);

        float length = currentAnimState_->GetLength();
        float time = currentAnimState_->GetTime();
        if (length > 0.0f)
            timeSlider_->SetValue(time / length * timeSlider_->GetRange());
        char timeBuf[32];
        snprintf(timeBuf, sizeof(timeBuf), "%.2f / %.2f", time, length);
        timeText_->SetText(timeBuf);
    }
}

void ModelViewer::HandlePostRenderUpdate(StringHash eventType, VariantMap& eventData)
{
    auto* debug = scene_->GetComponent<DebugRenderer>();
    if (!debug)
        return;

    auto* animModel = modelNode_->GetComponent<AnimatedModel>(true);
    if (!animModel)
        return;

    if (showSkeleton_)
        animModel->DrawDebugGeometry(debug, false);

    if (showBounds_)
        debug->AddBoundingBox(animModel->GetWorldBoundingBox(), Color::GREEN, false);
}

void ModelViewer::HandleFileSelected(StringHash eventType, VariantMap& eventData)
{
    unsigned index = fileList_->GetSelection();
    if (index >= filePaths_.Size())
        return;

    LoadModel(filePaths_[index]);
}

void ModelViewer::HandleAnimSelected(StringHash eventType, VariantMap& eventData)
{
    unsigned index = animList_->GetSelection();
    if (index >= animPaths_.Size())
        return;

    auto* cache = GetSubsystem<ResourceCache>();
    auto* animModel = modelNode_->GetComponent<AnimatedModel>(true);
    if (!animModel)
        return;

    animModel->RemoveAllAnimationStates();
    currentAnimState_ = nullptr;
    animPlaying_ = false;
    playButtonText_->SetText("Play");

    auto* anim = cache->GetResource<Animation>(animPaths_[index]);
    if (!anim)
        return;

    currentAnimState_ = animModel->AddAnimationState(anim);
    if (currentAnimState_)
    {
        currentAnimState_->SetWeight(1.0f);
        currentAnimState_->SetLooped(true);
        char timeBuf[32];
        snprintf(timeBuf, sizeof(timeBuf), "0.00 / %.2f", currentAnimState_->GetLength());
        timeText_->SetText(timeBuf);
    }
}

void ModelViewer::HandlePlayPause(StringHash eventType, VariantMap& eventData)
{
    animPlaying_ = !animPlaying_;
    playButtonText_->SetText(animPlaying_ ? "Pause" : "Play");
}

void ModelViewer::HandleTimeSliderChanged(StringHash eventType, VariantMap& eventData)
{
    if (!currentAnimState_ || animPlaying_)
        return;

    float normalized = timeSlider_->GetValue() / timeSlider_->GetRange();
    float time = normalized * currentAnimState_->GetLength();
    currentAnimState_->SetTime(time);
    char timeBuf[32];
    snprintf(timeBuf, sizeof(timeBuf), "%.2f / %.2f", time, currentAnimState_->GetLength());
    timeText_->SetText(timeBuf);
}

void ModelViewer::HandleSpeedSliderChanged(StringHash eventType, VariantMap& eventData)
{
    animSpeed_ = speedSlider_->GetValue();
    char buf[16];
    snprintf(buf, sizeof(buf), "%.1fx", animSpeed_);
    speedText_->SetText(buf);
}

void ModelViewer::HandleWireframeToggled(StringHash eventType, VariantMap& eventData)
{
    using namespace Toggled;
    showWireframe_ = eventData[P_STATE].GetBool();

    auto* animModel = modelNode_->GetComponent<AnimatedModel>(true);
    if (!animModel)
        return;

    for (unsigned i = 0; i < animModel->GetNumGeometries(); ++i)
    {
        auto* mat = animModel->GetMaterial(i);
        if (mat)
            mat->SetFillMode(showWireframe_ ? FILL_WIREFRAME : FILL_SOLID);
    }
}

void ModelViewer::HandleSkeletonToggled(StringHash eventType, VariantMap& eventData)
{
    using namespace Toggled;
    showSkeleton_ = eventData[P_STATE].GetBool();
}

void ModelViewer::HandleBoundsToggled(StringHash eventType, VariantMap& eventData)
{
    using namespace Toggled;
    showBounds_ = eventData[P_STATE].GetBool();
}

void ModelViewer::HandleScreenMode(StringHash eventType, VariantMap& eventData)
{
    auto* graphics = GetSubsystem<Graphics>();
    auto* renderer = GetSubsystem<Renderer>();

    panel_->SetFixedHeight(graphics->GetHeight());

    auto* viewport = renderer->GetViewport(0);
    if (viewport)
        viewport->SetRect(IntRect(PANEL_WIDTH, 0, graphics->GetWidth(), graphics->GetHeight()));
}

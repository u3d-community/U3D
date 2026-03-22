#pragma once

#include <Urho3D/Engine/Application.h>

using namespace Urho3D;

class ModelViewer : public Application
{
    URHO3D_OBJECT(ModelViewer, Application);

public:
    explicit ModelViewer(Context* context);

    void Setup() override;
    void Start() override;

private:
    void CreateScene();
    void CreateUI();
    void LoadModel(const String& resourcePath);
    void FitCamera();
    void DiscoverAnimations();
    void UpdateInfoText();
    void UpdateCameraPosition();

    void HandleUpdate(StringHash eventType, VariantMap& eventData);
    void HandlePostRenderUpdate(StringHash eventType, VariantMap& eventData);
    void HandleFileSelected(StringHash eventType, VariantMap& eventData);
    void HandleAnimSelected(StringHash eventType, VariantMap& eventData);
    void HandlePlayPause(StringHash eventType, VariantMap& eventData);
    void HandleTimeSliderChanged(StringHash eventType, VariantMap& eventData);
    void HandleSpeedSliderChanged(StringHash eventType, VariantMap& eventData);
    void HandleWireframeToggled(StringHash eventType, VariantMap& eventData);
    void HandleSkeletonToggled(StringHash eventType, VariantMap& eventData);
    void HandleBoundsToggled(StringHash eventType, VariantMap& eventData);
    void HandleLightYawChanged(StringHash eventType, VariantMap& eventData);
    void HandleScreenMode(StringHash eventType, VariantMap& eventData);

    // Scene
    SharedPtr<Scene> scene_;
    SharedPtr<Node> cameraNode_;
    SharedPtr<Node> modelNode_;
    SharedPtr<Node> lightNode_;

    // Orbit camera
    float cameraDistance_;
    float cameraYaw_;
    float cameraPitch_;
    Vector3 cameraTarget_;

    // Model state
    String currentModelPath_;
    AnimationState* currentAnimState_;
    bool animPlaying_;
    float animSpeed_;

    // Display toggles
    bool showWireframe_;
    bool showSkeleton_;
    bool showBounds_;
    float lightYaw_;

    // UI elements
    SharedPtr<UIElement> panel_;
    SharedPtr<ListView> fileList_;
    SharedPtr<Text> infoText_;
    SharedPtr<ListView> animList_;
    SharedPtr<Slider> timeSlider_;
    SharedPtr<Text> timeText_;
    SharedPtr<Button> playButton_;
    SharedPtr<Text> playButtonText_;
    SharedPtr<Slider> speedSlider_;
    SharedPtr<Text> speedText_;

    // Resource paths
    Vector<String> filePaths_;
    Vector<String> animPaths_;

    static const int PANEL_WIDTH = 300;
};

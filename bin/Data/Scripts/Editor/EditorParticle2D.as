// Urho3D particle 2d editor

Window@ particleEffect2dWindow;
ParticleEffect2D@ editParticleEffect2d;
XMLFile@ oldParticleEffect2dState;
bool inParticle2dEffectRefresh = false;
View3D@ particle2dEffectPreview;
Camera@ particle2dPreviewCamera;
Scene@ particle2dPreviewScene;
Node@ particle2dEffectPreviewNode;
Node@ particle2dEffectPreviewGizmoNode;
Node@ particle2dEffectPreviewGridNode;
CustomGeometry@ particle2dEffectPreviewGrid;
Node@ particle2dPreviewCameraNode;
Node@ particle2dPreviewLightNode;
Light@ particle2dPreviewLight;
ParticleEmitter2D@ particleEffect2dEmitter;
float particle2dResetTimer;
bool showParticle2dPreviewAxes = true;
Vector3 particle2dViewCamDir;
Vector3 particle2dViewCamRot;
float particle2dViewCamDist;

bool particle2dLoopEmission = true;

float particle2dGizmoOffset = 0.1f;
float particle2dGizmoOffsetX;
float particle2dGizmoOffsetY;

void CreateParticleEffectEditor2d()
{
    if (particleEffect2dWindow !is null)
        return;

    particleEffect2dWindow = LoadEditorUI("UI/EditorParticleEffect2DWindow.xml");
    ui.root.AddChild(particleEffect2dWindow);
    particleEffect2dWindow.opacity = uiMaxOpacity;

    InitParticleEffectPreview2d();
    InitParticleEffect2dBasicAttributes();
    RefreshParticleEffectEditor2d();

    int width = Min(ui.root.width - 60, 800);
    int height = Min(ui.root.height - 60, 600);
    particleEffect2dWindow.SetSize(width, height);
    CenterDialog(particleEffect2dWindow);

    HideParticleEffectEditor2d();

    SubscribeToEvent(particleEffect2dWindow.GetChild("NewButton", true), "Released", "NewParticleEffect2d");
    SubscribeToEvent(particleEffect2dWindow.GetChild("RevertButton", true), "Released", "RevertParticleEffect2d");
    SubscribeToEvent(particleEffect2dWindow.GetChild("SaveButton", true), "Released", "SaveParticleEffect2d");
    SubscribeToEvent(particleEffect2dWindow.GetChild("SaveAsButton", true), "Released", "SaveParticleEffect2dAs");
    SubscribeToEvent(particleEffect2dWindow.GetChild("CloseButton", true), "Released", "HideParticleEffectEditor2d");

    SubscribeToEvent(particleEffect2dWindow.GetChild("Speed", true), "TextChanged", "EditParticleEffect2dSpeed");
    SubscribeToEvent(particleEffect2dWindow.GetChild("SpeedVariance", true), "TextChanged", "EditParticleEffect2dSpeed");
    SubscribeToEvent(particleEffect2dWindow.GetChild("ParticleLifespan", true), "TextChanged", "EditParticleEffect2dParticleLifespan");
    SubscribeToEvent(particleEffect2dWindow.GetChild("ParticleLifespanVariance", true), "TextChanged", "EditParticleEffect2dParticleLifespan");
    SubscribeToEvent(particleEffect2dWindow.GetChild("ParticleAngle", true), "TextChanged", "EditParticleEffect2dParticleAngle");
    SubscribeToEvent(particleEffect2dWindow.GetChild("ParticleAngleVariance", true), "TextChanged", "EditParticleEffect2dParticleAngle");
    SubscribeToEvent(particleEffect2dWindow.GetChild("GravityX", true), "TextChanged", "EditParticleEffect2dGravity");
    SubscribeToEvent(particleEffect2dWindow.GetChild("GravityY", true), "TextChanged", "EditParticleEffect2dGravity");
    SubscribeToEvent(particleEffect2dWindow.GetChild("RadialAcceleration", true), "TextChanged", "EditParticleEffect2dRadialAcceleration");
    SubscribeToEvent(particleEffect2dWindow.GetChild("RadialAccelerationVariance", true), "TextChanged", "EditParticleEffect2dRadialAcceleration");
    SubscribeToEvent(particleEffect2dWindow.GetChild("TangentialAcceleration", true), "TextChanged", "EditParticleEffect2dTangentialAcceleration");
    SubscribeToEvent(particleEffect2dWindow.GetChild("TangentialAccelerationVariance", true), "TextChanged", "EditParticleEffect2dTangentialAcceleration");
    SubscribeToEvent(particleEffect2dWindow.GetChild("StartColor_R", true), "TextChanged", "EditParticleEffect2dStartColor");
    SubscribeToEvent(particleEffect2dWindow.GetChild("StartColor_G", true), "TextChanged", "EditParticleEffect2dStartColor");
    SubscribeToEvent(particleEffect2dWindow.GetChild("StartColor_B", true), "TextChanged", "EditParticleEffect2dStartColor");
    SubscribeToEvent(particleEffect2dWindow.GetChild("StartColor_A", true), "TextChanged", "EditParticleEffect2dStartColor");
    SubscribeToEvent(particleEffect2dWindow.GetChild("StartColorVariance_R", true), "TextChanged", "EditParticleEffect2dStartColor");
    SubscribeToEvent(particleEffect2dWindow.GetChild("StartColorVariance_G", true), "TextChanged", "EditParticleEffect2dStartColor");
    SubscribeToEvent(particleEffect2dWindow.GetChild("StartColorVariance_B", true), "TextChanged", "EditParticleEffect2dStartColor");
    SubscribeToEvent(particleEffect2dWindow.GetChild("StartColorVariance_A", true), "TextChanged", "EditParticleEffect2dStartColor");
    SubscribeToEvent(particleEffect2dWindow.GetChild("EndColor_R", true), "TextChanged", "EditParticleEffect2dEndColor");
    SubscribeToEvent(particleEffect2dWindow.GetChild("EndColor_G", true), "TextChanged", "EditParticleEffect2dEndColor");
    SubscribeToEvent(particleEffect2dWindow.GetChild("EndColor_B", true), "TextChanged", "EditParticleEffect2dEndColor");
    SubscribeToEvent(particleEffect2dWindow.GetChild("EndColor_A", true), "TextChanged", "EditParticleEffect2dEndColor");
    SubscribeToEvent(particleEffect2dWindow.GetChild("EndColorVariance_R", true), "TextChanged", "EditParticleEffect2dEndColor");
    SubscribeToEvent(particleEffect2dWindow.GetChild("EndColorVariance_G", true), "TextChanged", "EditParticleEffect2dEndColor");
    SubscribeToEvent(particleEffect2dWindow.GetChild("EndColorVariance_B", true), "TextChanged", "EditParticleEffect2dEndColor");
    SubscribeToEvent(particleEffect2dWindow.GetChild("EndColorVariance_A", true), "TextChanged", "EditParticleEffect2dEndColor");
    SubscribeToEvent(particleEffect2dWindow.GetChild("MaxNumParticles", true), "TextChanged", "EditParticleEffect2dMaxNumParticles");
    SubscribeToEvent(particleEffect2dWindow.GetChild("SourcePosVarianceX", true), "TextChanged", "EditParticleEffect2dSourcePosVariance");
    SubscribeToEvent(particleEffect2dWindow.GetChild("SourcePosVarianceY", true), "TextChanged", "EditParticleEffect2dSourcePosVariance");
    SubscribeToEvent(particleEffect2dWindow.GetChild("StartParticleSize", true), "TextChanged", "EditParticleEffect2dStartParticleSize");
    SubscribeToEvent(particleEffect2dWindow.GetChild("StartParticleSizeVariance", true), "TextChanged", "EditParticleEffect2dStartParticleSize");
    SubscribeToEvent(particleEffect2dWindow.GetChild("EndParticleSize", true), "TextChanged", "EditParticleEffect2dEndParticleSize");
    SubscribeToEvent(particleEffect2dWindow.GetChild("EndParticleSizeVariance", true), "TextChanged", "EditParticleEffect2dEndParticleSize");
    SubscribeToEvent(particleEffect2dWindow.GetChild("Duration", true), "TextChanged", "EditParticleEffect2dDuration");
    SubscribeToEvent(particleEffect2dWindow.GetChild("MinRadius", true), "TextChanged", "EditParticleEffect2dRadius");
    SubscribeToEvent(particleEffect2dWindow.GetChild("MinRadiusVariance", true), "TextChanged", "EditParticleEffect2dRadius");
    SubscribeToEvent(particleEffect2dWindow.GetChild("MaxRadius", true), "TextChanged", "EditParticleEffect2dRadius");
    SubscribeToEvent(particleEffect2dWindow.GetChild("MaxRadiusVariance", true), "TextChanged", "EditParticleEffect2dRadius");
    SubscribeToEvent(particleEffect2dWindow.GetChild("RotationPerSecond", true), "TextChanged", "EditParticleEffect2dRotation");
    SubscribeToEvent(particleEffect2dWindow.GetChild("RotationPerSecondVariance", true), "TextChanged", "EditParticleEffect2dRotation");
    SubscribeToEvent(particleEffect2dWindow.GetChild("RotationStart", true), "TextChanged", "EditParticleEffect2dRotation");
    SubscribeToEvent(particleEffect2dWindow.GetChild("RotationStartVariance", true), "TextChanged", "EditParticleEffect2dRotation");
    SubscribeToEvent(particleEffect2dWindow.GetChild("RotationEnd", true), "TextChanged", "EditParticleEffect2dRotation");
    SubscribeToEvent(particleEffect2dWindow.GetChild("RotationEndVariance", true), "TextChanged", "EditParticleEffect2dRotation");



    SubscribeToEvent(particleEffect2dWindow.GetChild("Speed", true), "TextFinished", "EditParticleEffect2dSpeed");
    SubscribeToEvent(particleEffect2dWindow.GetChild("SpeedVariance", true), "TextFinished", "EditParticleEffect2dSpeed");
    SubscribeToEvent(particleEffect2dWindow.GetChild("ParticleLifespan", true), "TextFinished", "EditParticleEffect2dParticleLifespan");
    SubscribeToEvent(particleEffect2dWindow.GetChild("ParticleLifespanVariance", true), "TextFinished", "EditParticleEffect2dParticleLifespan");
    SubscribeToEvent(particleEffect2dWindow.GetChild("ParticleAngle", true), "TextFinished", "EditParticleEffect2dParticleAngle");
    SubscribeToEvent(particleEffect2dWindow.GetChild("ParticleAngleVariance", true), "TextFinished", "EditParticleEffect2dParticleAngle");
    SubscribeToEvent(particleEffect2dWindow.GetChild("GravityX", true), "TextFinished", "EditParticleEffect2dGravity");
    SubscribeToEvent(particleEffect2dWindow.GetChild("GravityY", true), "TextFinished", "EditParticleEffect2dGravity");
    SubscribeToEvent(particleEffect2dWindow.GetChild("RadialAcceleration", true), "TextFinished", "EditParticleEffect2dRadialAcceleration");
    SubscribeToEvent(particleEffect2dWindow.GetChild("RadialAccelerationVariance", true), "TextFinished", "EditParticleEffect2dRadialAcceleration");
    SubscribeToEvent(particleEffect2dWindow.GetChild("TangentialAcceleration", true), "TextFinished", "EditParticleEffect2dTangentialAcceleration");
    SubscribeToEvent(particleEffect2dWindow.GetChild("TangentialAccelerationVariance", true), "TextFinished", "EditParticleEffect2dTangentialAcceleration");
    SubscribeToEvent(particleEffect2dWindow.GetChild("StartColor_R", true), "TextFinished", "EditParticleEffect2dStartColor");
    SubscribeToEvent(particleEffect2dWindow.GetChild("StartColor_G", true), "TextFinished", "EditParticleEffect2dStartColor");
    SubscribeToEvent(particleEffect2dWindow.GetChild("StartColor_B", true), "TextFinished", "EditParticleEffect2dStartColor");
    SubscribeToEvent(particleEffect2dWindow.GetChild("StartColor_A", true), "TextFinished", "EditParticleEffect2dStartColor");
    SubscribeToEvent(particleEffect2dWindow.GetChild("StartColorVariance_R", true), "TextFinished", "EditParticleEffect2dStartColor");
    SubscribeToEvent(particleEffect2dWindow.GetChild("StartColorVariance_G", true), "TextFinished", "EditParticleEffect2dStartColor");
    SubscribeToEvent(particleEffect2dWindow.GetChild("StartColorVariance_B", true), "TextFinished", "EditParticleEffect2dStartColor");
    SubscribeToEvent(particleEffect2dWindow.GetChild("StartColorVariance_A", true), "TextFinished", "EditParticleEffect2dStartColor");
    SubscribeToEvent(particleEffect2dWindow.GetChild("EndColor_R", true), "TextFinished", "EditParticleEffect2dEndColor");
    SubscribeToEvent(particleEffect2dWindow.GetChild("EndColor_G", true), "TextFinished", "EditParticleEffect2dEndColor");
    SubscribeToEvent(particleEffect2dWindow.GetChild("EndColor_B", true), "TextFinished", "EditParticleEffect2dEndColor");
    SubscribeToEvent(particleEffect2dWindow.GetChild("EndColor_A", true), "TextFinished", "EditParticleEffect2dEndColor");
    SubscribeToEvent(particleEffect2dWindow.GetChild("EndColorVariance_R", true), "TextFinished", "EditParticleEffect2dEndColor");
    SubscribeToEvent(particleEffect2dWindow.GetChild("EndColorVariance_G", true), "TextFinished", "EditParticleEffect2dEndColor");
    SubscribeToEvent(particleEffect2dWindow.GetChild("EndColorVariance_B", true), "TextFinished", "EditParticleEffect2dEndColor");
    SubscribeToEvent(particleEffect2dWindow.GetChild("EndColorVariance_A", true), "TextFinished", "EditParticleEffect2dEndColor");
    SubscribeToEvent(particleEffect2dWindow.GetChild("MaxNumParticles", true), "TextFinished", "EditParticleEffect2dMaxNumParticles");
    SubscribeToEvent(particleEffect2dWindow.GetChild("SourcePosVarianceX", true), "TextFinished", "EditParticleEffect2dSourcePosVariance");
    SubscribeToEvent(particleEffect2dWindow.GetChild("SourcePosVarianceY", true), "TextFinished", "EditParticleEffect2dSourcePosVariance");
    SubscribeToEvent(particleEffect2dWindow.GetChild("StartParticleSize", true), "TextFinished", "EditParticleEffect2dStartParticleSize");
    SubscribeToEvent(particleEffect2dWindow.GetChild("StartParticleSizeVariance", true), "TextFinished", "EditParticleEffect2dStartParticleSize");
    SubscribeToEvent(particleEffect2dWindow.GetChild("EndParticleSize", true), "TextFinished", "EditParticleEffect2dEndParticleSize");
    SubscribeToEvent(particleEffect2dWindow.GetChild("EndParticleSizeVariance", true), "TextFinished", "EditParticleEffect2dEndParticleSize");
    SubscribeToEvent(particleEffect2dWindow.GetChild("Duration", true), "TextFinished", "EditParticleEffect2dDuration");
    SubscribeToEvent(particleEffect2dWindow.GetChild("MinRadius", true), "TextFinished", "EditParticleEffect2dRadius");
    SubscribeToEvent(particleEffect2dWindow.GetChild("MinRadiusVariance", true), "TextFinished", "EditParticleEffect2dRadius");
    SubscribeToEvent(particleEffect2dWindow.GetChild("MaxRadius", true), "TextFinished", "EditParticleEffect2dRadius");
    SubscribeToEvent(particleEffect2dWindow.GetChild("MaxRadiusVariance", true), "TextFinished", "EditParticleEffect2dRadius");
    SubscribeToEvent(particleEffect2dWindow.GetChild("RotationPerSecond", true), "TextFinished", "EditParticleEffect2dRotation");
    SubscribeToEvent(particleEffect2dWindow.GetChild("RotationPerSecondVariance", true), "TextFinished", "EditParticleEffect2dRotation");
    SubscribeToEvent(particleEffect2dWindow.GetChild("RotationStart", true), "TextFinished", "EditParticleEffect2dRotation");
    SubscribeToEvent(particleEffect2dWindow.GetChild("RotationStartVariance", true), "TextFinished", "EditParticleEffect2dRotation");
    SubscribeToEvent(particleEffect2dWindow.GetChild("RotationEnd", true), "TextFinished", "EditParticleEffect2dRotation");
    SubscribeToEvent(particleEffect2dWindow.GetChild("RotationEndVariance", true), "TextFinished", "EditParticleEffect2dRotation");

    SubscribeToEvent(particleEffect2dWindow.GetChild("EmitterType", true), "ItemSelected", "EditParticleEffect2dEmitterType");
    SubscribeToEvent(particleEffect2dWindow.GetChild("BlendMode", true), "ItemSelected", "EditParticleEffect2dEmitterType");

    SubscribeToEvent(particleEffect2dWindow.GetChild("ResetViewport", true), "Released", "ParticleEffect2dResetViewport");
    SubscribeToEvent(particleEffect2dWindow.GetChild("ShowGrid", true), "Toggled", "ParticleEffect2dShowGrid");
    SubscribeToEvent(particleEffect2dWindow.GetChild("LoopEmission", true), "Toggled", "ParticleEffect2dToggleLoopEmission");

    SubscribeToEvent(particleEffect2dEmitter, "ParticlesEnd", "ParticleEmitter2dDoneEmitting");
}

void Particle2d_SetGizmoPosition()
{
    Vector3 screenPos = Vector3(particle2dGizmoOffsetX, particle2dGizmoOffsetY, 25.0);
    Vector3 newPos = particle2dPreviewCamera.ScreenToWorldPoint(screenPos);
    particle2dEffectPreviewGizmoNode.position = newPos;
}

void Particle2d_ResetCameraTransformation()
{
    particle2dPreviewCameraNode.position = Vector3(0, 0, -5);
    particle2dPreviewCameraNode.LookAt(Vector3(0, 0, 0));
    particle2dViewCamDir = -particle2dPreviewCameraNode.position;
    
    // Manually set initial rotation because eulerAngle always return 0 on first frame
    particle2dViewCamRot = Vector3(0.0, 180.0, 0.0);

    particle2dViewCamDist = particle2dViewCamDir.length;
    particle2dViewCamDir.Normalize();
}

void ParticleEffect2dResetViewport(StringHash eventType, VariantMap& eventData)
{
    Particle2d_ResetCameraTransformation();
    Particle2d_SetGizmoPosition();
    particle2dEffectPreview.QueueUpdate();
}

void ParticleEffect2dShowGrid(StringHash eventType, VariantMap& eventData)
{
    CheckBox@ element = eventData["Element"].GetPtr();
    showParticle2dPreviewAxes = element.checked;
    particle2dEffectPreviewGridNode.enabled = showParticle2dPreviewAxes;
    particle2dEffectPreview.QueueUpdate();
}

void ParticleEffect2dToggleLoopEmission(StringHash eventType, VariantMap& eventData)
{
    CheckBox@ element = eventData["Element"].GetPtr();
    particle2dLoopEmission = element.checked;
}


void UpdateParticleEffect2dWindow(float timeStep)
{
    // Handle Particle Editor 2D looping.
    if (particleEffect2dWindow !is null and particleEffect2dWindow.visible)
    {
        if (!particleEffect2dEmitter.emitting)
        {
            if (particle2dResetTimer == 0.0f)
                particle2dResetTimer = editParticleEffect2d.GetDuration() + 0.2f;
            else
            {
                particle2dResetTimer = Max(particle2dResetTimer - timeStep, 0.0f);
                if (particle2dResetTimer <= 0.0001f && particle2dLoopEmission)
                {
                    particleEffect2dEmitter.emitting = true;
                    particle2dResetTimer = 0.0f;
                }
            }
        }
    }
}

void ParticleEmitter2dDoneEmitting(StringHash eventType, VariantMap& eventData)
{
    particleEffect2dEmitter.emitting = false;
}


void InitParticleEffect2dBasicAttributes()
{
    CreateDragSlider(cast<LineEdit>(particleEffect2dWindow.GetChild("Speed", true)));
    CreateDragSlider(cast<LineEdit>(particleEffect2dWindow.GetChild("SpeedVariance", true)));
    CreateDragSlider(cast<LineEdit>(particleEffect2dWindow.GetChild("ParticleLifespan", true)));
    CreateDragSlider(cast<LineEdit>(particleEffect2dWindow.GetChild("ParticleLifespanVariance", true)));
    CreateDragSlider(cast<LineEdit>(particleEffect2dWindow.GetChild("ParticleAngle", true)));
    CreateDragSlider(cast<LineEdit>(particleEffect2dWindow.GetChild("ParticleAngleVariance", true)));
    CreateDragSlider(cast<LineEdit>(particleEffect2dWindow.GetChild("GravityX", true)));
    CreateDragSlider(cast<LineEdit>(particleEffect2dWindow.GetChild("GravityY", true)));
    CreateDragSlider(cast<LineEdit>(particleEffect2dWindow.GetChild("RadialAcceleration", true)));
    CreateDragSlider(cast<LineEdit>(particleEffect2dWindow.GetChild("RadialAccelerationVariance", true)));
    CreateDragSlider(cast<LineEdit>(particleEffect2dWindow.GetChild("TangentialAcceleration", true)));
    CreateDragSlider(cast<LineEdit>(particleEffect2dWindow.GetChild("TangentialAccelerationVariance", true)));
    CreateDragSlider(cast<LineEdit>(particleEffect2dWindow.GetChild("StartColor_R", true)));
    CreateDragSlider(cast<LineEdit>(particleEffect2dWindow.GetChild("StartColor_G", true)));
    CreateDragSlider(cast<LineEdit>(particleEffect2dWindow.GetChild("StartColor_B", true)));
    CreateDragSlider(cast<LineEdit>(particleEffect2dWindow.GetChild("StartColor_A", true)));
    CreateDragSlider(cast<LineEdit>(particleEffect2dWindow.GetChild("StartColorVariance_R", true)));
    CreateDragSlider(cast<LineEdit>(particleEffect2dWindow.GetChild("StartColorVariance_G", true)));
    CreateDragSlider(cast<LineEdit>(particleEffect2dWindow.GetChild("StartColorVariance_B", true)));
    CreateDragSlider(cast<LineEdit>(particleEffect2dWindow.GetChild("StartColorVariance_A", true)));
    CreateDragSlider(cast<LineEdit>(particleEffect2dWindow.GetChild("EndColor_R", true)));
    CreateDragSlider(cast<LineEdit>(particleEffect2dWindow.GetChild("EndColor_G", true)));
    CreateDragSlider(cast<LineEdit>(particleEffect2dWindow.GetChild("EndColor_B", true)));
    CreateDragSlider(cast<LineEdit>(particleEffect2dWindow.GetChild("EndColor_A", true)));
    CreateDragSlider(cast<LineEdit>(particleEffect2dWindow.GetChild("EndColorVariance_R", true)));
    CreateDragSlider(cast<LineEdit>(particleEffect2dWindow.GetChild("EndColorVariance_G", true)));
    CreateDragSlider(cast<LineEdit>(particleEffect2dWindow.GetChild("EndColorVariance_B", true)));
    CreateDragSlider(cast<LineEdit>(particleEffect2dWindow.GetChild("EndColorVariance_A", true)));
    CreateDragSlider(cast<LineEdit>(particleEffect2dWindow.GetChild("MaxNumParticles", true)));
    CreateDragSlider(cast<LineEdit>(particleEffect2dWindow.GetChild("SourcePosVarianceX", true)));
    CreateDragSlider(cast<LineEdit>(particleEffect2dWindow.GetChild("SourcePosVarianceY", true)));
    CreateDragSlider(cast<LineEdit>(particleEffect2dWindow.GetChild("StartParticleSize", true)));
    CreateDragSlider(cast<LineEdit>(particleEffect2dWindow.GetChild("StartParticleSizeVariance", true)));
    CreateDragSlider(cast<LineEdit>(particleEffect2dWindow.GetChild("EndParticleSize", true)));
    CreateDragSlider(cast<LineEdit>(particleEffect2dWindow.GetChild("EndParticleSizeVariance", true)));
    CreateDragSlider(cast<LineEdit>(particleEffect2dWindow.GetChild("Duration", true)));
    CreateDragSlider(cast<LineEdit>(particleEffect2dWindow.GetChild("MinRadius", true)));
    CreateDragSlider(cast<LineEdit>(particleEffect2dWindow.GetChild("MinRadiusVariance", true)));
    CreateDragSlider(cast<LineEdit>(particleEffect2dWindow.GetChild("MaxRadius", true)));
    CreateDragSlider(cast<LineEdit>(particleEffect2dWindow.GetChild("MaxRadiusVariance", true)));
    CreateDragSlider(cast<LineEdit>(particleEffect2dWindow.GetChild("RotationPerSecond", true)));
    CreateDragSlider(cast<LineEdit>(particleEffect2dWindow.GetChild("RotationPerSecondVariance", true)));
    CreateDragSlider(cast<LineEdit>(particleEffect2dWindow.GetChild("RotationStart", true)));
    CreateDragSlider(cast<LineEdit>(particleEffect2dWindow.GetChild("RotationStartVariance", true)));
    CreateDragSlider(cast<LineEdit>(particleEffect2dWindow.GetChild("RotationEnd", true)));
    CreateDragSlider(cast<LineEdit>(particleEffect2dWindow.GetChild("RotationEndVariance", true)));
}

void EditParticleEffect2dSpeed(StringHash eventType, VariantMap& eventData)
{
    if (inParticle2dEffectRefresh)
        return;

    if (editParticleEffect2d is null)
        return;

    BeginParticleEffect2dEdit();

    LineEdit@ element = eventData["Element"].GetPtr();

    if (element.name == "Speed")
        editParticleEffect2d.SetSpeed(element.text.ToFloat());

    if (element.name == "SpeedVariance")
        editParticleEffect2d.SetSpeedVariance(element.text.ToFloat());

    EndParticleEffect2dEdit();
}

void EditParticleEffect2dParticleLifespan(StringHash eventType, VariantMap& eventData)
{
    if (inParticle2dEffectRefresh)
        return;

    if (editParticleEffect2d is null)
        return;

    BeginParticleEffect2dEdit();

    LineEdit@ element = eventData["Element"].GetPtr();

    if (element.name == "ParticleLifespan"){
        // base lifespan must always be above 0!
        editParticleEffect2d.SetParticleLifeSpan(Max(0.01, element.text.ToFloat()));
    }


    if (element.name == "ParticleLifespanVariance")
        editParticleEffect2d.SetParticleLifespanVariance(element.text.ToFloat());

    RefreshParticleEffect2dEmitter();

    EndParticleEffect2dEdit();
}

void EditParticleEffect2dParticleAngle(StringHash eventType, VariantMap& eventData)
{
    if (inParticle2dEffectRefresh)
        return;

    if (editParticleEffect2d is null)
        return;

    BeginParticleEffect2dEdit();

    LineEdit@ element = eventData["Element"].GetPtr();

    if (element.name == "ParticleAngle")
        editParticleEffect2d.SetAngle(element.text.ToFloat());

    if (element.name == "ParticleAngleVariance")
        editParticleEffect2d.SetAngleVariance(element.text.ToFloat());

    EndParticleEffect2dEdit();
}

void EditParticleEffect2dStartParticleSize(StringHash eventType, VariantMap& eventData)
{
    if (inParticle2dEffectRefresh)
        return;

    if (editParticleEffect2d is null)
        return;

    BeginParticleEffect2dEdit();

    LineEdit@ element = eventData["Element"].GetPtr();

    if (element.name == "StartParticleSize")
        editParticleEffect2d.SetStartParticleSize(element.text.ToFloat());

    if (element.name == "StartParticleSizeVariance")
        editParticleEffect2d.SetStartParticleSizeVariance(element.text.ToFloat());

    EndParticleEffect2dEdit();
}

void EditParticleEffect2dEndParticleSize(StringHash eventType, VariantMap& eventData)
{
    if (inParticle2dEffectRefresh)
        return;

    if (editParticleEffect2d is null)
        return;

    BeginParticleEffect2dEdit();

    LineEdit@ element = eventData["Element"].GetPtr();

    if (element.name == "EndParticleSize")
        editParticleEffect2d.SetFinishParticleSize(element.text.ToFloat());

    if (element.name == "EndParticleSizeVariance")
        editParticleEffect2d.SetFinishParticleSizeVariance(element.text.ToFloat());

    EndParticleEffect2dEdit();
}

void EditParticleEffect2dDuration(StringHash eventType, VariantMap& eventData)
{
    if (inParticle2dEffectRefresh)
        return;

    if (editParticleEffect2d is null)
        return;

    BeginParticleEffect2dEdit();

    LineEdit@ element = eventData["Element"].GetPtr();

    if (element.name == "Duration")
    {
        editParticleEffect2d.SetDuration(element.text.ToFloat());
        RefreshParticleEffect2dEmitter();
    }

    EndParticleEffect2dEdit();
}

void EditParticleEffect2dRadius(StringHash eventType, VariantMap& eventData)
{
    if (inParticle2dEffectRefresh)
        return;

    if (editParticleEffect2d is null)
        return;

    BeginParticleEffect2dEdit();

    LineEdit@ element = eventData["Element"].GetPtr();

    if (element.name == "MinRadius")
        editParticleEffect2d.SetMinRadius(element.text.ToFloat());

    if (element.name == "MinRadiusVariance")
        editParticleEffect2d.SetMinRadiusVariance(element.text.ToFloat());

    if (element.name == "MaxRadius")
        editParticleEffect2d.SetMaxRadius(element.text.ToFloat());

    if (element.name == "MaxRadiusVariance")
        editParticleEffect2d.SetMaxRadiusVariance(element.text.ToFloat());

    EndParticleEffect2dEdit();
}

void EditParticleEffect2dRotation(StringHash eventType, VariantMap& eventData)
{
    if (inParticle2dEffectRefresh)
        return;

    if (editParticleEffect2d is null)
        return;

    BeginParticleEffect2dEdit();

    LineEdit@ element = eventData["Element"].GetPtr();

    if (element.name == "RotationPerSecond")
        editParticleEffect2d.SetRotatePerSecond(element.text.ToFloat());

    if (element.name == "RotationPerSecondVariance")
        editParticleEffect2d.SetRotatePerSecondVariance(element.text.ToFloat());

    if (element.name == "RotationStart")
        editParticleEffect2d.SetRotationStart(element.text.ToFloat());

    if (element.name == "RotationStartVariance")
        editParticleEffect2d.SetRotationStartVariance(element.text.ToFloat());

    if (element.name == "RotationEnd")
        editParticleEffect2d.SetRotationEnd(element.text.ToFloat());

    if (element.name == "RotationEndVariance")
        editParticleEffect2d.SetRotationEndVariance(element.text.ToFloat());

    EndParticleEffect2dEdit();
}

void EditParticleEffect2dGravity(StringHash eventType, VariantMap& eventData)
{
    if (inParticle2dEffectRefresh)
        return;

    if (editParticleEffect2d is null)
        return;

    BeginParticleEffect2dEdit();

    LineEdit@ element = eventData["Element"].GetPtr();

    Vector2 v = editParticleEffect2d.GetGravity();

    if (element.name == "GravityX")
        editParticleEffect2d.SetGravity(Vector2(element.text.ToFloat(), v.y));

    if (element.name == "GravityY")
        editParticleEffect2d.SetGravity(Vector2(v.x, element.text.ToFloat()));

    EndParticleEffect2dEdit();
}

void EditParticleEffect2dSourcePosVariance(StringHash eventType, VariantMap& eventData)
{
    if (inParticle2dEffectRefresh)
        return;

    if (editParticleEffect2d is null)
        return;

    BeginParticleEffect2dEdit();

    LineEdit@ element = eventData["Element"].GetPtr();

    Vector2 v = editParticleEffect2d.GetSourcePositionVariance();

    if (element.name == "SourcePosVarianceX")
        editParticleEffect2d.SetSourcePositionVariance(Vector2(element.text.ToFloat(), v.y));

    if (element.name == "SourcePosVarianceY")
        editParticleEffect2d.SetSourcePositionVariance(Vector2(v.x, element.text.ToFloat()));

    EndParticleEffect2dEdit();
}

void EditParticleEffect2dRadialAcceleration(StringHash eventType, VariantMap& eventData)
{
    if (inParticle2dEffectRefresh)
        return;

    if (editParticleEffect2d is null)
        return;


    BeginParticleEffect2dEdit();

    LineEdit@ element = eventData["Element"].GetPtr();

    if (element.name == "RadialAcceleration")
        editParticleEffect2d.SetRadialAcceleration(element.text.ToFloat());

    if (element.name == "RadialAccelerationVariance")
        editParticleEffect2d.SetRadialAccelVariance(element.text.ToFloat());

    EndParticleEffect2dEdit();
}

void EditParticleEffect2dTangentialAcceleration(StringHash eventType, VariantMap& eventData)
{
    if (inParticle2dEffectRefresh)
        return;

    if (editParticleEffect2d is null)
        return;

    BeginParticleEffect2dEdit();

    LineEdit@ element = eventData["Element"].GetPtr();

    if (element.name == "TangentialAcceleration")
        editParticleEffect2d.SetTangentialAcceleration(element.text.ToFloat());

    if (element.name == "TangentialAccelerationVariance")
        editParticleEffect2d.SetTangentialAccelVariance(element.text.ToFloat());

    EndParticleEffect2dEdit();
}

void EditParticleEffect2dMaxNumParticles(StringHash eventType, VariantMap& eventData)
{
    if (inParticle2dEffectRefresh)
        return;

    if (editParticleEffect2d is null)
        return;

    BeginParticleEffect2dEdit();

    LineEdit@ element = eventData["Element"].GetPtr();

    if (element.name == "MaxNumParticles")
    {
        editParticleEffect2d.SetMaxParticles(element.text.ToInt());
        RefreshParticleEffect2dEmitter();
    }

    EndParticleEffect2dEdit();
}

void EditParticleEffect2dStartColor(StringHash eventType, VariantMap& eventData)
{
    if (inParticle2dEffectRefresh)
        return;

    if (editParticleEffect2d is null)
        return;

    BeginParticleEffect2dEdit();

    LineEdit@ element = eventData["Element"].GetPtr();

    Color v = editParticleEffect2d.GetStartColor();
    Color vVar = editParticleEffect2d.GetStartColorVariance();

    if (element.name == "StartColor_R")
        editParticleEffect2d.SetStartColor(Color(element.text.ToFloat(), v.g, v.b, v.a));

    if (element.name == "StartColor_G")
        editParticleEffect2d.SetStartColor(Color(v.r, element.text.ToFloat(), v.b, v.a));

    if (element.name == "StartColor_B")
        editParticleEffect2d.SetStartColor(Color(v.r, v.g, element.text.ToFloat(), v.a));

    if (element.name == "StartColor_A")
        editParticleEffect2d.SetStartColor(Color(v.r, v.g, v.b, element.text.ToFloat()));


    if (element.name == "StartColorVariance_R")
        editParticleEffect2d.SetStartColorVariance(Color(element.text.ToFloat(), vVar.g, vVar.b, vVar.a));

    if (element.name == "StartColorVariance_G")
        editParticleEffect2d.SetStartColorVariance(Color(vVar.r, element.text.ToFloat(), vVar.b, vVar.a));

    if (element.name == "StartColorVariance_B")
        editParticleEffect2d.SetStartColorVariance(Color(vVar.r, vVar.g, element.text.ToFloat(), vVar.a));

    if (element.name == "StartColorVariance_A")
        editParticleEffect2d.SetStartColorVariance(Color(vVar.r, vVar.g, vVar.b, element.text.ToFloat()));

    EndParticleEffect2dEdit();
}

void EditParticleEffect2dEndColor(StringHash eventType, VariantMap& eventData)
{
    if (inParticle2dEffectRefresh)
        return;

    if (editParticleEffect2d is null)
        return;

    BeginParticleEffect2dEdit();

    LineEdit@ element = eventData["Element"].GetPtr();

    Color v = editParticleEffect2d.GetFinishColor();
    Color vVar = editParticleEffect2d.GetFinishColorVariance();

    if (element.name == "EndColor_R")
        editParticleEffect2d.SetFinishColor(Color(element.text.ToFloat(), v.g, v.b, v.a));

    if (element.name == "EndColor_G")
        editParticleEffect2d.SetFinishColor(Color(v.r, element.text.ToFloat(), v.b, v.a));

    if (element.name == "EndColor_B")
        editParticleEffect2d.SetFinishColor(Color(v.r, v.g, element.text.ToFloat(), v.a));

    if (element.name == "EndColor_A")
        editParticleEffect2d.SetFinishColor(Color(v.r, v.g, v.b, element.text.ToFloat()));


    if (element.name == "EndColorVariance_R")
        editParticleEffect2d.SetFinishColorVariance(Color(element.text.ToFloat(), vVar.g, vVar.b, vVar.a));

    if (element.name == "EndColorVariance_G")
        editParticleEffect2d.SetFinishColorVariance(Color(vVar.r, element.text.ToFloat(), vVar.b, vVar.a));

    if (element.name == "EndColorVariance_B")
        editParticleEffect2d.SetFinishColorVariance(Color(vVar.r, vVar.g, element.text.ToFloat(), vVar.a));

    if (element.name == "EndColorVariance_A")
        editParticleEffect2d.SetFinishColorVariance(Color(vVar.r, vVar.g, vVar.b, element.text.ToFloat()));

    EndParticleEffect2dEdit();
}

void EditParticleEffect2dEmitterType(StringHash eventType, VariantMap& eventData)
{
    if (inParticle2dEffectRefresh)
        return;

    if (editParticleEffect2d is null)
        return;

    BeginParticleEffect2dEdit();

    DropDownList@ element = eventData["Element"].GetPtr();

    if (element.name == "EmitterType")
    {
    switch (element.selection)
    {
        case 0:
            editParticleEffect2d.SetEmitterType(EMITTER_TYPE_GRAVITY);
            break;

        case 1:
            editParticleEffect2d.SetEmitterType(EMITTER_TYPE_RADIAL);
            break;

    }

    RefreshEmitterTypeDependentElements();
    }

    if (element.name == "BlendMode")
    {
    switch (element.selection)
    {
        case 0:
            editParticleEffect2d.SetBlendMode(BLEND_REPLACE);
            break;

        case 1:
            editParticleEffect2d.SetBlendMode(BLEND_ADD);
            break;

        case 2:
            editParticleEffect2d.SetBlendMode(BLEND_MULTIPLY);
            break;

        case 3:
            editParticleEffect2d.SetBlendMode(BLEND_ALPHA);
            break;

        case 4:
            editParticleEffect2d.SetBlendMode(BLEND_ADDALPHA);
            break;

        case 5:
            editParticleEffect2d.SetBlendMode(BLEND_PREMULALPHA);
            break;

        case 6:
            editParticleEffect2d.SetBlendMode(BLEND_INVDESTALPHA);
            break;

        case 7:
            editParticleEffect2d.SetBlendMode(BLEND_SUBTRACT);
            break;

        case 8:
            editParticleEffect2d.SetBlendMode(BLEND_SUBTRACTALPHA);
            break;

    }

        RefreshParticleEffect2dEmitter();
    }


    EndParticleEffect2dEdit();
}

bool ToggleParticleEffectEditor2d()
{
    if (particleEffect2dWindow.visible == false)
        ShowParticleEffectEditor2d();
    else
        HideParticleEffectEditor2d();
    return true;
}

void ShowParticleEffectEditor2d()
{
    RefreshParticleEffectEditor2d();
    particleEffect2dWindow.visible = true;
    particleEffect2dWindow.BringToFront();
}

void HideParticleEffectEditor2d()
{
    if (particleEffect2dWindow !is null)
        particleEffect2dWindow.visible = false;
}

void UpdateParticleEffect2dPreviewGrid()
{
    uint gridSize = 8;
    uint gridSubdivisions = 3;

    //particle2dEffectPreviewGridNode.scale = Vector3(gridScale, gridScale, gridScale);
    uint size = uint(Floor(gridSize / 2) * 2);
    float halfSizeScaled = size / 2;
    float scale = 1.0;
    uint subdivisionSize = uint(Pow(2.0f, float(gridSubdivisions)));

    if (subdivisionSize > 0)
    {
        size *= subdivisionSize;
        scale /= subdivisionSize;
    }

    uint halfSize = size / 2;

    particle2dEffectPreviewGrid.BeginGeometry(0, LINE_LIST);
    float lineOffset = -halfSizeScaled;
    for (uint i = 0; i <= size; ++i)
    {
        bool lineCenter = i == halfSize;
        bool lineSubdiv = !Equals(Mod(i, subdivisionSize), 0.0);

        particle2dEffectPreviewGrid.DefineVertex(Vector3(lineOffset, halfSizeScaled, 0.0));
        particle2dEffectPreviewGrid.DefineColor(lineCenter ? gridYColor : (lineSubdiv ? gridSubdivisionColor : gridColor));
        particle2dEffectPreviewGrid.DefineVertex(Vector3(lineOffset, -halfSizeScaled, 0.0));
        particle2dEffectPreviewGrid.DefineColor(lineCenter ? gridYColor : (lineSubdiv ? gridSubdivisionColor : gridColor));

        particle2dEffectPreviewGrid.DefineVertex(Vector3(-halfSizeScaled, lineOffset, 0.0));
        particle2dEffectPreviewGrid.DefineColor(lineCenter ? gridXColor : (lineSubdiv ? gridSubdivisionColor : gridColor));
        particle2dEffectPreviewGrid.DefineVertex(Vector3(halfSizeScaled, lineOffset, 0.0));
        particle2dEffectPreviewGrid.DefineColor(lineCenter ? gridXColor : (lineSubdiv ? gridSubdivisionColor : gridColor));

        lineOffset  += scale;
    }
    particle2dEffectPreviewGrid.Commit();
    
}

void InitParticleEffectPreview2d()
{
    particle2dPreviewScene = Scene("particle2dPreviewScene");
    particle2dPreviewScene.CreateComponent("Octree");

    Node@ zoneNode = particle2dPreviewScene.CreateChild("Zone");
    Zone@ zone = zoneNode.CreateComponent("Zone");
    zone.boundingBox = BoundingBox(-1000, 1000);
    zone.ambientColor = Color(0.15, 0.15, 0.15);
    zone.fogColor = Color(0, 0, 0);
    zone.fogStart = 10.0;
    zone.fogEnd = 1000.0;

    particle2dPreviewCameraNode = particle2dPreviewScene.CreateChild("PreviewCamera");
    particle2dPreviewCamera = particle2dPreviewCameraNode.CreateComponent("Camera");
    particle2dPreviewCamera.nearClip = 0.1f;
    particle2dPreviewCamera.farClip = 1000.0f;
    particle2dPreviewCamera.fov = 45.0f;

    particle2dPreviewLightNode = particle2dPreviewScene.CreateChild("particle2dPreviewLight");
    particle2dPreviewLightNode.direction = Vector3(0.5, -0.5, 0.5);
    particle2dPreviewLight = particle2dPreviewLightNode.CreateComponent("Light");
    particle2dPreviewLight.lightType = LIGHT_DIRECTIONAL;
    particle2dPreviewLight.specularIntensity = 0.5;

    particle2dEffectPreviewNode = particle2dPreviewScene.CreateChild("PreviewEmitter");
    particle2dEffectPreviewNode.rotation = Quaternion(0, 0, 0);
    
    Particle2d_ResetCameraTransformation();

    particle2dEffectPreviewGizmoNode = particle2dPreviewScene.CreateChild("Gizmo");
    StaticModel@ gizmo = particle2dEffectPreviewGizmoNode.CreateComponent("StaticModel");
    gizmo.model = cache.GetResource("Model", "Models/Editor/Axes.mdl");
    gizmo.materials[0] = cache.GetResource("Material", "Materials/Editor/RedUnlit.xml");
    gizmo.materials[1] = cache.GetResource("Material", "Materials/Editor/GreenUnlit.xml");
    gizmo.materials[2] = cache.GetResource("Material", "Materials/Editor/BlueUnlit.xml");
    gizmo.occludee = false;

    particle2dEffectPreviewGridNode = particle2dPreviewScene.CreateChild("Grid");
    particle2dEffectPreviewGrid = particle2dEffectPreviewGridNode.CreateComponent("CustomGeometry");
    particle2dEffectPreviewGrid.numGeometries = 1;
    particle2dEffectPreviewGrid.material = cache.GetResource("Material", "Materials/VColUnlit.xml");
    particle2dEffectPreviewGrid.viewMask = 0x80000000; // Editor raycasts use viewmask 0x7fffffff
    particle2dEffectPreviewGrid.occludee = false;

    UpdateParticleEffect2dPreviewGrid();

    particleEffect2dEmitter = particle2dEffectPreviewNode.CreateComponent("ParticleEmitter2D");
    editParticleEffect2d = CreateNewParticleEffect2d();
    particleEffect2dEmitter.SetEffect(editParticleEffect2d);

    particle2dEffectPreview = particleEffect2dWindow.GetChild("ParticleEffectPreview", true);
    particle2dEffectPreview.SetView(particle2dPreviewScene, particle2dPreviewCamera);
    particle2dEffectPreview.viewport.renderPath = renderPath;

    SubscribeToEvent(particle2dEffectPreview, "DragMove", "NavigateParticleEffect2dPreview");
    SubscribeToEvent(particle2dEffectPreview, "Resized", "ResizeParticleEffect2dPreview");
}

ParticleEffect2D@ CreateNewParticleEffect2d()
{
    ParticleEffect2D@ effect = ParticleEffect2D();
    Sprite2D@ res = cache.GetResource("Sprite2D", "Urho2D/sun.png");
    if (res is null)
        log.Error("Could not load default sprite for new 2d particle effect.");

    effect.SetSprite(res);
    return effect;
}

void EditParticleEffect2d(ParticleEffect2D@ effect)
{
    if (effect is null)
        return;

    if (editParticleEffect2d !is null)
        UnsubscribeFromEvent(editParticleEffect2d, "ReloadFinished");

    if (!effect.name.empty)
        cache.ReloadResource(effect);
        
    editParticleEffect2d = effect;
    particleEffect2dEmitter.effect = editParticleEffect2d;
    RefreshParticleEffect2dEmitter();

    if (editParticleEffect2d !is null)
        SubscribeToEvent(editParticleEffect2d, "ReloadFinished", "RefreshParticleEffectEditor2d");

    ShowParticleEffectEditor2d();
}

void RefreshParticleEffect2dEmitter()
{
    if (editParticleEffect2d is null || particleEffect2dEmitter is null)
        return;

    particleEffect2dEmitter.SetSprite(editParticleEffect2d.GetSprite());
    particleEffect2dEmitter.SetBlendMode(editParticleEffect2d.GetBlendMode());
    particleEffect2dEmitter.SetMaxParticles(editParticleEffect2d.GetMaxParticles());

    particleEffect2dEmitter.Reset();
}

void RefreshParticleEffectEditor2d()
{
    inParticle2dEffectRefresh = true;

    RefreshParticleEffect2dPreview();
    RefreshParticleEffect2dName();
    RefreshParticleEffect2dSprite();
    RefreshParticleEffect2dBasicAttributes();
    RefreshEmitterTypeDependentElements();

    inParticle2dEffectRefresh = false;
}

void RefreshParticleEffect2dPreview()
{
    if (particleEffect2dEmitter is null || editParticleEffect2d is null)
        return;
    cast<CheckBox>(particleEffect2dWindow.GetChild("ShowGrid", true)).checked = showParticle2dPreviewAxes;
    cast<CheckBox>(particleEffect2dWindow.GetChild("LoopEmission", true)).checked = particle2dLoopEmission;
    particleEffect2dEmitter.effect = editParticleEffect2d;
    RefreshParticleEffect2dEmitter();
    particle2dEffectPreview.QueueUpdate();
}

void RefreshParticleEffect2dName()
{
    UIElement@ container = particleEffect2dWindow.GetChild("NameContainer", true);
    if (container is null)
        return;
        
    container.RemoveAllChildren();

    LineEdit@ nameEdit = CreateAttributeLineEdit(container, null, 0, 0);
    if (editParticleEffect2d !is null)
        nameEdit.text = editParticleEffect2d.name;
    SubscribeToEvent(nameEdit, "TextFinished", "EditParticleEffect2dName");

    Button@ pickButton = CreateResourcePickerButton(container, null, 0, 0, "smallButtonPick");
    SubscribeToEvent(pickButton, "Released", "PickEditParticleEffect2d");
}


void RefreshParticleEffect2dBasicAttributes()
{
    if (editParticleEffect2d is null)
        return;

    cast<LineEdit>(particleEffect2dWindow.GetChild("Speed", true)).text = editParticleEffect2d.GetSpeed();
    cast<LineEdit>(particleEffect2dWindow.GetChild("SpeedVariance", true)).text = editParticleEffect2d.GetSpeedVariance();
    cast<LineEdit>(particleEffect2dWindow.GetChild("ParticleLifespan", true)).text = editParticleEffect2d.GetParticleLifeSpan();
    cast<LineEdit>(particleEffect2dWindow.GetChild("ParticleLifespanVariance", true)).text = editParticleEffect2d.GetParticleLifespanVariance();

    cast<LineEdit>(particleEffect2dWindow.GetChild("ParticleAngle", true)).text = editParticleEffect2d.GetAngle();
    cast<LineEdit>(particleEffect2dWindow.GetChild("ParticleAngleVariance", true)).text = editParticleEffect2d.GetAngleVariance();

    cast<LineEdit>(particleEffect2dWindow.GetChild("GravityX", true)).text = editParticleEffect2d.GetGravity().x;
    cast<LineEdit>(particleEffect2dWindow.GetChild("GravityY", true)).text = editParticleEffect2d.GetGravity().y;

    cast<LineEdit>(particleEffect2dWindow.GetChild("RadialAcceleration", true)).text = editParticleEffect2d.GetRadialAcceleration();
    cast<LineEdit>(particleEffect2dWindow.GetChild("RadialAccelerationVariance", true)).text = editParticleEffect2d.GetRadialAccelVariance();
    cast<LineEdit>(particleEffect2dWindow.GetChild("TangentialAcceleration", true)).text = editParticleEffect2d.GetTangentialAcceleration();
    cast<LineEdit>(particleEffect2dWindow.GetChild("TangentialAccelerationVariance", true)).text = editParticleEffect2d.GetTangentialAccelVariance();

    cast<LineEdit>(particleEffect2dWindow.GetChild("StartColor_R", true)).text = editParticleEffect2d.GetStartColor().r;
    cast<LineEdit>(particleEffect2dWindow.GetChild("StartColor_G", true)).text = editParticleEffect2d.GetStartColor().g;
    cast<LineEdit>(particleEffect2dWindow.GetChild("StartColor_B", true)).text = editParticleEffect2d.GetStartColor().b;
    cast<LineEdit>(particleEffect2dWindow.GetChild("StartColor_A", true)).text = editParticleEffect2d.GetStartColor().a;

    cast<LineEdit>(particleEffect2dWindow.GetChild("StartColorVariance_R", true)).text = editParticleEffect2d.GetStartColorVariance().r;
    cast<LineEdit>(particleEffect2dWindow.GetChild("StartColorVariance_G", true)).text = editParticleEffect2d.GetStartColorVariance().g;
    cast<LineEdit>(particleEffect2dWindow.GetChild("StartColorVariance_B", true)).text = editParticleEffect2d.GetStartColorVariance().b;
    cast<LineEdit>(particleEffect2dWindow.GetChild("StartColorVariance_A", true)).text = editParticleEffect2d.GetStartColorVariance().a;

    cast<LineEdit>(particleEffect2dWindow.GetChild("EndColor_R", true)).text = editParticleEffect2d.GetFinishColor().r;
    cast<LineEdit>(particleEffect2dWindow.GetChild("EndColor_G", true)).text = editParticleEffect2d.GetFinishColor().g;
    cast<LineEdit>(particleEffect2dWindow.GetChild("EndColor_B", true)).text = editParticleEffect2d.GetFinishColor().b;
    cast<LineEdit>(particleEffect2dWindow.GetChild("EndColor_A", true)).text = editParticleEffect2d.GetFinishColor().a;

    cast<LineEdit>(particleEffect2dWindow.GetChild("EndColorVariance_R", true)).text = editParticleEffect2d.GetFinishColorVariance().r;
    cast<LineEdit>(particleEffect2dWindow.GetChild("EndColorVariance_G", true)).text = editParticleEffect2d.GetFinishColorVariance().g;
    cast<LineEdit>(particleEffect2dWindow.GetChild("EndColorVariance_B", true)).text = editParticleEffect2d.GetFinishColorVariance().b;
    cast<LineEdit>(particleEffect2dWindow.GetChild("EndColorVariance_A", true)).text = editParticleEffect2d.GetFinishColorVariance().a;

    cast<LineEdit>(particleEffect2dWindow.GetChild("MaxNumParticles", true)).text = editParticleEffect2d.GetMaxParticles();

    cast<LineEdit>(particleEffect2dWindow.GetChild("SourcePosVarianceX", true)).text = editParticleEffect2d.GetSourcePositionVariance().x;
    cast<LineEdit>(particleEffect2dWindow.GetChild("SourcePosVarianceY", true)).text = editParticleEffect2d.GetSourcePositionVariance().y
    ;

    cast<LineEdit>(particleEffect2dWindow.GetChild("StartParticleSize", true)).text = editParticleEffect2d.GetStartParticleSize();
    cast<LineEdit>(particleEffect2dWindow.GetChild("StartParticleSizeVariance", true)).text = editParticleEffect2d.GetStartParticleSizeVariance();
    cast<LineEdit>(particleEffect2dWindow.GetChild("EndParticleSize", true)).text = editParticleEffect2d.GetFinishParticleSize();
    cast<LineEdit>(particleEffect2dWindow.GetChild("EndParticleSizeVariance", true)).text = editParticleEffect2d.GetFinishParticleSizeVariance();

    cast<LineEdit>(particleEffect2dWindow.GetChild("Duration", true)).text = editParticleEffect2d.GetDuration();

    cast<LineEdit>(particleEffect2dWindow.GetChild("MinRadius", true)).text = editParticleEffect2d.GetMinRadius();
    cast<LineEdit>(particleEffect2dWindow.GetChild("MinRadiusVariance", true)).text = editParticleEffect2d.GetMinRadiusVariance();
    cast<LineEdit>(particleEffect2dWindow.GetChild("MaxRadius", true)).text = editParticleEffect2d.GetMaxRadius();
    cast<LineEdit>(particleEffect2dWindow.GetChild("MaxRadiusVariance", true)).text = editParticleEffect2d.GetMaxRadiusVariance();

    cast<LineEdit>(particleEffect2dWindow.GetChild("RotationPerSecond", true)).text = editParticleEffect2d.GetRotatePerSecond();
    cast<LineEdit>(particleEffect2dWindow.GetChild("RotationPerSecondVariance", true)).text = editParticleEffect2d.GetRotatePerSecondVariance();
    cast<LineEdit>(particleEffect2dWindow.GetChild("RotationStart", true)).text = editParticleEffect2d.GetRotationStart();
    cast<LineEdit>(particleEffect2dWindow.GetChild("RotationStartVariance", true)).text = editParticleEffect2d.GetRotationStartVariance();
    cast<LineEdit>(particleEffect2dWindow.GetChild("RotationEnd", true)).text = editParticleEffect2d.GetRotationEnd();
    cast<LineEdit>(particleEffect2dWindow.GetChild("RotationEndVariance", true)).text = editParticleEffect2d.GetRotationEndVariance();

    switch (editParticleEffect2d.GetEmitterType())
    {
        case EMITTER_TYPE_GRAVITY:
            cast<DropDownList>(particleEffect2dWindow.GetChild("EmitterType", true)).selection = 0;
            break;
        case EMITTER_TYPE_RADIAL:
            cast<DropDownList>(particleEffect2dWindow.GetChild("EmitterType", true)).selection = 1;
            break;
    }

    switch (editParticleEffect2d.GetBlendMode())
    {
        case BLEND_REPLACE:
            cast<DropDownList>(particleEffect2dWindow.GetChild("BlendMode", true)).selection = 0;
            break;
        case BLEND_ADD:
            cast<DropDownList>(particleEffect2dWindow.GetChild("BlendMode", true)).selection = 1;
            break;
        case BLEND_MULTIPLY:
            cast<DropDownList>(particleEffect2dWindow.GetChild("BlendMode", true)).selection = 2;
            break;
        case BLEND_ALPHA:
            cast<DropDownList>(particleEffect2dWindow.GetChild("BlendMode", true)).selection = 3;
            break;
        case BLEND_ADDALPHA:
            cast<DropDownList>(particleEffect2dWindow.GetChild("BlendMode", true)).selection = 4;
            break;
        case BLEND_PREMULALPHA:
            cast<DropDownList>(particleEffect2dWindow.GetChild("BlendMode", true)).selection = 5;
            break;
        case BLEND_INVDESTALPHA:
            cast<DropDownList>(particleEffect2dWindow.GetChild("BlendMode", true)).selection = 6;
            break;
        case BLEND_SUBTRACT:
            cast<DropDownList>(particleEffect2dWindow.GetChild("BlendMode", true)).selection = 7;
            break;
        case BLEND_SUBTRACTALPHA:
            cast<DropDownList>(particleEffect2dWindow.GetChild("BlendMode", true)).selection = 8;
            break;
    }
}

void RefreshEmitterTypeDependentElements()
{
    UIElement@ radialOnlyContent = particleEffect2dWindow.GetChild("RadialOnlyContent", true);
    UIElement@ gravityOnlyContent = particleEffect2dWindow.GetChild("GravityOnlyContent", true);

    if (editParticleEffect2d is null)
    {
        // gravity starts visible
        radialOnlyContent.SetVisible(false);
        gravityOnlyContent.SetVisible(true);
        return;
    }

    switch (editParticleEffect2d.GetEmitterType())
    {
        case EMITTER_TYPE_GRAVITY:
            radialOnlyContent.SetVisible(false);
            gravityOnlyContent.SetVisible(true);
            break;
        case EMITTER_TYPE_RADIAL:
            radialOnlyContent.SetVisible(true);
            gravityOnlyContent.SetVisible(false);
            break;
    }

    // force a layout update of the scroll view
    radialOnlyContent.GetParent().SetHeight(0);
}

void RefreshParticleEffect2dSprite()
{
    UIElement@ container = particleEffect2dWindow.GetChild("ParticleSpriteContainer", true);
    if (container is null)
        return;

    container.RemoveAllChildren();

    LineEdit@ nameEdit = CreateAttributeLineEdit(container, null, 0, 0);
    if (editParticleEffect2d !is null)
    {
        if (editParticleEffect2d.GetSprite() !is null)
            nameEdit.text = editParticleEffect2d.GetSprite().name;
        else
        {
            nameEdit.text = "";
        }
    }

    SubscribeToEvent(nameEdit, "TextFinished", "EditParticleEffect2dSprite");

    Button@ pickButton = CreateResourcePickerButton(container, null, 0, 0, "smallButtonPick");
    SubscribeToEvent(pickButton, "Released", "PickEditParticleEffect2dSprite");
}

void NavigateParticleEffect2dPreview(StringHash eventType, VariantMap& eventData)
{
    int dx = eventData["DX"].GetInt();
    int dy = eventData["DY"].GetInt();

    if (particle2dEffectPreview.height > 0 && particle2dEffectPreview.width > 0)
    {
        if (!input.keyDown[KEY_LSHIFT])
        {
            particle2dViewCamRot.x -= dy * 20 * time.timeStep;
            particle2dViewCamRot.y += dx * 20 * time.timeStep;
            particle2dViewCamRot.x = Clamp(particle2dViewCamRot.x, -89.5, 89.5);
        }
        else
        {
            particle2dViewCamDist += dy * 1.5 * time.timeStep;
            particle2dViewCamDist -= dx * 1.5 * time.timeStep;
            particle2dViewCamDist = Max(particle2dViewCamDist, 0.2);
        }
        particle2dPreviewCameraNode.position = particle2dEffectPreviewNode.position +
            Quaternion(particle2dViewCamRot.x, particle2dViewCamRot.y, 0) * particle2dViewCamDir * particle2dViewCamDist;
        particle2dPreviewCameraNode.LookAt(particle2dEffectPreviewNode.position);

        Particle2d_SetGizmoPosition();
        particle2dEffectPreview.QueueUpdate();
    }

}

void ResizeParticleEffect2dPreview(StringHash eventType, VariantMap& eventData)
{
    
    float width = float(particle2dEffectPreview.width);
    float height = float(particle2dEffectPreview.height);
    
    // Manually set aspect ratio because first frame is always returning aspect ratio of 1
    float aspectRatio = width / height;
    particle2dPreviewCamera.aspectRatio = aspectRatio;

    particle2dGizmoOffsetX = particle2dGizmoOffset;
    particle2dGizmoOffsetY = 1.0f - particle2dGizmoOffset * aspectRatio;

    if(width > height)
    {
        aspectRatio = height / width;
        particle2dGizmoOffsetY = 1.0f - particle2dGizmoOffset;
        particle2dGizmoOffsetX = particle2dGizmoOffset * aspectRatio;
    }

    Particle2d_SetGizmoPosition();
    particle2dEffectPreview.QueueUpdate();
}

void EditParticleEffect2dName(StringHash eventType, VariantMap& eventData)
{
    LineEdit@ nameEdit = eventData["Element"].GetPtr();
    String newParticleEffectName = nameEdit.text.Trimmed();
    if (!newParticleEffectName.empty && !(editParticleEffect2d !is null && newParticleEffectName == editParticleEffect2d.name))
    {
        ParticleEffect2D@ newParticleEffect = cache.GetResource("ParticleEffect2D", newParticleEffectName);
        if (newParticleEffect !is null)
            EditParticleEffect2d(newParticleEffect);
    }
}

void PickEditParticleEffect2d()
{
    @resourcePicker = GetResourcePicker(StringHash("ParticleEffect2D"));
    if (resourcePicker is null)
        return;

    String lastPath = resourcePicker.lastPath;
    if (lastPath.empty)
        lastPath = sceneResourcePath;
    CreateFileSelector(localization.Get("Pick ") + resourcePicker.typeName, "OK", "Cancel", lastPath, resourcePicker.filters, resourcePicker.lastFilter, false);
    SubscribeToEvent(uiFileSelector, "FileSelected", "PickEditParticleEffect2dDone");
}

void PickEditParticleEffect2dDone(StringHash eventType, VariantMap& eventData)
{
    StoreResourcePickerPath();
    CloseFileSelector();

    if (!eventData["OK"].GetBool())
    {
        @resourcePicker = null;
        return;
    }

    String resourceName = eventData["FileName"].GetString();
    Resource@ res = GetPickedResource(resourceName);

    if (res !is null)
        EditParticleEffect2d(cast<ParticleEffect2D>(res));

    @resourcePicker = null;
}

void EditParticleEffect2dSprite(StringHash eventType, VariantMap& eventData)
{
    LineEdit@ nameEdit = eventData["Element"].GetPtr();
    String newSpriteText = nameEdit.text.Trimmed();
    if (!newSpriteText.empty && editParticleEffect2d !is null)
    {
        Sprite2D@ newSprite = cache.GetResource("Sprite2D", newSpriteText);
        if (newSprite !is null)
        {
            editParticleEffect2d.SetSprite(newSprite);
            RefreshParticleEffect2dSprite();
            RefreshParticleEffect2dEmitter();
        }
    }
}

void PickEditParticleEffect2dSprite(StringHash eventType, VariantMap& eventData)
{
    @resourcePicker = GetResourcePicker(StringHash("Sprite2D"));
    if (resourcePicker is null)
        return;

    String lastPath = resourcePicker.lastPath;
    if (lastPath.empty)
        lastPath = sceneResourcePath;
    CreateFileSelector(localization.Get("Pick ") + resourcePicker.typeName, "OK", "Cancel", lastPath, resourcePicker.filters, resourcePicker.lastFilter, false);
    SubscribeToEvent(uiFileSelector, "FileSelected", "PickEditParticleEffect2dSpriteDone");
}

void PickEditParticleEffect2dSpriteDone(StringHash eventType, VariantMap& eventData)
{
    StoreResourcePickerPath();
    CloseFileSelector();

    if (!eventData["OK"].GetBool())
    {
        @resourcePicker = null;
        return;
    }

    String resourceName = eventData["FileName"].GetString();
    Resource@ res = GetPickedResource(resourceName);

    if (res !is null && editParticleEffect2d !is null)
    {
        editParticleEffect2d.SetSprite(res);
        RefreshParticleEffect2dSprite();
        RefreshParticleEffect2dEmitter();
    }

    @resourcePicker = null;
}

void NewParticleEffect2d()
{
    BeginParticleEffect2dEdit();

    EditParticleEffect2d(CreateNewParticleEffect2d());
    
    EndParticleEffect2dEdit();
}

void RevertParticleEffect2d()
{
    if (inParticle2dEffectRefresh)
        return;

    if (editParticleEffect2d is null)
        return;

    if (editParticleEffect2d.name.empty)
    {
        NewParticleEffect2d();
        return;
    }

    BeginParticleEffect2dEdit();
    
    cache.ReloadResource(editParticleEffect2d);

    EndParticleEffect2dEdit();
    
    RefreshParticleEffectEditor2d();
}

void SaveParticleEffect2d()
{
    if (editParticleEffect2d is null || editParticleEffect2d.name.empty)
        return;

    String fullName = cache.GetResourceFileName(editParticleEffect2d.name);
    if (fullName.empty)
        return;

    File saveFile(fullName, FILE_WRITE);
    editParticleEffect2d.Save(saveFile);
}

void SaveParticleEffect2dAs()
{
    if (editParticleEffect2d is null)
        return;

    @resourcePicker = GetResourcePicker(StringHash("ParticleEffect2D"));
    if (resourcePicker is null)
        return;

    String lastPath = resourcePicker.lastPath;
    if (lastPath.empty)
        lastPath = sceneResourcePath;
    CreateFileSelector("Save particle effect 2D as", "Save", "Cancel", lastPath, resourcePicker.filters, resourcePicker.lastFilter);
    SubscribeToEvent(uiFileSelector, "FileSelected", "SaveParticleEffect2dAsDone");
}

void SaveParticleEffect2dAsDone(StringHash eventType, VariantMap& eventData)
{
    StoreResourcePickerPath();
    CloseFileSelector();
    @resourcePicker = null;

    if (editParticleEffect2d is null)
        return;

    if (!eventData["OK"].GetBool())
    {
        @resourcePicker = null;
        return;
    }

    String fullName = eventData["FileName"].GetString();

    // Add default extension for saving if not specified
    String filter = eventData["Filter"].GetString();
    if (GetExtension(fullName).empty && filter != "*.*")
        fullName = fullName + filter.Substring(1);

    File saveFile(fullName, FILE_WRITE);
    if (editParticleEffect2d.Save(saveFile))
    {
        saveFile.Close();

        // Load the new resource to update the name in the editor
        ParticleEffect2D@ newEffect = cache.GetResource("ParticleEffect2D", GetResourceNameFromFullName(fullName));
        if (newEffect !is null)
            EditParticleEffect2d(newEffect);
    }
}

void BeginParticleEffect2dEdit()
{
    if (editParticleEffect2d is null)
        return;

    inParticle2dEffectRefresh = true;

    oldParticleEffect2dState = XMLFile();
    XMLElement particleElem = oldParticleEffect2dState.CreateRoot("particleEmitterConfig");
    editParticleEffect2d.Save(particleElem);
}

void EndParticleEffect2dEdit()
{
    if (editParticleEffect2d is null)
        return;

    if (!dragEditAttribute)
    {
        EditParticleEffect2dAction@ action = EditParticleEffect2dAction();
        action.Define(particleEffect2dEmitter, editParticleEffect2d, oldParticleEffect2dState);
        SaveEditAction(action);
    }

    inParticle2dEffectRefresh = false;
    
    particle2dEffectPreview.QueueUpdate();
}

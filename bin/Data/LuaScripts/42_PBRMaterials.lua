-- PBR materials example.
-- This sample demonstrates:
--      - Loading a scene that showcases physically based materials & shaders
--
-- To use with deferred rendering, a PBR deferred renderpath should be chosen:
-- CoreData/RenderPaths/PBRDeferred.xml or CoreData/RenderPaths/PBRDeferredHWDepth.xml

require "LuaScripts/Utilities/Sample"

local dynamicMaterial = nil
local roughnessLabel = nil
local metallicLabel = nil
local ambientLabel = nil
local taaLabel = nil
local zone = nil
local effectRenderPath = nil
local currentJitter = Vector2(0, 0)
local previousCameraPosition = Vector3(0, 0, 0)
local previousViewSize = IntVector2(0, 0)
local previousViewProj = Matrix4()
local previousCameraDirection = Vector3(0, 0, 1)
local previousFarClip = 0
local taaFrame = 0
local taaStrength = 0.5
local cullCameraNode = nil

function Start()
    -- Execute the common startup for samples
    SampleStart()

    -- Create the scene content
    CreateScene()

    -- Create the UI content
    CreateUI()
    
    CreateInstructions()

    -- Setup the viewport for displaying the scene
    SetupViewport()

    -- Subscribe to global events for camera movement
    SubscribeToEvents()
end

function CreateInstructions()
    -- Construct new Text object, set string to display and font to use
    local instructionText = ui.root:CreateChild("Text")
    instructionText:SetText("Use sliders to change Roughness and Metallic\n" ..
                            "Hold RMB and use WASD keys and mouse to move")
    instructionText:SetFont(cache:GetResource("Font", "Fonts/Anonymous Pro.ttf"), 15)

    -- Position the text relative to the screen center
    instructionText.horizontalAlignment = HA_CENTER
    instructionText.verticalAlignment = VA_CENTER
    instructionText:SetPosition(0, ui.root.height / 4)
end

function CreateScene()
    scene_ = Scene()

    -- Load scene content prepared in the editor (XML format). GetFile() returns an open file from the resource system
    -- which scene.LoadXML() will read
    local file = cache:GetFile("Scenes/PBRExample.xml")
    scene_:LoadXML(file)
    -- In Lua the file returned by GetFile() needs to be deleted manually
    file:delete()
    
    local sphereWithDynamicMatNode = scene_:GetChild("SphereWithDynamicMat")
    local staticModel = sphereWithDynamicMatNode:GetComponent("StaticModel")
    dynamicMaterial = staticModel:GetMaterial(0)

    local zoneNode = scene_:GetChild("Zone")
    zone = zoneNode:GetComponent("Zone")

    -- Create the camera (not included in the scene file)
    cameraNode = scene_:CreateChild("Camera")
    cameraNode:CreateComponent("Camera")

    cameraNode.position = sphereWithDynamicMatNode.position + Vector3(2.0, 2.0, 2.0)
    cameraNode:LookAt(sphereWithDynamicMatNode.position)
    yaw = cameraNode.rotation:YawAngle()
    pitch = cameraNode.rotation:PitchAngle()
end

function CreateUI()
    -- Set up global UI style into the root UI element
    local style = cache:GetResource("XMLFile", "UI/DefaultStyle.xml")
    ui.root.defaultStyle = style

    -- Create a Cursor UI element because we want to be able to hide and show it at will. When hidden, the mouse cursor will
    -- control the camera, and when visible, it will interact with the UI
    local cursor = ui.root:CreateChild("Cursor")
    cursor:SetStyleAuto()
    ui.cursor = cursor
    -- Set starting position of the cursor at the rendering window center
    cursor:SetPosition(graphics.width / 2, graphics.height / 2)

    roughnessLabel = ui.root:CreateChild("Text")
    roughnessLabel:SetFont(cache:GetResource("Font", "Fonts/Anonymous Pro.ttf"), 15)
    roughnessLabel:SetPosition(370, 50)
    roughnessLabel.textEffect = TE_SHADOW

    metallicLabel = ui.root:CreateChild("Text")
    metallicLabel:SetFont(cache:GetResource("Font", "Fonts/Anonymous Pro.ttf"), 15)
    metallicLabel:SetPosition(370, 100)
    metallicLabel.textEffect = TE_SHADOW

    ambientLabel = ui.root:CreateChild("Text")
    ambientLabel:SetFont(cache:GetResource("Font", "Fonts/Anonymous Pro.ttf"), 15)
    ambientLabel:SetPosition(370, 150)
    ambientLabel.textEffect = TE_SHADOW

    taaLabel = ui.root:CreateChild("Text")
    taaLabel:SetFont(cache:GetResource("Font", "Fonts/Anonymous Pro.ttf"), 15)
    taaLabel:SetPosition(370, 200)
    taaLabel.textEffect = TE_SHADOW

    local roughnessSlider = ui.root:CreateChild("Slider")
    roughnessSlider:SetStyleAuto()
    roughnessSlider:SetPosition(50, 50)
    roughnessSlider:SetSize(300, 20)
    roughnessSlider.range = 1.0    -- 0 - 1 range
    SubscribeToEvent(roughnessSlider, "SliderChanged", "HandleRoughnessSliderChanged")
    roughnessSlider.value = 0.5
    
    local metallicSlider = ui.root:CreateChild("Slider")
    metallicSlider:SetStyleAuto()
    metallicSlider:SetPosition(50, 100)
    metallicSlider:SetSize(300, 20)
    metallicSlider.range = 1.0    -- 0 - 1 range
    SubscribeToEvent(metallicSlider, "SliderChanged", "HandleMetallicSliderChanged")
    metallicSlider.value = 0.5

    local ambientSlider = ui.root:CreateChild("Slider")
    ambientSlider:SetStyleAuto()
    ambientSlider:SetPosition(50, 150)
    ambientSlider:SetSize(300, 20)
    ambientSlider.range = 10.0    -- 0 - 10 range
    SubscribeToEvent(ambientSlider, "SliderChanged", "HandleAmbientSliderChanged")
    ambientSlider.value = zone.ambientColor.a

    local taaSlider = ui.root:CreateChild("Slider")
    taaSlider:SetStyleAuto()
    taaSlider:SetPosition(50, 200)
    taaSlider:SetSize(300, 20)
    taaSlider.range = 0.95
    SubscribeToEvent(taaSlider, "SliderChanged", "HandleTAASliderChanged")
    taaSlider.value = taaStrength
end

function HandleRoughnessSliderChanged(eventType, eventData)
    local newValue = eventData["Value"]:GetFloat()
    dynamicMaterial:SetShaderParameter("Roughness", Variant(newValue))
    roughnessLabel.text = "Roughness: " .. newValue
end

function HandleMetallicSliderChanged(eventType, eventData)
    local newValue = eventData["Value"]:GetFloat()
    dynamicMaterial:SetShaderParameter("Metallic", Variant(newValue))
    metallicLabel.text = "Metallic: " .. newValue
end

function HandleAmbientSliderChanged(eventType, eventData)
    local newValue = eventData["Value"]:GetFloat()
    local col = Color(0, 0, 0, newValue)
    zone.ambientColor = col
    ambientLabel.text = "Ambient HDR Scale: " .. zone.ambientColor.a
end

function HandleTAASliderChanged(eventType, eventData)
    taaStrength = eventData["Value"]:GetFloat()
    taaLabel.text = "TAA Strength: " .. taaStrength
end

function SetupViewport()
    renderer.hdrRendering = true

    -- Set up a viewport to the Renderer subsystem so that the 3D scene can be seen
    local viewport = Viewport:new(scene_, cameraNode:GetComponent("Camera"))
    viewport:SetRenderPath(cache:GetResource("XMLFile", "RenderPaths/ForwardDepth.xml"))
    cullCameraNode = scene_:CreateChild("TAACullCamera")
    cullCameraNode.worldPosition = cameraNode.worldPosition
    cullCameraNode.worldRotation = cameraNode.worldRotation
    viewport.cullCamera = cullCameraNode:CreateComponent("Camera")
    renderer:SetViewport(0, viewport)

    -- Add post-processing effects appropriate with the example scene
    effectRenderPath = viewport:GetRenderPath():Clone()
    effectRenderPath:Append(cache:GetResource("XMLFile", "PostProcess/AutoExposure.xml"))
    effectRenderPath:Append(cache:GetResource("XMLFile", "PostProcess/TAA.xml"))
    effectRenderPath:Append(cache:GetResource("XMLFile", "PostProcess/Tonemap.xml"))
    effectRenderPath:Append(cache:GetResource("XMLFile", "PostProcess/GammaCorrection.xml"))

    viewport.renderPath = effectRenderPath
end

function UpdateTAA()
    local pattern = {
        Vector2(0, -0.166667), Vector2(-0.25, 0.166667), Vector2(0.25, -0.388889),
        Vector2(-0.375, -0.055556), Vector2(0.125, 0.277778), Vector2(-0.125, -0.277778),
        Vector2(0.375, 0.055556), Vector2(-0.4375, 0.388889)
    }
    local sample = pattern[taaFrame % #pattern + 1]
    local cameraPosition = cameraNode.worldPosition
    local cameraRotation = cameraNode.worldRotation
    local viewSize = IntVector2(Max(graphics.width, 1), Max(graphics.height, 1))
    local viewResized = viewSize ~= previousViewSize
    currentJitter = Vector2(sample.x / viewSize.x, sample.y / viewSize.y)
    local camera = cameraNode:GetComponent("Camera")
    camera.projectionOffset = currentJitter
    local currentViewProj = camera:GetProjection() * camera:GetView()
    local currentCameraDirection = cameraNode.worldDirection
    local historyWeight = taaFrame ~= 0 and not viewResized and taaStrength or 0.0
    effectRenderPath:SetShaderParameter("TAAParams", Vector4(historyWeight, 0.02, 0, 0))
    effectRenderPath:SetShaderParameter("PrevViewProj", taaFrame ~= 0 and previousViewProj or currentViewProj)
    effectRenderPath:SetShaderParameter("PrevCameraPos", Vector4(previousCameraPosition,
        previousFarClip > 0 and previousFarClip or camera.farClip))
    effectRenderPath:SetShaderParameter("PrevCameraDir", Vector4(
        taaFrame ~= 0 and previousCameraDirection or currentCameraDirection, 0))
    previousCameraPosition = cameraPosition
    previousViewSize = viewSize
    cullCameraNode.worldPosition = cameraPosition
    cullCameraNode.worldRotation = cameraRotation
    previousViewProj = currentViewProj
    previousCameraDirection = currentCameraDirection
    previousFarClip = camera.farClip
    taaFrame = taaFrame + 1
end

function SubscribeToEvents()
    -- Subscribe HandleUpdate() function for camera motion
    SubscribeToEvent("Update", "HandleUpdate")
end

function HandleUpdate(eventType, eventData)
    -- Take the frame time step, which is stored as a float
    local timeStep = eventData["TimeStep"]:GetFloat()

    -- Move the camera, scale movement with time step
    MoveCamera(timeStep)

    UpdateTAA()
end

function MoveCamera(timeStep)
    -- Right mouse button controls mouse cursor visibility: hide when pressed
    ui.cursor.visible = not input:GetMouseButtonDown(MOUSEB_RIGHT)

    -- Do not move if the UI has a focused element
    if ui.focusElement ~= nil then
        return
    end

    -- Movement speed as world units per second
    local MOVE_SPEED = 10.0
    -- Mouse sensitivity as degrees per pixel
    local MOUSE_SENSITIVITY = 0.1

    -- Use this frame's mouse motion to adjust camera node yaw and pitch. Clamp the pitch between -90 and 90 degrees
    -- Only move the camera when the cursor is hidden
    if not ui.cursor.visible then
        local mouseMove = input.mouseMove
        yaw = yaw + MOUSE_SENSITIVITY * mouseMove.x
        pitch = pitch + MOUSE_SENSITIVITY * mouseMove.y
        pitch = Clamp(pitch, -90.0, 90.0)

        -- Construct new orientation for the camera scene node from yaw and pitch. Roll is fixed to zero
        cameraNode.rotation = Quaternion(pitch, yaw, 0.0)
    end

    -- Read WASD keys and move the camera scene node to the corresponding direction if they are pressed
    if input:GetKeyDown(KEY_W) then
        cameraNode:Translate(Vector3(0.0, 0.0, 1.0) * MOVE_SPEED * timeStep)
    end
    if input:GetKeyDown(KEY_S) then
        cameraNode:Translate(Vector3(0.0, 0.0, -1.0) * MOVE_SPEED * timeStep)
    end
    if input:GetKeyDown(KEY_A) then
        cameraNode:Translate(Vector3(-1.0, 0.0, 0.0) * MOVE_SPEED * timeStep)
    end
    if input:GetKeyDown(KEY_D) then
        cameraNode:Translate(Vector3(1.0, 0.0, 0.0) * MOVE_SPEED * timeStep)
    end
end

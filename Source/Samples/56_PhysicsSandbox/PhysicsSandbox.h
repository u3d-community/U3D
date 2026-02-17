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

#pragma once

#include "Sample.h"

namespace Urho3D
{

class Node;
class Scene;
class Text;

}

/// Physics sandbox example.
/// This sample demonstrates:
///     - Flicking a sphere to knock over a tower of boxes
///     - Camera follow with smooth interpolation
///     - Touch and mouse input for flick mechanics
///     - Automatic scene reset after the sphere settles
class PhysicsSandbox : public Sample
{
    URHO3D_OBJECT(PhysicsSandbox, Sample);

public:
    /// Construct.
    explicit PhysicsSandbox(Context* context);

    /// Setup before engine initialization.
    void Setup() override;
    /// Setup after engine initialization and before running the main loop.
    void Start() override;

private:
    /// Construct the scene content.
    void CreateScene();
    /// Construct an instruction text to the UI.
    void CreateInstructions();
    /// Set up a viewport for displaying the scene.
    void SetupViewport();
    /// Subscribe to application-wide logic update and post-render update events.
    void SubscribeToEvents();
    /// Handle press (mouse or touch) to begin a flick.
    void HandlePress(int screenX, int screenY);
    /// Handle release (mouse or touch) to apply flick impulse.
    void HandleRelease(int screenX, int screenY);
    /// Handle the logic update event.
    void HandleUpdate(StringHash eventType, VariantMap& eventData);
    /// Handle the post-update event (camera follow).
    void HandlePostUpdate(StringHash eventType, VariantMap& eventData);
    /// Handle mouse button down.
    void HandleMouseButtonDown(StringHash eventType, VariantMap& eventData);
    /// Handle mouse button up.
    void HandleMouseButtonUp(StringHash eventType, VariantMap& eventData);
    /// Handle touch begin.
    void HandleTouchBegin(StringHash eventType, VariantMap& eventData);
    /// Handle touch end.
    void HandleTouchEnd(StringHash eventType, VariantMap& eventData);
    /// Handle the post-render update event.
    void HandlePostRenderUpdate(StringHash eventType, VariantMap& eventData);

    /// The flick-able sphere node.
    Node* sphereNode_;
    /// Screen position at touch/click begin.
    IntVector2 flickStart_;
    /// World-space hit point on sphere surface.
    Vector3 hitPoint_;
    /// Tracking a flick in progress.
    bool flickActive_;
    /// Sphere has been launched.
    bool launched_;
    /// Counts time sphere velocity is near zero.
    float settleTimer_;
    /// Flag for drawing debug geometry.
    bool drawDebug_;
    /// Instruction text element.
    Text* instructionText_;
    /// Initial camera position.
    Vector3 initialCameraPos_;
    /// Initial camera look-at target.
    Vector3 initialCameraTarget_;
};

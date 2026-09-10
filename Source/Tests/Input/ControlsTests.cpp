#include "TestUtils.h"
#include <Urho3D/Input/Controls.h>
#include <Urho3D/Input/Input.h>

using namespace Urho3D;
using namespace U3DTest;

TEST_CASE("Controls tracks buttons, edges, axes, and extra data", "[Input]")
{
    Controls previous;
    Controls controls;
    REQUIRE(controls.buttons_ == 0);
    controls.Set(0x1);
    controls.Set(0x4);
    REQUIRE(controls.IsDown(0x1));
    REQUIRE(controls.IsDown(0x4));
    REQUIRE_FALSE(controls.IsDown(0x2));
    REQUIRE(controls.IsPressed(0x1, previous));
    previous = controls;
    REQUIRE_FALSE(controls.IsPressed(0x1, previous));
    controls.Set(0x1, false);
    REQUIRE_FALSE(controls.IsDown(0x1));
    controls.yaw_ = 12.5f;
    controls.pitch_ = -3.0f;
    controls.extraData_["aim"] = Vector3::FORWARD;
    controls.Reset();
    REQUIRE(controls.buttons_ == 0);
    REQUIRE_EQ_F(controls.yaw_, 0.0f);
    REQUIRE_EQ_F(controls.pitch_, 0.0f);
    REQUIRE(controls.extraData_.Empty());
}

TEST_CASE("Headless input has stable empty device state", "[Input]")
{
    Input* input = HeadlessContext()->GetSubsystem<Input>();
    REQUIRE(input != nullptr);
    REQUIRE_FALSE(input->GetKeyDown(KEY_UNKNOWN));
    REQUIRE_FALSE(input->GetKeyPress(KEY_UNKNOWN));
    REQUIRE_FALSE(input->GetMouseButtonDown(MOUSEB_NONE));
    REQUIRE_FALSE(input->GetMouseButtonPress(MOUSEB_NONE));
    REQUIRE(input->GetNumTouches() == 0);
    REQUIRE(input->GetNumJoysticks() == 0);
}

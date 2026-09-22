#include "TestUtils.h"
#include <Urho3D/UI/ProgressBar.h>
#include <Urho3D/UI/ScrollBar.h>
#include <Urho3D/UI/Slider.h>

using namespace Urho3D;
using namespace U3DTest;

TEST_CASE("Slider clamps range, value, and repeat rate", "[UI]")
{
    SharedPtr<Slider> slider(new Slider(HeadlessContext()));
    REQUIRE(slider->GetKnob() != nullptr);
    slider->SetRange(-1.0f);
    REQUIRE_EQ_F(slider->GetRange(), 0.0f);
    slider->SetRange(10.0f);
    slider->SetValue(12.0f);
    REQUIRE_EQ_F(slider->GetValue(), 10.0f);
    slider->ChangeValue(-20.0f);
    REQUIRE_EQ_F(slider->GetValue(), 0.0f);
    slider->SetOrientation(O_VERTICAL);
    REQUIRE(slider->GetOrientation() == O_VERTICAL);
    slider->SetRepeatRate(-5.0f);
    REQUIRE_EQ_F(slider->GetRepeatRate(), 0.0f);
}

TEST_CASE("ProgressBar and ScrollBar clamp public values", "[UI]")
{
    SharedPtr<ProgressBar> progress(new ProgressBar(HeadlessContext()));
    progress->SetRange(100.0f);
    progress->SetValue(120.0f);
    REQUIRE_EQ_F(progress->GetValue(), 100.0f);
    progress->SetValue(-1.0f);
    REQUIRE_EQ_F(progress->GetValue(), 0.0f);
    progress->SetShowPercentText(true);
    REQUIRE(progress->GetShowPercentText());
    REQUIRE(progress->GetKnob() != nullptr);

    SharedPtr<ScrollBar> scroll(new ScrollBar(HeadlessContext()));
    scroll->SetRange(20.0f);
    scroll->SetValue(25.0f);
    REQUIRE_EQ_F(scroll->GetValue(), 20.0f);
    scroll->SetStepFactor(0.25f);
    REQUIRE_EQ_F(scroll->GetStepFactor(), 0.25f);
}

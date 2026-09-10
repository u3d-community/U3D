#include "TestUtils.h"
#include <Urho3D/Engine/Engine.h>
#include <Urho3D/Engine/EngineDefs.h>

using namespace Urho3D;
using namespace U3DTest;

TEST_CASE("Engine parses command line parameters", "[Engine]")
{
    Vector<String> args{"-headless", "-nolimit", "-nosound", "-w", "-x", "1280", "-y", "720", "-m", "4",
        "-renderpath", "Custom.xml", "-pp", "Prefix", "-p", "Data;CoreData", "-v"};
    const VariantMap parameters = Engine::ParseParameters(args);
    REQUIRE(parameters[EP_HEADLESS]->GetBool());
    REQUIRE_FALSE(parameters[EP_FRAME_LIMITER]->GetBool());
    REQUIRE_FALSE(parameters[EP_SOUND]->GetBool());
    REQUIRE_FALSE(parameters[EP_FULL_SCREEN]->GetBool());
    REQUIRE(parameters[EP_WINDOW_WIDTH]->GetInt() == 1280);
    REQUIRE(parameters[EP_WINDOW_HEIGHT]->GetInt() == 720);
    REQUIRE(parameters[EP_MULTI_SAMPLE]->GetInt() == 4);
    REQUIRE(parameters[EP_RENDER_PATH]->GetString() == "Custom.xml");
    REQUIRE(parameters[EP_RESOURCE_PREFIX_PATHS]->GetString() == "Prefix");
    REQUIRE(parameters[EP_RESOURCE_PATHS]->GetString() == "Data;CoreData");
    REQUIRE(parameters[EP_VSYNC]->GetBool());
    REQUIRE(Engine::HasParameter(parameters, EP_HEADLESS));
    REQUIRE_FALSE(Engine::HasParameter(parameters, "Missing"));
    REQUIRE(Engine::GetParameter(parameters, "Missing", 42).GetInt() == 42);
}

TEST_CASE("Engine runtime settings clamp and round trip", "[Engine]")
{
    TestContext context;
    SharedPtr<Engine> engine(new Engine(context));
    REQUIRE_FALSE(engine->IsInitialized());
    engine->SetMinFps(-1);
    engine->SetMaxFps(-2);
    engine->SetMaxInactiveFps(-3);
    REQUIRE(engine->GetMinFps() == 0);
    REQUIRE(engine->GetMaxFps() == 0);
    REQUIRE(engine->GetMaxInactiveFps() == 0);
    engine->SetMinFps(15);
    engine->SetMaxFps(120);
    engine->SetMaxInactiveFps(30);
    REQUIRE(engine->GetMinFps() == 15);
    REQUIRE(engine->GetMaxFps() == 120);
    REQUIRE(engine->GetMaxInactiveFps() == 30);
    engine->SetTimeStepSmoothing(0);
    REQUIRE(engine->GetTimeStepSmoothing() == 1);
    engine->SetTimeStepSmoothing(100);
    REQUIRE(engine->GetTimeStepSmoothing() == 20);
    engine->SetPauseMinimized(false);
    engine->SetAutoExit(false);
    engine->SetNextTimeStep(-1.0f);
    REQUIRE_FALSE(engine->GetPauseMinimized());
    REQUIRE_FALSE(engine->GetAutoExit());
    REQUIRE_EQ_F(engine->GetNextTimeStep(), 0.0f);
}

TEST_CASE("Headless engine exposes initialized state", "[Engine]")
{
    Engine* engine = HeadlessContext()->GetSubsystem<Engine>();
    REQUIRE(engine != nullptr);
    REQUIRE(engine->IsInitialized());
    REQUIRE(engine->IsHeadless());
    REQUIRE(engine->CreateConsole() == nullptr);
    REQUIRE(engine->CreateDebugHud() == nullptr);
}

#include "TestUtils.h"
#include <Urho3D/Network/Network.h>
#include <Urho3D/Network/NetworkPriority.h>

using namespace Urho3D;
using namespace U3DTest;

TEST_CASE("NetworkPriority clamps settings and schedules updates", "[Network]")
{
    TestContext context;
    NetworkPriority priority(context);
    REQUIRE_EQ_F(priority.GetBasePriority(), 100.0f);
    priority.SetBasePriority(-1.0f);
    priority.SetDistanceFactor(-1.0f);
    priority.SetMinPriority(-1.0f);
    REQUIRE_EQ_F(priority.GetBasePriority(), 0.0f);
    REQUIRE_EQ_F(priority.GetDistanceFactor(), 0.0f);
    REQUIRE_EQ_F(priority.GetMinPriority(), 0.0f);
    priority.SetBasePriority(50.0f);
    priority.SetDistanceFactor(2.0f);
    priority.SetMinPriority(10.0f);
    priority.SetAlwaysUpdateOwner(false);
    float accumulator = 0.0f;
    REQUIRE_FALSE(priority.CheckUpdate(10.0f, accumulator));
    REQUIRE_EQ_F(accumulator, 30.0f);
    REQUIRE_FALSE(priority.CheckUpdate(10.0f, accumulator));
    REQUIRE_FALSE(priority.CheckUpdate(10.0f, accumulator));
    REQUIRE(priority.CheckUpdate(10.0f, accumulator));
    REQUIRE_EQ_F(accumulator, 20.0f);
    REQUIRE_FALSE(priority.GetAlwaysUpdateOwner());
}

TEST_CASE("Network settings and remote event allowlist", "[Network]")
{
    Network* network = HeadlessContext()->GetSubsystem<Network>();
    REQUIRE(network != nullptr);
    network->SetUpdateFps(0);
    REQUIRE(network->GetUpdateFps() == 1);
    network->SetUpdateFps(120);
    REQUIRE(network->GetUpdateFps() == 120);
    network->SetSimulatedLatency(-10);
    REQUIRE(network->GetSimulatedLatency() == 0);
    network->SetSimulatedLatency(25);
    REQUIRE(network->GetSimulatedLatency() == 25);
    network->SetSimulatedPacketLoss(-1.0f);
    REQUIRE_EQ_F(network->GetSimulatedPacketLoss(), 0.0f);
    network->SetSimulatedPacketLoss(2.0f);
    REQUIRE_EQ_F(network->GetSimulatedPacketLoss(), 1.0f);
    const StringHash eventType("UnitTestRemoteEvent");
    REQUIRE_FALSE(network->CheckRemoteEvent(eventType));
    network->RegisterRemoteEvent(eventType);
    REQUIRE(network->CheckRemoteEvent(eventType));
    network->UnregisterRemoteEvent(eventType);
    REQUIRE_FALSE(network->CheckRemoteEvent(eventType));
    network->RegisterRemoteEvent(eventType);
    network->UnregisterAllRemoteEvents();
    REQUIRE_FALSE(network->CheckRemoteEvent(eventType));
    network->SetPackageCacheDir(ScratchPath("network-cache"));
    REQUIRE(network->GetPackageCacheDir().EndsWith("/"));
    REQUIRE_FALSE(network->IsServerRunning());
    REQUIRE(network->GetServerConnection() == nullptr);
    REQUIRE(network->GetClientConnections().Empty());
}

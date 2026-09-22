#include "TestUtils.h"
#include <Urho3D/IO/VectorBuffer.h>
#include <Urho3D/Resource/ResourceCache.h>

using namespace Urho3D;
using namespace U3DTest;

namespace
{
class MemoryResource : public Resource
{
    URHO3D_OBJECT(MemoryResource, Resource);
public:
    explicit MemoryResource(Context* context) : Resource(context) {}
};

class PrefixRouter : public ResourceRouter
{
    URHO3D_OBJECT(PrefixRouter, ResourceRouter);
public:
    explicit PrefixRouter(Context* context) : ResourceRouter(context) {}
    void Route(String& name, ResourceRequest) override
    {
        if (name.StartsWith("alias/"))
            name = name.Substring(6);
        else if (name.StartsWith("blocked/"))
            name.Clear();
    }
};
}

TEST_CASE("ResourceCache manages manual resources and budgets", "[Resource]")
{
    Context* context = HeadlessContext();
    ResourceCache* cache = context->GetSubsystem<ResourceCache>();
    SharedPtr<MemoryResource> resource(new MemoryResource(context));
    resource->SetName("Generated/Test.bin");
    resource->SetMemoryUse(256);
    REQUIRE(cache->AddManualResource(resource));
    REQUIRE(cache->AddManualResource(resource));
    REQUIRE(cache->GetExistingResource(MemoryResource::GetTypeStatic(), resource->GetName()) == resource);
    REQUIRE(cache->GetMemoryUse(MemoryResource::GetTypeStatic()) == 256);
    cache->SetMemoryBudget(MemoryResource::GetTypeStatic(), 1024);
    REQUIRE(cache->GetMemoryBudget(MemoryResource::GetTypeStatic()) == 1024);
    PODVector<Resource*> resources;
    cache->GetResources(resources, MemoryResource::GetTypeStatic());
    REQUIRE(resources.Contains(resource));
    REQUIRE_FALSE(cache->PrintMemoryUsage().Empty());
    resource.Reset();
    cache->ReleaseResource(MemoryResource::GetTypeStatic(), "Generated/Test.bin", true);
    REQUIRE(cache->GetExistingResource(MemoryResource::GetTypeStatic(), "Generated/Test.bin") == nullptr);
}

TEST_CASE("ResourceCache settings, sanitation, and routing", "[Resource]")
{
    Context* context = HeadlessContext();
    ResourceCache* cache = context->GetSubsystem<ResourceCache>();
    cache->SetReturnFailedResources(true);
    cache->SetSearchPackagesFirst(false);
    cache->SetFinishBackgroundResourcesMs(0);
    REQUIRE(cache->GetReturnFailedResources());
    REQUIRE_FALSE(cache->GetSearchPackagesFirst());
    REQUIRE(cache->GetFinishBackgroundResourcesMs() == 1);
    REQUIRE(cache->SanitateResourceName("./UI/../UI/DefaultStyle.xml") == "UI/UI/DefaultStyle.xml");
    REQUIRE(cache->GetResourceFileName("UI/DefaultStyle.xml").EndsWith("UI/DefaultStyle.xml"));

    SharedPtr<PrefixRouter> router(new PrefixRouter(context));
    cache->AddResourceRouter(router);
    REQUIRE(cache->GetResourceRouter(0) != nullptr);
    REQUIRE(cache->Exists("alias/UI/DefaultStyle.xml"));
    REQUIRE_FALSE(cache->Exists("blocked/UI/DefaultStyle.xml"));
    cache->RemoveResourceRouter(router);
    cache->SetReturnFailedResources(false);
}

#include "TestUtils.h"

#include <Urho3D/IO/VectorBuffer.h>
#include <Urho3D/Resource/Resource.h>
#include <Urho3D/Resource/XMLElement.h>
#include <Urho3D/Resource/XMLFile.h>

using namespace Urho3D;
using namespace U3DTest;

namespace
{

class TestResource : public ResourceWithMetadata
{
    URHO3D_OBJECT(TestResource, ResourceWithMetadata);

public:
    explicit TestResource(Context* context) : ResourceWithMetadata(context) {}

    bool beginResult_{true};
    bool endResult_{true};
    unsigned beginCalls_{};
    unsigned endCalls_{};

    bool BeginLoad(Deserializer&) override
    {
        ++beginCalls_;
        return beginResult_;
    }

    bool EndLoad() override
    {
        ++endCalls_;
        return endResult_;
    }

    void ReadMetadataXML(const XMLElement& element) { LoadMetadataFromXML(element); }
    void ReadMetadataJSON(const JSONArray& array) { LoadMetadataFromJSON(array); }
    void WriteMetadataXML(XMLElement& element) const { SaveMetadataToXML(element); }
    void CopyMetadataFrom(const TestResource& source) { CopyMetadata(source); }
};

}

TEST_CASE("Resource load lifecycle and properties", "[Resource]")
{
    TestContext context;
    SharedPtr<TestResource> resource(new TestResource(context));
    VectorBuffer source;

    REQUIRE(resource->GetName().Empty());
    REQUIRE(resource->GetMemoryUse() == 0);
    REQUIRE(resource->GetAsyncLoadState() == ASYNC_DONE);

    resource->SetName("Folder/Thing.dat");
    resource->SetMemoryUse(1234);
    REQUIRE(resource->GetName() == "Folder/Thing.dat");
    REQUIRE(resource->GetNameHash() == StringHash("Folder/Thing.dat"));
    REQUIRE(resource->GetMemoryUse() == 1234);

    SECTION("successful load calls both phases and restores async state")
    {
        REQUIRE(resource->Load(source));
        REQUIRE(resource->beginCalls_ == 1);
        REQUIRE(resource->endCalls_ == 1);
        REQUIRE(resource->GetAsyncLoadState() == ASYNC_DONE);
    }

    SECTION("failed begin skips end")
    {
        resource->beginResult_ = false;
        REQUIRE_FALSE(resource->Load(source));
        REQUIRE(resource->beginCalls_ == 1);
        REQUIRE(resource->endCalls_ == 0);
        REQUIRE(resource->GetAsyncLoadState() == ASYNC_DONE);
    }

    SECTION("failed end fails the complete load")
    {
        resource->endResult_ = false;
        REQUIRE_FALSE(resource->Load(source));
        REQUIRE(resource->beginCalls_ == 1);
        REQUIRE(resource->endCalls_ == 1);
    }

    SECTION("resource helper functions handle nulls")
    {
        REQUIRE(GetResourceName(nullptr).Empty());
        REQUIRE(GetResourceType(nullptr, StringHash("Fallback")) == StringHash("Fallback"));
        REQUIRE(GetResourceRef(nullptr, StringHash("Fallback")) == ResourceRef(StringHash("Fallback"), ""));
    }
}

TEST_CASE("Resource metadata can be managed and serialized", "[Resource]")
{
    TestContext context;
    TestResource resource(context);

    REQUIRE_FALSE(resource.HasMetadata());
    REQUIRE(resource.GetMetadata("missing").IsEmpty());

    resource.AddMetadata("answer", 42);
    resource.AddMetadata("label", "first");
    resource.AddMetadata("label", "updated");
    REQUIRE(resource.HasMetadata());
    REQUIRE(resource.GetMetadata("answer").GetInt() == 42);
    REQUIRE(resource.GetMetadata("label").GetString() == "updated");

    SECTION("remove one or all entries")
    {
        resource.RemoveMetadata("answer");
        REQUIRE(resource.GetMetadata("answer").IsEmpty());
        REQUIRE(resource.HasMetadata());
        resource.RemoveAllMetadata();
        REQUIRE_FALSE(resource.HasMetadata());
    }

    SECTION("XML round trip preserves names, values, and overwrite order")
    {
        SharedPtr<XMLFile> document(new XMLFile(context));
        XMLElement root = document->CreateRoot("root");
        resource.WriteMetadataXML(root);

        TestResource restored(context);
        restored.ReadMetadataXML(root);
        REQUIRE(restored.GetMetadata("answer").GetInt() == 42);
        REQUIRE(restored.GetMetadata("label").GetString() == "updated");
    }

    SECTION("JSON metadata and copying are deep value copies")
    {
        JSONArray array;
        JSONValue entry;
        entry.SetVariant(Variant(7));
        entry.Set("name", "json");
        array.Push(entry);

        TestResource restored(context);
        restored.ReadMetadataJSON(array);
        REQUIRE(restored.GetMetadata("json").GetInt() == 7);

        TestResource copied(context);
        copied.CopyMetadataFrom(resource);
        resource.AddMetadata("answer", 99);
        REQUIRE(copied.GetMetadata("answer").GetInt() == 42);
    }
}

#include "TestUtils.h"

#include <Urho3D/IO/VectorBuffer.h>
#include <Urho3D/Resource/JSONFile.h>
#include <Urho3D/Resource/XMLFile.h>
#include <Urho3D/Scene/Component.h>
#include <Urho3D/Scene/Node.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/SceneEvents.h>
#include <Urho3D/Scene/ReplicationState.h>
#include <Urho3D/Scene/Serializable.h>
#include <Urho3D/IO/File.h>

using namespace Urho3D;
using namespace U3DTest;

namespace
{

class Probe : public Component
{
    URHO3D_OBJECT(Probe, Component);

public:
    explicit Probe(Context* context) :
        Component(context),
        health_(100),
        label_("default"),
        speed_(1.0f),
        mode_(0),
        localOnly_(7),
        applyCount_(0)
    {
    }

    static void RegisterObject(Context* context)
    {
        context->RegisterFactory<Probe>();

        URHO3D_ATTRIBUTE("Health", int, health_, 100, AM_DEFAULT);
        URHO3D_ATTRIBUTE("Label", String, label_, "default", AM_DEFAULT);
        URHO3D_ACCESSOR_ATTRIBUTE("Speed", GetSpeed, SetSpeed, float, 1.0f, AM_DEFAULT);
        URHO3D_ENUM_ATTRIBUTE("Mode", mode_, modeNames, 0, AM_DEFAULT);
        URHO3D_ATTRIBUTE("LocalOnly", int, localOnly_, 7, AM_FILE);
    }

    void ApplyAttributes() override { ++applyCount_; }

    float GetSpeed() const { return speed_; }
    void SetSpeed(float speed) { speed_ = speed; }

    int health_;
    String label_;
    float speed_;
    int mode_;
    int localOnly_;
    int applyCount_;

    static const char* modeNames[];
};

const char* Probe::modeNames[] = {"Idle", "Walk", "Run", nullptr};

Context* ProbeContext()
{
    Context* context = HeadlessContext();
    static bool registered = false;
    if (!registered)
    {
        Probe::RegisterObject(context);
        registered = true;
    }
    return context;
}

SharedPtr<Scene> MakeScene()
{
    return SharedPtr<Scene>(new Scene(ProbeContext()));
}

}

TEST_CASE("Serializable exposes its attributes by index and by name", "[Scene]")
{
    SharedPtr<Scene> scene = MakeScene();
    auto* probe = scene->CreateChild("Probe")->CreateComponent<Probe>();

    const Vector<AttributeInfo>* attributes = probe->GetAttributes();
    REQUIRE(attributes != nullptr);
    REQUIRE(probe->GetNumAttributes() == attributes->Size());
    REQUIRE(probe->GetNumAttributes() >= 5);

    REQUIRE(probe->GetAttribute("Health").GetInt() == 100);
    REQUIRE(probe->GetAttribute("Label").GetString() == "default");
    REQUIRE_EQ_F(probe->GetAttribute("Speed").GetFloat(), 1.0f);

    REQUIRE(probe->SetAttribute("Health", 42));
    REQUIRE(probe->health_ == 42);
    REQUIRE(probe->GetAttribute("Health").GetInt() == 42);

    REQUIRE(probe->SetAttribute("Speed", 2.5f));
    REQUIRE_EQ_F(probe->GetSpeed(), 2.5f);

    SECTION("an unknown attribute is rejected and reads back empty")
    {
        REQUIRE_FALSE(probe->SetAttribute("NoSuchAttribute", 1));
        REQUIRE(probe->GetAttribute("NoSuchAttribute").IsEmpty());
    }

    SECTION("an out of range index is rejected")
    {
        REQUIRE_FALSE(probe->SetAttribute(999u, Variant(1)));
        REQUIRE(probe->GetAttribute(999u).IsEmpty());
        REQUIRE(probe->GetAttributeDefault(999u).IsEmpty());
    }

    SECTION("the defaults are reported separately from the live values")
    {
        REQUIRE(probe->GetAttributeDefault("Health").GetInt() == 100);
        REQUIRE(probe->GetAttributeDefault("Label").GetString() == "default");
        REQUIRE(probe->GetAttribute("Health").GetInt() == 42);
    }

    SECTION("ResetToDefault puts every attribute back")
    {
        probe->SetAttribute("Label", "changed");
        probe->ResetToDefault();
        REQUIRE(probe->health_ == 100);
        REQUIRE(probe->label_ == "default");
        REQUIRE_EQ_F(probe->speed_, 1.0f);
    }

    SECTION("an enum attribute round trips through its name")
    {
        REQUIRE(probe->SetAttribute("Mode", 2));
        REQUIRE(probe->mode_ == 2);
        REQUIRE(probe->GetAttribute("Mode").GetInt() == 2);
    }
}

TEST_CASE("Serializable round trips through every document format", "[Scene]")
{
    SharedPtr<Scene> scene = MakeScene();
    auto* probe = scene->CreateChild("Probe")->CreateComponent<Probe>();
    probe->SetAttribute("Health", 55);
    probe->SetAttribute("Label", "saved");
    probe->SetAttribute("Speed", 3.25f);
    probe->SetAttribute("Mode", 1);
    probe->SetAttribute("LocalOnly", 99);

    SECTION("binary")
    {
        VectorBuffer buffer;
        REQUIRE(probe->Save(buffer));
        buffer.Seek(0);

        REQUIRE(buffer.ReadStringHash() == Probe::GetTypeStatic());
        REQUIRE(buffer.ReadUInt() == probe->GetID());

        auto* restored = scene->CreateChild("Restored")->CreateComponent<Probe>();
        REQUIRE(restored->Load(buffer));
        REQUIRE(restored->health_ == 55);
        REQUIRE(restored->label_ == "saved");
        REQUIRE_EQ_F(restored->speed_, 3.25f);
        REQUIRE(restored->mode_ == 1);
        REQUIRE(restored->localOnly_ == 99);
    }

    SECTION("XML")
    {
        SharedPtr<XMLFile> document(new XMLFile(ProbeContext()));
        XMLElement root = document->CreateRoot("component");
        REQUIRE(probe->SaveXML(root));

        auto* restored = scene->CreateChild("Restored")->CreateComponent<Probe>();
        REQUIRE(restored->LoadXML(root));
        REQUIRE(restored->health_ == 55);
        REQUIRE(restored->label_ == "saved");
        REQUIRE(restored->applyCount_ == 0);

        restored->ApplyAttributes();
        REQUIRE(restored->applyCount_ == 1);
    }

    SECTION("JSON")
    {
        JSONValue json;
        REQUIRE(probe->SaveJSON(json));

        auto* restored = scene->CreateChild("Restored")->CreateComponent<Probe>();
        REQUIRE(restored->LoadJSON(json));
        REQUIRE(restored->health_ == 55);
        REQUIRE_EQ_F(restored->speed_, 3.25f);
        REQUIRE(restored->mode_ == 1);
    }

    SECTION("an XML document missing an attribute leaves that one at its default")
    {
        SharedPtr<XMLFile> document(new XMLFile(ProbeContext()));
        XMLElement root = document->CreateRoot("component");
        XMLElement attribute = root.CreateChild("attribute");
        attribute.SetAttribute("name", "Health");
        attribute.SetAttribute("value", "77");

        auto* restored = scene->CreateChild("Restored")->CreateComponent<Probe>();
        REQUIRE(restored->LoadXML(root));
        REQUIRE(restored->health_ == 77);
        REQUIRE(restored->label_ == "default");
    }

    SECTION("an unknown attribute name in the document is skipped")
    {
        SharedPtr<XMLFile> document(new XMLFile(ProbeContext()));
        XMLElement root = document->CreateRoot("component");
        XMLElement attribute = root.CreateChild("attribute");
        attribute.SetAttribute("name", "GoneInThisVersion");
        attribute.SetAttribute("value", "1");

        auto* restored = scene->CreateChild("Restored")->CreateComponent<Probe>();
        REQUIRE(restored->LoadXML(root));
        REQUIRE(restored->health_ == 100);
    }
}

TEST_CASE("Serializable writes and reads network delta updates", "[Scene]")
{
    SharedPtr<Scene> scene = MakeScene();
    auto* sender = scene->CreateChild("Sender")->CreateComponent<Probe>();

    const Vector<AttributeInfo>* networkAttributes = sender->GetNetworkAttributes();
    REQUIRE(networkAttributes != nullptr);
    REQUIRE(sender->GetNumNetworkAttributes() == networkAttributes->Size());

    REQUIRE(sender->GetNumNetworkAttributes() < sender->GetNumAttributes());

    sender->AllocateNetworkState();
    REQUIRE(sender->GetNetworkState() != nullptr);

    sender->SetAttribute("Health", 30);
    sender->SetAttribute("Label", "over the wire");

    sender->PrepareNetworkUpdate();

    VectorBuffer buffer;
    sender->WriteInitialDeltaUpdate(buffer, 0);
    REQUIRE(buffer.GetSize() > 0);

    buffer.Seek(0);
    auto* receiver = scene->CreateChild("Receiver")->CreateComponent<Probe>();
    receiver->AllocateNetworkState();
    REQUIRE(receiver->ReadDeltaUpdate(buffer));
    REQUIRE(receiver->health_ == 30);
    REQUIRE(receiver->label_ == "over the wire");

    REQUIRE(receiver->localOnly_ == 7);

    SECTION("a delta carrying a single attribute only moves that one")
    {
        receiver->SetAttribute("Label", "untouched");

        DirtyBits bits;
        bits.Set(0);

        VectorBuffer delta;
        sender->SetAttribute("Health", 88);
        sender->PrepareNetworkUpdate();
        sender->WriteDeltaUpdate(delta, bits, 0);
        delta.Seek(0);

        REQUIRE(receiver->ReadDeltaUpdate(delta));
        REQUIRE(receiver->health_ == 88);
        REQUIRE(receiver->label_ == "untouched");
    }

    SECTION("latest data updates only carry the attributes marked for them")
    {
        VectorBuffer latest;
        sender->PrepareNetworkUpdate();
        sender->WriteLatestDataUpdate(latest, 7);
        REQUIRE(latest.GetSize() == 1);

        latest.Seek(0);
        REQUIRE_FALSE(receiver->ReadLatestDataUpdate(latest));
    }

    SECTION("an intercepted attribute raises an event instead of being applied")
    {
        receiver->SetInterceptNetworkUpdate("Health", true);
        REQUIRE(receiver->GetInterceptNetworkUpdate("Health"));

        int intercepted = 0;
        receiver->SubscribeToEvent(receiver, E_INTERCEPTNETWORKUPDATE,
            [&intercepted](StringHash eventType, VariantMap& eventData) { ++intercepted; });

        DirtyBits bits;
        bits.Set(0);
        VectorBuffer delta;
        sender->SetAttribute("Health", 123);
        sender->PrepareNetworkUpdate();
        sender->WriteDeltaUpdate(delta, bits, 0);
        delta.Seek(0);

        const int before = receiver->health_;
        REQUIRE_FALSE(receiver->ReadDeltaUpdate(delta));
        REQUIRE(intercepted == 1);
        REQUIRE(receiver->health_ == before);

        receiver->SetInterceptNetworkUpdate("Health", false);
        REQUIRE_FALSE(receiver->GetInterceptNetworkUpdate("Health"));
    }
}

TEST_CASE("Serializable instance defaults override the class defaults", "[Scene]")
{
    SharedPtr<Scene> scene = MakeScene();
    auto* probe = scene->CreateChild("Probe")->CreateComponent<Probe>();

    probe->SetInstanceDefault(true);
    probe->SetAttribute("Health", 250);
    probe->SetInstanceDefault(false);

    REQUIRE(probe->GetAttributeDefault("Health").GetInt() == 250);

    probe->SetAttribute("Health", 10);
    probe->ResetToDefault();
    REQUIRE(probe->health_ == 250);

    probe->RemoveInstanceDefault();
    REQUIRE(probe->GetAttributeDefault("Health").GetInt() == 100);
}

TEST_CASE("A temporary component is left out of saved scenes", "[Scene]")
{
    SharedPtr<Scene> source = MakeScene();
    Node* node = source->CreateChild("Holder");
    auto* kept = node->CreateComponent<Probe>();
    auto* skipped = node->CreateComponent<Probe>();

    kept->SetAttribute("Health", 11);
    skipped->SetAttribute("Health", 22);
    skipped->SetTemporary(true);
    REQUIRE(skipped->IsTemporary());
    REQUIRE_FALSE(kept->IsTemporary());

    VectorBuffer buffer;
    REQUIRE(source->Save(buffer));
    buffer.Seek(0);

    SharedPtr<Scene> loaded = MakeScene();
    REQUIRE(loaded->Load(buffer));

    Node* restoredNode = loaded->GetChild("Holder");
    REQUIRE(restoredNode != nullptr);
    PODVector<Component*> restoredComponents;
    restoredNode->GetComponents(restoredComponents, Probe::GetTypeStatic());
    REQUIRE(restoredComponents.Size() == 1);
    REQUIRE(restoredNode->GetComponent<Probe>()->health_ == 11);

    SECTION("a temporary node is skipped as well")
    {
        Node* temporary = source->CreateChild("Temporary");
        temporary->SetTemporary(true);

        VectorBuffer second;
        REQUIRE(source->Save(second));
        second.Seek(0);

        SharedPtr<Scene> reloaded = MakeScene();
        REQUIRE(reloaded->Load(second));
        REQUIRE(reloaded->GetChild("Temporary") == nullptr);
        REQUIRE(reloaded->GetChild("Holder") != nullptr);
    }
}

TEST_CASE("Node components are found by type, name, and depth", "[Scene]")
{
    SharedPtr<Scene> scene = MakeScene();
    Node* parent = scene->CreateChild("Parent");
    Node* child = parent->CreateChild("Child");

    auto* first = parent->CreateComponent<Probe>();
    auto* second = parent->CreateComponent<Probe>();
    auto* nested = child->CreateComponent<Probe>();

    REQUIRE(parent->GetNumComponents() == 2);
    REQUIRE(parent->GetComponent<Probe>() == first);
    REQUIRE(parent->HasComponent<Probe>());
    REQUIRE(parent->GetComponent(Probe::GetTypeStatic()) == first);

    PODVector<Probe*> found;
    parent->GetComponents<Probe>(found);
    REQUIRE(found.Size() == 2);
    REQUIRE(found[1] == second);

    parent->GetComponents<Probe>(found, true);
    REQUIRE(found.Size() == 3);
    REQUIRE(found.Contains(nested));

    REQUIRE(scene->GetComponent(first->GetID()) == first);
    REQUIRE(scene->GetComponent(0) == nullptr);

    SECTION("a component can be removed by pointer, by type, or wholesale")
    {
        parent->RemoveComponent(second);
        REQUIRE(parent->GetNumComponents() == 1);

        parent->RemoveComponent<Probe>();
        REQUIRE(parent->GetNumComponents() == 0);
        REQUIRE_FALSE(parent->HasComponent<Probe>());

        child->RemoveAllComponents();
        REQUIRE(child->GetNumComponents() == 0);
    }

    SECTION("a component knows the node and scene it belongs to")
    {
        REQUIRE(nested->GetNode() == child);
        REQUIRE(nested->GetScene() == scene.Get());
        REQUIRE(nested->IsEnabled());

        nested->SetEnabled(false);
        REQUIRE_FALSE(nested->IsEnabled());
        REQUIRE_FALSE(nested->IsEnabledEffective());
    }

    SECTION("a component under a disabled node is not effectively enabled")
    {
        REQUIRE(nested->IsEnabledEffective());
        child->SetEnabled(false);
        REQUIRE(nested->IsEnabled());
        REQUIRE_FALSE(nested->IsEnabledEffective());
    }

    SECTION("cloning a node copies its components and their values")
    {
        first->SetAttribute("Health", 64);
        Node* clone = parent->Clone();
        REQUIRE(clone->GetNumComponents() == 2);
        REQUIRE(clone->GetComponent<Probe>()->health_ == 64);
        REQUIRE(clone->GetComponent<Probe>() != first);
    }
}

TEST_CASE("Scene resolves node and component references when loading", "[Scene]")
{
    SharedPtr<Scene> source = MakeScene();
    Node* first = source->CreateChild("First");
    Node* second = source->CreateChild("Second");
    first->CreateComponent<Probe>();
    second->CreateComponent<Probe>();

    const unsigned firstId = first->GetID();

    VectorBuffer buffer;
    REQUIRE(source->Save(buffer));

    SECTION("loading into a fresh scene keeps the same ids")
    {
        buffer.Seek(0);
        SharedPtr<Scene> loaded = MakeScene();
        REQUIRE(loaded->Load(buffer));
        REQUIRE(loaded->GetNode(firstId) != nullptr);
        REQUIRE(loaded->GetNode(firstId)->GetName() == "First");
    }

    SECTION("instantiating into a populated scene hands out fresh ids")
    {
        SharedPtr<Scene> target = MakeScene();
        Node* existing = target->CreateChild("Existing");

        buffer.Seek(0);
        SharedPtr<Scene> temporary = MakeScene();
        buffer.Seek(0);
        REQUIRE(temporary->Load(buffer));

        Node* toCopy = temporary->GetChild("First");
        REQUIRE(toCopy != nullptr);

        VectorBuffer nodeBuffer;
        REQUIRE(toCopy->Save(nodeBuffer));
        nodeBuffer.Seek(0);

        Node* instantiated = target->Instantiate(nodeBuffer, Vector3::ZERO, Quaternion::IDENTITY);
        REQUIRE(instantiated != nullptr);
        REQUIRE(instantiated->GetName() == "First");
        REQUIRE(instantiated->GetID() != existing->GetID());
        REQUIRE(instantiated->GetComponent<Probe>() != nullptr);
    }
}

TEST_CASE("Scene tracks its file name, checksum, and update state", "[Scene]")
{
    SharedPtr<Scene> scene = MakeScene();

    REQUIRE(scene->GetFileName().Empty());
    REQUIRE(scene->IsUpdateEnabled());
    REQUIRE_EQ_F(scene->GetTimeScale(), 1.0f);
    REQUIRE(scene->GetAsyncProgress() >= 0.0f);
    REQUIRE_FALSE(scene->IsAsyncLoading());

    scene->SetTimeScale(-1.0f);
    REQUIRE(scene->GetTimeScale() >= 0.0f);
    scene->SetTimeScale(2.0f);
    REQUIRE_EQ_F(scene->GetTimeScale(), 2.0f);

    scene->SetUpdateEnabled(false);
    REQUIRE_FALSE(scene->IsUpdateEnabled());
    scene->SetUpdateEnabled(true);

    scene->SetElapsedTime(5.0f);
    REQUIRE_EQ_F(scene->GetElapsedTime(), 5.0f);

    scene->SetSmoothingConstant(-1.0f);
    REQUIRE(scene->GetSmoothingConstant() > 0.0f);
    scene->SetSnapThreshold(-1.0f);
    REQUIRE(scene->GetSnapThreshold() >= 0.0f);

    SECTION("a saved and reloaded scene keeps its elapsed time")
    {
        VectorBuffer buffer;
        REQUIRE(scene->Save(buffer));
        buffer.Seek(0);

        SharedPtr<Scene> loaded = MakeScene();
        REQUIRE(loaded->Load(buffer));
        REQUIRE_EQ_F(loaded->GetElapsedTime(), 5.0f);
    }

    SECTION("loading rubbish fails without leaving the scene half built")
    {
        const char* rubbish = "not a scene at all";
        VectorBuffer bad(rubbish, String(rubbish).Length());
        REQUIRE_FALSE(scene->Load(bad));
        REQUIRE(scene->GetNumChildren() == 0);
    }

    SECTION("the scene is written to and read back from an XML file on disk")
    {
        const String path = ScratchPath("serialisable-scene.xml");
        RemoveScratch("serialisable-scene.xml");

        scene->CreateChild("OnDisk")->CreateComponent<Probe>()->SetAttribute("Health", 33);
        {
            File out(ProbeContext(), path, FILE_WRITE);
            REQUIRE(out.IsOpen());
            REQUIRE(scene->SaveXML(out));
        }

        SharedPtr<Scene> loaded = MakeScene();
        {
            File in(ProbeContext(), path, FILE_READ);
            REQUIRE(in.IsOpen());
            REQUIRE(loaded->LoadXML(in));
        }

        REQUIRE(loaded->GetChild("OnDisk") != nullptr);
        REQUIRE(loaded->GetChild("OnDisk")->GetComponent<Probe>()->health_ == 33);
        REQUIRE(loaded->GetFileName() == path);

        RemoveScratch("serialisable-scene.xml");
    }

    SECTION("an asynchronous load completes over several updates")
    {
        const String path = ScratchPath("serialisable-async.xml");
        RemoveScratch("serialisable-async.xml");

        scene->CreateChild("Async")->CreateComponent<Probe>()->SetAttribute("Health", 44);
        {
            File out(ProbeContext(), path, FILE_WRITE);
            REQUIRE(scene->SaveXML(out));
        }

        SharedPtr<Scene> loaded = MakeScene();
        SharedPtr<File> in(new File(ProbeContext(), path, FILE_READ));
        REQUIRE(loaded->LoadAsyncXML(in));

        for (int i = 0; i < 100 && loaded->IsAsyncLoading(); ++i)
            loaded->Update(0.016f);

        REQUIRE_FALSE(loaded->IsAsyncLoading());
        REQUIRE_EQ_F(loaded->GetAsyncProgress(), 1.0f);
        REQUIRE(loaded->GetChild("Async") != nullptr);
        REQUIRE(loaded->GetChild("Async")->GetComponent<Probe>()->health_ == 44);

        RemoveScratch("serialisable-async.xml");
    }
}

TEST_CASE("Scene sends update events while it runs", "[Scene]")
{
    SharedPtr<Scene> scene = MakeScene();

    int updates = 0;
    int postUpdates = 0;
    float lastStep = 0.0f;

    scene->SubscribeToEvent(scene, E_SCENEUPDATE,
        [&updates, &lastStep](StringHash eventType, VariantMap& eventData)
        {
            ++updates;
            lastStep = eventData[SceneUpdate::P_TIMESTEP].GetFloat();
        });
    scene->SubscribeToEvent(scene, E_SCENEPOSTUPDATE,
        [&postUpdates](StringHash eventType, VariantMap& eventData) { ++postUpdates; });

    scene->Update(0.25f);
    REQUIRE(updates == 1);
    REQUIRE(postUpdates == 1);
    REQUIRE_EQ_F(lastStep, 0.25f);

    SECTION("the time scale is applied to the step the scene reports")
    {
        scene->SetTimeScale(2.0f);
        scene->Update(0.25f);
        REQUIRE_EQ_F(lastStep, 0.5f);
    }

    SECTION("the update flag gates the engine driven update, not a manual one")
    {
        scene->SetUpdateEnabled(false);
        scene->Update(0.25f);
        REQUIRE(updates == 2);
    }

    SECTION("elapsed time accumulates across updates")
    {
        scene->SetElapsedTime(0.0f);
        scene->SetTimeScale(1.0f);
        scene->Update(0.5f);
        scene->Update(0.5f);
        REQUIRE_EQ_F(scene->GetElapsedTime(), 1.0f);
    }
}

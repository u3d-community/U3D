#include "TestUtils.h"

#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/DebugRenderer.h>
#include <Urho3D/Graphics/DecalSet.h>
#include <Urho3D/Graphics/Geometry.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Graphics/RenderPath.h>
#include <Urho3D/Graphics/Skeleton.h>
#include <Urho3D/Graphics/StaticModel.h>
#include <Urho3D/Math/Polyhedron.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Resource/XMLFile.h>
#include <Urho3D/Scene/Scene.h>

using namespace Urho3D;
using namespace U3DTest;

namespace
{

XMLFile* LoadXml(const String& name)
{
    return HeadlessContext()->GetSubsystem<ResourceCache>()->GetResource<XMLFile>(name);
}

}

TEST_CASE("RenderPath loads its commands and render targets", "[Graphics]")
{
    XMLFile* file = LoadXml("RenderPaths/Forward.xml");
    REQUIRE(file != nullptr);

    SharedPtr<RenderPath> path(new RenderPath());
    REQUIRE(path->Load(file));
    REQUIRE(path->GetNumCommands() > 0);

    RenderPathCommand* first = path->GetCommand(0);
    REQUIRE(first != nullptr);
    REQUIRE(path->GetCommand(path->GetNumCommands()) == nullptr);
    REQUIRE(first->type_ != CMD_NONE);

    SECTION("appending another path adds to the command list")
    {
        XMLFile* bloom = LoadXml("PostProcess/Bloom.xml");
        REQUIRE(bloom != nullptr);

        const unsigned before = path->GetNumCommands();
        const unsigned targetsBefore = path->GetNumRenderTargets();
        REQUIRE(path->Append(bloom));
        REQUIRE(path->GetNumCommands() > before);
        REQUIRE(path->GetNumRenderTargets() >= targetsBefore);
    }

    SECTION("a clone is an independent copy of the command list")
    {
        SharedPtr<RenderPath> clone = path->Clone();
        REQUIRE(clone.NotNull());
        REQUIRE(clone->GetNumCommands() == path->GetNumCommands());

        clone->RemoveCommand(0);
        REQUIRE(clone->GetNumCommands() == path->GetNumCommands() - 1);
    }

    SECTION("commands can be added, inserted, and removed")
    {
        RenderPathCommand command;
        command.type_ = CMD_CLEAR;
        command.tag_ = "UnitTest";

        const unsigned before = path->GetNumCommands();
        path->AddCommand(command);
        REQUIRE(path->GetNumCommands() == before + 1);
        REQUIRE(path->GetCommand(before)->tag_ == "UnitTest");

        path->InsertCommand(0, command);
        REQUIRE(path->GetNumCommands() == before + 2);
        REQUIRE(path->GetCommand(0)->tag_ == "UnitTest");

        path->RemoveCommands("UnitTest");
        REQUIRE(path->GetNumCommands() == before);
    }

    SECTION("a tag can be enabled, disabled, and toggled")
    {
        RenderPathCommand command;
        command.type_ = CMD_CLEAR;
        command.tag_ = "Toggleable";
        command.enabled_ = true;
        path->AddCommand(command);

        REQUIRE(path->IsAdded("Toggleable"));
        REQUIRE(path->IsEnabled("Toggleable"));

        path->SetEnabled("Toggleable", false);
        REQUIRE_FALSE(path->IsEnabled("Toggleable"));

        path->ToggleEnabled("Toggleable");
        REQUIRE(path->IsEnabled("Toggleable"));

        REQUIRE_FALSE(path->IsAdded("NeverAdded"));
        REQUIRE_FALSE(path->IsEnabled("NeverAdded"));
    }

    SECTION("render targets can be added and removed by index, name, and tag")
    {
        RenderTargetInfo target;
        target.name_ = "UnitTestTarget";
        target.tag_ = "UnitTestTag";
        target.sizeMode_ = SIZE_VIEWPORTDIVISOR;
        target.size_ = Vector2(2.0f, 2.0f);

        const unsigned before = path->GetNumRenderTargets();
        path->AddRenderTarget(target);
        REQUIRE(path->GetNumRenderTargets() == before + 1);

        path->RemoveRenderTarget("UnitTestTarget");
        REQUIRE(path->GetNumRenderTargets() == before);

        path->AddRenderTarget(target);
        path->RemoveRenderTargets("UnitTestTag");
        REQUIRE(path->GetNumRenderTargets() == before);

        path->AddRenderTarget(target);
        path->RemoveRenderTarget(path->GetNumRenderTargets() - 1);
        REQUIRE(path->GetNumRenderTargets() == before);
    }

    SECTION("a shader parameter is only updated where a command already declares it")
    {
        path->SetShaderParameter("NeverDeclared", Variant(0.75f));
        REQUIRE(path->GetShaderParameter("NeverDeclared").IsEmpty());

        RenderPathCommand command;
        command.type_ = CMD_QUAD;
        command.SetShaderParameter("UnitTestParam", Variant(0.0f));
        path->AddCommand(command);

        path->SetShaderParameter("UnitTestParam", Variant(0.75f));
        REQUIRE_NEAR(path->GetShaderParameter("UnitTestParam").GetFloat(), 0.75f, 0.001f);
    }
}

TEST_CASE("RenderPathCommand carries its own textures and parameters", "[Graphics]")
{
    RenderPathCommand command;
    command.type_ = CMD_QUAD;
    command.SetTextureName(TU_DIFFUSE, "Textures/Logo.png");
    command.SetShaderParameter("Amount", Variant(2.0f));
    command.SetOutput(0, "Target", FACE_POSITIVE_X);

    REQUIRE(command.GetTextureName(TU_DIFFUSE) == "Textures/Logo.png");
    REQUIRE(command.GetTextureName(MAX_TEXTURE_UNITS).Empty());
    REQUIRE_NEAR(command.GetShaderParameter("Amount").GetFloat(), 2.0f, 0.001f);
    REQUIRE(command.GetOutputName(0) == "Target");
    REQUIRE(command.GetOutputFace(0) == FACE_POSITIVE_X);
    REQUIRE(command.GetNumOutputs() >= 1);

    command.RemoveShaderParameter("Amount");
    REQUIRE(command.GetShaderParameter("Amount").IsEmpty());

    command.SetNumOutputs(3);
    REQUIRE(command.GetNumOutputs() == 3);
}

TEST_CASE("RenderPath loading a document with no commands leaves it empty", "[Graphics]")
{
    SharedPtr<RenderPath> path(new RenderPath());
    XMLFile* wrong = LoadXml("UI/DefaultStyle.xml");
    REQUIRE(wrong != nullptr);
    REQUIRE(path->Load(wrong));
    REQUIRE(path->GetNumCommands() == 0);
    REQUIRE(path->GetNumRenderTargets() == 0);
}

TEST_CASE("DebugRenderer accumulates line geometry", "[Graphics]")
{
    Context* context = HeadlessContext();
    SharedPtr<Scene> scene(new Scene(context));
    scene->CreateComponent<Octree>();
    auto* debug = scene->CreateComponent<DebugRenderer>();

    Node* cameraNode = scene->CreateChild("Camera");
    cameraNode->SetPosition(Vector3(0.0f, 0.0f, -20.0f));
    auto* camera = cameraNode->CreateComponent<Camera>();
    camera->SetAspectRatio(1.0f);

    debug->SetView(camera);
    REQUIRE(debug->GetView() == camera->GetView());
    REQUIRE_FALSE(debug->HasContent());

    debug->AddLine(Vector3::ZERO, Vector3::ONE, Color::RED);
    REQUIRE(debug->HasContent());

    SECTION("the end of the frame drops the geometry")
    {
        debug->SendEvent(E_ENDFRAME);
        REQUIRE_FALSE(debug->HasContent());
    }

    SECTION("every primitive helper contributes something")
    {
        debug->AddTriangle(Vector3::ZERO, Vector3::RIGHT, Vector3::UP, Color::GREEN);
        debug->AddPolygon(Vector3::ZERO, Vector3::RIGHT, Vector3::ONE, Vector3::UP, Color::BLUE);
        debug->AddBoundingBox(BoundingBox(-1.0f, 1.0f), Color::WHITE);
        debug->AddBoundingBox(BoundingBox(-1.0f, 1.0f), Matrix3x4::IDENTITY, Color::WHITE, true, true);
        debug->AddSphere(Sphere(Vector3::ZERO, 2.0f), Color::YELLOW);
        debug->AddSphereSector(Sphere(Vector3::ZERO, 2.0f), Quaternion::IDENTITY, 45.0f, true, Color::YELLOW);
        debug->AddCylinder(Vector3::ZERO, 1.0f, 3.0f, Color::CYAN);
        debug->AddCircle(Vector3::ZERO, Vector3::UP, 1.0f, Color::MAGENTA);
        debug->AddCross(Vector3::ZERO, 1.0f, Color::WHITE);
        debug->AddQuad(Vector3::ZERO, 2.0f, 2.0f, Color::WHITE);
        debug->AddNode(cameraNode);

        Frustum frustum;
        frustum.Define(45.0f, 1.0f, 1.0f, 1.0f, 10.0f);
        debug->AddFrustum(frustum, Color::WHITE);

        Polyhedron polyhedron(BoundingBox(-1.0f, 1.0f));
        debug->AddPolyhedron(polyhedron, Color::WHITE);

        REQUIRE(debug->HasContent());
    }

    SECTION("a model's triangles can be pushed straight in")
    {
        auto* model = context->GetSubsystem<ResourceCache>()->GetResource<Model>("Models/Box.mdl");
        REQUIRE(model != nullptr);
        Geometry* geometry = model->GetGeometry(0, 0);

        const unsigned char* vertexData = nullptr;
        const unsigned char* indexData = nullptr;
        unsigned vertexSize = 0;
        unsigned indexSize = 0;
        const PODVector<VertexElement>* elements = nullptr;
        geometry->GetRawData(vertexData, vertexSize, indexData, indexSize, elements);

        debug->AddTriangleMesh(vertexData, vertexSize, indexData, indexSize,
            geometry->GetIndexStart(), geometry->GetIndexCount(), Matrix3x4::IDENTITY, Color::WHITE);
        REQUIRE(debug->HasContent());
    }

    SECTION("a skeleton draws a bone at a time")
    {
        auto* skinned = context->GetSubsystem<ResourceCache>()->GetResource<Model>("Models/Kachujin/Kachujin.mdl");
        REQUIRE(skinned != nullptr);
        debug->AddSkeleton(skinned->GetSkeleton(), Color::WHITE);
        REQUIRE(debug->HasContent());
    }

    SECTION("the line antialias flag round trips")
    {
        debug->SetLineAntiAlias(true);
        REQUIRE(debug->GetLineAntiAlias());
        debug->SetLineAntiAlias(false);
        REQUIRE_FALSE(debug->GetLineAntiAlias());
    }

    SECTION("a point is tested against the debug frustum")
    {
        REQUIRE(debug->IsInside(BoundingBox(Vector3(-1.0f, -1.0f, -1.0f), Vector3(1.0f, 1.0f, 1.0f))));
        REQUIRE_FALSE(debug->IsInside(BoundingBox(Vector3(1000.0f, 0.0f, 0.0f), Vector3(1001.0f, 1.0f, 1.0f))));
    }
}

TEST_CASE("DecalSet refuses to project without a graphics device", "[Graphics]")
{
    Context* context = HeadlessContext();
    auto* cache = context->GetSubsystem<ResourceCache>();

    SharedPtr<Scene> scene(new Scene(context));
    scene->CreateComponent<Octree>();

    Node* targetNode = scene->CreateChild("Target");
    targetNode->SetScale(10.0f);
    auto* target = targetNode->CreateComponent<StaticModel>();
    target->SetModel(cache->GetResource<Model>("Models/Box.mdl"));

    Node* decalNode = scene->CreateChild("Decals");
    auto* decals = decalNode->CreateComponent<DecalSet>();

    Quaternion lookDown;
    lookDown.FromLookRotation(Vector3::DOWN);

    REQUIRE_FALSE(decals->AddDecal(target, Vector3(0.0f, 6.0f, 0.0f), lookDown,
        2.0f, 1.0f, 3.0f, Vector2::ZERO, Vector2::ONE));
    REQUIRE(decals->GetNumDecals() == 0);
    REQUIRE(decals->GetNumVertices() == 0);
    REQUIRE(decals->GetNumIndices() == 0);

    SECTION("a null target is refused as well")
    {
        REQUIRE_FALSE(decals->AddDecal(nullptr, Vector3::ZERO, Quaternion::IDENTITY,
            1.0f, 1.0f, 1.0f, Vector2::ZERO, Vector2::ONE));
    }

    SECTION("removing decals from an empty set is harmless")
    {
        decals->RemoveDecals(5);
        decals->RemoveAllDecals();
        REQUIRE(decals->GetNumDecals() == 0);
    }
}

TEST_CASE("DecalSet keeps its material and buffer settings", "[Graphics]")
{
    Context* context = HeadlessContext();
    auto* cache = context->GetSubsystem<ResourceCache>();

    SharedPtr<Scene> scene(new Scene(context));
    scene->CreateComponent<Octree>();
    auto* decals = scene->CreateChild("Decals")->CreateComponent<DecalSet>();

    auto* material = cache->GetResource<Material>("Materials/UrhoDecal.xml");
    REQUIRE(material != nullptr);
    decals->SetMaterial(material);
    REQUIRE(decals->GetMaterial() == material);

    decals->SetMaxVertices(128);
    decals->SetMaxIndices(256);
    REQUIRE(decals->GetMaxVertices() == 128);
    REQUIRE(decals->GetMaxIndices() == 256);

    decals->SetMaxVertices(1);
    REQUIRE(decals->GetMaxVertices() >= 3);
    decals->SetMaxIndices(1);
    REQUIRE(decals->GetMaxIndices() >= 3);

    decals->SetOptimizeBufferSize(true);
    REQUIRE(decals->GetOptimizeBufferSize());
    decals->SetOptimizeBufferSize(false);
    REQUIRE_FALSE(decals->GetOptimizeBufferSize());

    SECTION("the material round trips through its resource ref")
    {
        const ResourceRef reference = decals->GetMaterialAttr();
        REQUIRE(reference.type_ == Material::GetTypeStatic());

        decals->SetMaterial(nullptr);
        decals->SetMaterialAttr(reference);
        REQUIRE(decals->GetMaterial() == material);
    }

    SECTION("an empty set has no bounding volume to speak of")
    {
        REQUIRE(decals->GetNumDecals() == 0);
        REQUIRE(decals->GetNumVertices() == 0);
    }
}

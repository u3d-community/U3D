#include "TestUtils.h"

#include <Urho3D/Graphics/BillboardSet.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/CustomGeometry.h>
#include <Urho3D/Graphics/Drawable.h>
#include <Urho3D/Graphics/GraphicsDefs.h>
#include <Urho3D/Graphics/Light.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/OcclusionBuffer.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Graphics/RibbonTrail.h>
#include <Urho3D/Graphics/StaticModel.h>
#include <Urho3D/Graphics/Technique.h>
#include <Urho3D/Graphics/VertexBuffer.h>
#include <Urho3D/Graphics/Zone.h>
#include <Urho3D/IO/VectorBuffer.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Resource/XMLFile.h>
#include <Urho3D/Scene/Scene.h>

using namespace Urho3D;
using namespace U3DTest;

namespace
{

template <class T> T* LoadAsset(const String& name)
{
    return HeadlessContext()->GetSubsystem<ResourceCache>()->GetResource<T>(name);
}

}

TEST_CASE("Material carries shader parameters and techniques", "[Graphics]")
{
    auto* material = LoadAsset<Material>("Materials/Stone.xml");
    REQUIRE(material != nullptr);
    REQUIRE(material->GetNumTechniques() > 0);
    REQUIRE(material->GetTechnique(0) != nullptr);

    material->SetShaderParameter("MatDiffColor", Variant(Color::RED));
    REQUIRE(material->GetShaderParameter("MatDiffColor").GetColor() == Color::RED);
    REQUIRE(material->GetShaderParameter("NeverSet").IsEmpty());

    material->SetShaderParameter("Roughness", Variant(0.25f));
    REQUIRE_NEAR(material->GetShaderParameter("Roughness").GetFloat(), 0.25f, 0.001f);

    material->RemoveShaderParameter("Roughness");
    REQUIRE(material->GetShaderParameter("Roughness").IsEmpty());

    material->SetCullMode(CULL_NONE);
    material->SetShadowCullMode(CULL_CW);
    material->SetFillMode(FILL_WIREFRAME);
    REQUIRE(material->GetCullMode() == CULL_NONE);
    REQUIRE(material->GetShadowCullMode() == CULL_CW);
    REQUIRE(material->GetFillMode() == FILL_WIREFRAME);

    material->SetRenderOrder(200);
    REQUIRE(material->GetRenderOrder() == 200);

    material->SetOcclusion(false);
    REQUIRE_FALSE(material->GetOcclusion());

    SECTION("a clone is independent of the original")
    {
        SharedPtr<Material> clone = material->Clone("Cloned");
        REQUIRE(clone.NotNull());
        REQUIRE(clone->GetNumTechniques() == material->GetNumTechniques());
        REQUIRE(clone->GetShaderParameter("MatDiffColor").GetColor() == Color::RED);

        clone->SetShaderParameter("MatDiffColor", Variant(Color::BLUE));
        REQUIRE(material->GetShaderParameter("MatDiffColor").GetColor() == Color::RED);

        clone->SetCullMode(CULL_CCW);
        REQUIRE(material->GetCullMode() == CULL_NONE);
    }

    SECTION("a UV transform builds a texture matrix")
    {
        material->SetUVTransform(Vector2(0.5f, 0.25f), 0.0f, 2.0f);
        const Variant transform = material->GetShaderParameter("UOffset");
        REQUIRE_FALSE(transform.IsEmpty());
        REQUIRE_NEAR(transform.GetVector4().x_, 2.0f, 0.001f);
    }

    SECTION("the technique list can be resized but never emptied")
    {
        material->SetNumTechniques(3);
        REQUIRE(material->GetNumTechniques() == 3);
        REQUIRE(material->GetTechnique(2) == nullptr);

        material->SetNumTechniques(1);
        REQUIRE(material->GetNumTechniques() == 1);

        material->SetNumTechniques(0);
        REQUIRE(material->GetNumTechniques() == 1);
    }

    SECTION("shader defines round trip")
    {
        material->SetVertexShaderDefines("VSDEFINE");
        material->SetPixelShaderDefines("PSDEFINE");
        REQUIRE(material->GetVertexShaderDefines() == "VSDEFINE");
        REQUIRE(material->GetPixelShaderDefines() == "PSDEFINE");
    }

    SECTION("an XML round trip preserves the parameters")
    {
        SharedPtr<XMLFile> document(new XMLFile(HeadlessContext()));
        XMLElement root = document->CreateRoot("material");
        REQUIRE(material->Save(root));

        SharedPtr<Material> restored(new Material(HeadlessContext()));
        REQUIRE(restored->Load(root));
        REQUIRE(restored->GetCullMode() == CULL_NONE);
        REQUIRE(restored->GetRenderOrder() == 200);
        REQUIRE(restored->GetShaderParameter("MatDiffColor").GetColor() == Color::RED);

        REQUIRE(restored->GetShaderParameter("MatSpecColor").GetType() == VAR_VECTOR4);
    }
}

TEST_CASE("Technique indexes its passes by name", "[Graphics]")
{
    auto* technique = LoadAsset<Technique>("Techniques/Diff.xml");
    REQUIRE(technique != nullptr);
    REQUIRE(technique->GetNumPasses() > 0);
    REQUIRE_FALSE(technique->GetPassNames().Empty());

    const String first = technique->GetPassNames()[0];
    Pass* pass = technique->GetPass(first);
    REQUIRE(pass != nullptr);
    REQUIRE(technique->HasPass(first));
    REQUIRE_FALSE(technique->HasPass("nosuchpass"));
    REQUIRE(technique->GetPass("nosuchpass") == nullptr);
    REQUIRE(technique->GetPassIndex(first) == Technique::GetPassIndex(first));

    SECTION("a pass carries its own render state")
    {
        pass->SetBlendMode(BLEND_ALPHA);
        pass->SetDepthTestMode(CMP_LESS);
        pass->SetDepthWrite(false);
        pass->SetCullMode(CULL_NONE);
        REQUIRE(pass->GetBlendMode() == BLEND_ALPHA);
        REQUIRE(pass->GetDepthTestMode() == CMP_LESS);
        REQUIRE_FALSE(pass->GetDepthWrite());
        REQUIRE(pass->GetCullMode() == CULL_NONE);

        pass->SetVertexShader("MyVS");
        pass->SetPixelShader("MyPS");
        REQUIRE(pass->GetVertexShader() == "MyVS");
        REQUIRE(pass->GetPixelShader() == "MyPS");
    }

    SECTION("passes can be created and removed")
    {
        Pass* extra = technique->CreatePass("unittestpass");
        REQUIRE(extra != nullptr);
        REQUIRE(technique->HasPass("unittestpass"));
        REQUIRE(technique->CreatePass("unittestpass") == extra);

        technique->RemovePass("unittestpass");
        REQUIRE_FALSE(technique->HasPass("unittestpass"));
    }

    SECTION("a clone keeps the passes but not the identity")
    {
        SharedPtr<Technique> clone = technique->Clone("ClonedTechnique");
        REQUIRE(clone.NotNull());
        REQUIRE(clone != technique);
        REQUIRE(clone->GetNumPasses() == technique->GetNumPasses());

        clone->RemovePass(first);
        REQUIRE(technique->HasPass(first));
    }
}

TEST_CASE("Light clamps its parameters by type", "[Graphics]")
{
    SharedPtr<Scene> scene(new Scene(HeadlessContext()));
    scene->CreateComponent<Octree>();
    Node* node = scene->CreateChild("Light");
    auto* light = node->CreateComponent<Light>();

    REQUIRE(light->GetLightType() == LIGHT_POINT);

    light->SetRange(-5.0f);
    REQUIRE(light->GetRange() >= 0.0f);
    light->SetRange(20.0f);
    REQUIRE_EQ_F(light->GetRange(), 20.0f);

    light->SetBrightness(2.5f);
    REQUIRE_EQ_F(light->GetBrightness(), 2.5f);

    light->SetColor(Color::GREEN);
    REQUIRE(light->GetColor() == Color::GREEN);
    REQUIRE(light->GetEffectiveColor().g_ > light->GetColor().g_ * 0.5f);

    light->SetSpecularIntensity(-1.0f);
    REQUIRE(light->GetSpecularIntensity() >= 0.0f);

    light->SetFov(500.0f);
    REQUIRE(light->GetFov() <= 180.0f);
    light->SetFov(45.0f);
    REQUIRE_EQ_F(light->GetFov(), 45.0f);

    light->SetAspectRatio(-1.0f);
    REQUIRE(light->GetAspectRatio() > 0.0f);

    light->SetFadeDistance(-1.0f);
    REQUIRE(light->GetFadeDistance() >= 0.0f);

    SECTION("a point light has a spherical volume")
    {
        light->SetLightType(LIGHT_POINT);
        light->SetRange(10.0f);
        node->SetPosition(Vector3(1.0f, 2.0f, 3.0f));

        const BoundingBox bounds = light->GetWorldBoundingBox();
        REQUIRE(bounds.Center().Equals(Vector3(1.0f, 2.0f, 3.0f)));
        REQUIRE_NEAR(bounds.Size().x_, 20.0f, 0.01f);
    }

    SECTION("a directional light is unbounded")
    {
        light->SetLightType(LIGHT_DIRECTIONAL);
        REQUIRE(light->GetLightType() == LIGHT_DIRECTIONAL);
        REQUIRE(light->GetWorldBoundingBox().Size().x_ > 1000.0f);
    }

    SECTION("a spot light builds a frustum along its direction")
    {
        light->SetLightType(LIGHT_SPOT);
        light->SetRange(50.0f);
        light->SetFov(90.0f);
        node->SetPosition(Vector3::ZERO);
        node->SetRotation(Quaternion::IDENTITY);

        const Frustum frustum = light->GetFrustum();
        REQUIRE(frustum.IsInside(Vector3(0.0f, 0.0f, 25.0f)) != OUTSIDE);
        REQUIRE(frustum.IsInside(Vector3(0.0f, 0.0f, -25.0f)) == OUTSIDE);
    }

    SECTION("shadow settings round trip")
    {
        light->SetCastShadows(true);
        REQUIRE(light->GetCastShadows());

        light->SetShadowFadeDistance(30.0f);
        REQUIRE_EQ_F(light->GetShadowFadeDistance(), 30.0f);

        light->SetShadowDistance(100.0f);
        REQUIRE_EQ_F(light->GetShadowDistance(), 100.0f);

        BiasParameters bias(0.0001f, 0.5f);
        light->SetShadowBias(bias);
        REQUIRE_NEAR(light->GetShadowBias().constantBias_, 0.0001f, 0.00001f);

        light->SetShadowResolution(0.1f);
        REQUIRE(light->GetShadowResolution() >= 0.125f);
        light->SetShadowResolution(10.0f);
        REQUIRE(light->GetShadowResolution() <= 1.0f);
    }

    SECTION("the intensity used for sorting follows the brightness")
    {
        light->SetLightType(LIGHT_POINT);
        light->SetBrightness(1.0f);
        const float bright = light->GetIntensityDivisor();
        light->SetBrightness(0.1f);
        REQUIRE(light->GetIntensityDivisor() < bright);
    }
}

TEST_CASE("Zone bounds its volume and blends ambient colour", "[Graphics]")
{
    SharedPtr<Scene> scene(new Scene(HeadlessContext()));
    scene->CreateComponent<Octree>();
    Node* node = scene->CreateChild("Zone");
    auto* zone = node->CreateComponent<Zone>();

    zone->SetBoundingBox(BoundingBox(-10.0f, 10.0f));
    REQUIRE(zone->GetBoundingBox().Size() == Vector3(20.0f, 20.0f, 20.0f));

    zone->SetAmbientColor(Color::GRAY);
    REQUIRE(zone->GetAmbientColor() == Color::GRAY);

    zone->SetFogColor(Color::BLUE);
    REQUIRE(zone->GetFogColor() == Color::BLUE);

    zone->SetFogStart(-1.0f);
    REQUIRE(zone->GetFogStart() >= 0.0f);
    zone->SetFogEnd(-1.0f);
    REQUIRE(zone->GetFogEnd() >= 0.0f);

    zone->SetFogStart(10.0f);
    zone->SetFogEnd(100.0f);
    REQUIRE_EQ_F(zone->GetFogStart(), 10.0f);
    REQUIRE_EQ_F(zone->GetFogEnd(), 100.0f);

    zone->SetPriority(5);
    REQUIRE(zone->GetPriority() == 5);

    zone->SetHeightFog(true);
    REQUIRE(zone->GetHeightFog());

    zone->SetOverride(true);
    REQUIRE(zone->GetOverride());

    SECTION("a point is inside the transformed volume")
    {
        node->SetPosition(Vector3(100.0f, 0.0f, 0.0f));
        REQUIRE(zone->IsInside(Vector3(100.0f, 0.0f, 0.0f)));
        REQUIRE_FALSE(zone->IsInside(Vector3::ZERO));
    }

    SECTION("the inverse world transform maps world space back into the zone")
    {
        node->SetPosition(Vector3(5.0f, 0.0f, 0.0f));
        const Vector3 local = zone->GetInverseWorldTransform() * Vector3(5.0f, 0.0f, 0.0f);
        REQUIRE(local.Equals(Vector3::ZERO));
    }
}

TEST_CASE("BillboardSet manages a bank of billboards", "[Graphics]")
{
    SharedPtr<Scene> scene(new Scene(HeadlessContext()));
    scene->CreateComponent<Octree>();
    auto* set = scene->CreateChild("Billboards")->CreateComponent<BillboardSet>();

    set->SetNumBillboards(4);
    REQUIRE(set->GetNumBillboards() == 4);
    REQUIRE(set->GetBillboards().Size() == 4);
    REQUIRE(set->GetBillboard(0) != nullptr);
    REQUIRE(set->GetBillboard(99) == nullptr);

    Billboard* first = set->GetBillboard(0);
    first->position_ = Vector3(5.0f, 0.0f, 0.0f);
    first->size_ = Vector2(2.0f, 2.0f);
    first->enabled_ = true;
    set->Commit();

    REQUIRE(set->GetWorldBoundingBox().max_.x_ >= 5.0f);

    set->SetRelative(false);
    set->SetScaled(false);
    set->SetSorted(true);
    set->SetFixedScreenSize(true);
    REQUIRE_FALSE(set->IsRelative());
    REQUIRE_FALSE(set->IsScaled());
    REQUIRE(set->IsSorted());
    REQUIRE(set->IsFixedScreenSize());

    set->SetFaceCameraMode(FC_ROTATE_Y);
    REQUIRE(set->GetFaceCameraMode() == FC_ROTATE_Y);

    set->SetAnimationLodBias(2.0f);
    REQUIRE_EQ_F(set->GetAnimationLodBias(), 2.0f);

    SECTION("shrinking the bank drops the extra billboards")
    {
        set->SetNumBillboards(1);
        REQUIRE(set->GetNumBillboards() == 1);
        REQUIRE(set->GetBillboard(1) == nullptr);
    }

    SECTION("a disabled billboard is left out of the bounds")
    {
        first->enabled_ = false;
        set->Commit();
        REQUIRE(set->GetWorldBoundingBox().max_.x_ < 5.0f);
    }
}

TEST_CASE("CustomGeometry builds vertices at runtime", "[Graphics]")
{
    SharedPtr<Scene> scene(new Scene(HeadlessContext()));
    scene->CreateComponent<Octree>();
    auto* custom = scene->CreateChild("Custom")->CreateComponent<CustomGeometry>();

    REQUIRE(custom->GetNumGeometries() == 1);
    custom->BeginGeometry(0, TRIANGLE_LIST);
    custom->DefineVertex(Vector3(0.0f, 0.0f, 0.0f));
    custom->DefineNormal(Vector3::UP);
    custom->DefineVertex(Vector3(1.0f, 0.0f, 0.0f));
    custom->DefineNormal(Vector3::UP);
    custom->DefineVertex(Vector3(0.0f, 0.0f, 1.0f));
    custom->DefineNormal(Vector3::UP);
    custom->Commit();

    REQUIRE(custom->GetNumGeometries() == 1);
    REQUIRE(custom->GetNumVertices(0) == 3);
    REQUIRE(custom->GetVertex(0, 0) != nullptr);
    REQUIRE(custom->GetVertex(0, 0)->position_ == Vector3::ZERO);
    REQUIRE(custom->GetVertex(0, 99) == nullptr);
    REQUIRE(custom->GetVertex(99, 0) == nullptr);
    REQUIRE(custom->GetWorldBoundingBox().Defined());

    SECTION("a second geometry is kept separate")
    {
        custom->SetNumGeometries(2);
        custom->BeginGeometry(1, TRIANGLE_LIST);
        custom->DefineVertex(Vector3(10.0f, 0.0f, 0.0f));
        custom->DefineVertex(Vector3(11.0f, 0.0f, 0.0f));
        custom->DefineVertex(Vector3(10.0f, 0.0f, 1.0f));
        custom->Commit();

        REQUIRE(custom->GetNumGeometries() == 2);
        REQUIRE(custom->GetNumVertices(1) == 3);
        REQUIRE(custom->GetWorldBoundingBox().max_.x_ >= 11.0f);
    }

    SECTION("an index past the end of the list is refused rather than written through")
    {
        custom->BeginGeometry(custom->GetNumGeometries(), TRIANGLE_LIST);
        custom->DefineGeometry(custom->GetNumGeometries(), TRIANGLE_LIST, 3, false, false, false, false);
        REQUIRE(custom->GetNumGeometries() == 1);
        REQUIRE(custom->GetNumVertices(0) == 3);
    }

    SECTION("clearing empties every geometry")
    {
        custom->Clear();
        custom->Commit();
        REQUIRE(custom->GetNumVertices(0) == 0);
    }
}

TEST_CASE("RibbonTrail records points as its node moves", "[Graphics]")
{
    SharedPtr<Scene> scene(new Scene(HeadlessContext()));
    scene->CreateComponent<Octree>();
    Node* node = scene->CreateChild("Trail");
    auto* trail = node->CreateComponent<RibbonTrail>();
    trail->SetUpdateInvisible(true);

    trail->SetTrailType(TT_FACE_CAMERA);
    trail->SetLifetime(5.0f);
    trail->SetWidth(0.5f);
    trail->SetVertexDistance(0.1f);
    trail->SetEndColor(Color::RED);
    trail->SetStartColor(Color::BLUE);
    trail->SetEmitting(true);

    REQUIRE(trail->GetTrailType() == TT_FACE_CAMERA);
    REQUIRE_EQ_F(trail->GetLifetime(), 5.0f);
    REQUIRE_EQ_F(trail->GetWidth(), 0.5f);
    REQUIRE_EQ_F(trail->GetVertexDistance(), 0.1f);
    REQUIRE(trail->GetEndColor() == Color::RED);
    REQUIRE(trail->GetStartColor() == Color::BLUE);
    REQUIRE(trail->IsEmitting());

    SECTION("the trail records points as the node travels")
    {
        FrameInfo frame{};
        frame.timeStep_ = 0.1f;

        node->SetPosition(Vector3::ZERO);
        scene->Update(frame.timeStep_);
        trail->Update(frame);
        REQUIRE_FALSE(trail->GetWorldBoundingBox().Defined());

        for (int i = 1; i <= 20; ++i)
        {
            node->SetPosition(Vector3((float)i, 0.0f, 0.0f));
            scene->Update(frame.timeStep_);
            trail->Update(frame);
        }

        REQUIRE(trail->GetWorldBoundingBox().Defined());
        REQUIRE(trail->GetWorldBoundingBox().Size().x_ > 5.0f);
    }

    SECTION("a bone trail needs a parent node to hang from")
    {
        trail->SetTrailType(TT_BONE);
        REQUIRE(trail->GetTrailType() == TT_FACE_CAMERA);

        Node* holder = scene->CreateChild("Holder");
        node->SetParent(holder);
        trail->SetTrailType(TT_BONE);
        REQUIRE(trail->GetTrailType() == TT_BONE);

        trail->SetEndScale(2.0f);
        REQUIRE_EQ_F(trail->GetEndScale(), 2.0f);
    }
}

TEST_CASE("OcclusionBuffer rasterises occluders on the CPU", "[Graphics]")
{
    Context* context = HeadlessContext();
    SharedPtr<Scene> scene(new Scene(context));
    Node* cameraNode = scene->CreateChild("Camera");
    cameraNode->SetPosition(Vector3(0.0f, 0.0f, -10.0f));
    auto* camera = cameraNode->CreateComponent<Camera>();
    camera->SetAspectRatio(1.0f);
    camera->SetFarClip(1000.0f);

    SharedPtr<OcclusionBuffer> buffer(new OcclusionBuffer(context));
    REQUIRE(buffer->SetSize(64, 64, false));
    REQUIRE(buffer->GetWidth() == 64);
    REQUIRE(buffer->GetHeight() == 64);
    REQUIRE_FALSE(buffer->IsThreaded());

    buffer->SetView(camera);
    buffer->Reset();
    buffer->Clear();
    REQUIRE(buffer->GetNumTriangles() == 0);

    const Vector3 quad[] = {
        Vector3(-20.0f, -20.0f, 0.0f), Vector3(-20.0f, 20.0f, 0.0f), Vector3(20.0f, 20.0f, 0.0f),
        Vector3(-20.0f, -20.0f, 0.0f), Vector3(20.0f, 20.0f, 0.0f), Vector3(20.0f, -20.0f, 0.0f),
    };

    buffer->SetCullMode(CULL_NONE);
    REQUIRE(buffer->AddTriangles(Matrix3x4::IDENTITY, quad, sizeof(Vector3), 0, 6));
    buffer->DrawTriangles();
    REQUIRE(buffer->GetNumTriangles() >= 2);
    buffer->BuildDepthHierarchy();

    REQUIRE_FALSE(buffer->IsVisible(BoundingBox(Vector3(-1.0f, -1.0f, 10.0f), Vector3(1.0f, 1.0f, 12.0f))));
    REQUIRE(buffer->IsVisible(BoundingBox(Vector3(-1.0f, -1.0f, -5.0f), Vector3(1.0f, 1.0f, -3.0f))));

    SECTION("a box off to the side of the occluder stays visible")
    {
        REQUIRE(buffer->IsVisible(BoundingBox(Vector3(200.0f, 0.0f, 10.0f), Vector3(202.0f, 2.0f, 12.0f))));
    }

    SECTION("clearing the buffer makes everything visible again")
    {
        buffer->Clear();
        buffer->BuildDepthHierarchy();
        REQUIRE(buffer->IsVisible(BoundingBox(Vector3(-1.0f, -1.0f, 10.0f), Vector3(1.0f, 1.0f, 12.0f))));
    }

    SECTION("the triangle budget is enforced")
    {
        buffer->Reset();
        buffer->Clear();
        buffer->SetMaxTriangles(1);
        REQUIRE(buffer->GetMaxTriangles() == 1);
        REQUIRE_FALSE(buffer->AddTriangles(Matrix3x4::IDENTITY, quad, sizeof(Vector3), 0, 6));
    }

    SECTION("the indexed overload draws the same quad")
    {
        buffer->Reset();
        buffer->Clear();
        const unsigned short indices[] = {0, 1, 2, 3, 4, 5};
        REQUIRE(buffer->AddTriangles(Matrix3x4::IDENTITY, quad, sizeof(Vector3),
            indices, sizeof(unsigned short), 0, 6));
        buffer->DrawTriangles();
        buffer->BuildDepthHierarchy();
        REQUIRE_FALSE(buffer->IsVisible(BoundingBox(Vector3(-1.0f, -1.0f, 10.0f), Vector3(1.0f, 1.0f, 12.0f))));
    }
}

TEST_CASE("Vertex element helpers describe a buffer layout", "[Graphics]")
{
    PODVector<VertexElement> elements;
    elements.Push(VertexElement(TYPE_VECTOR3, SEM_POSITION));
    elements.Push(VertexElement(TYPE_VECTOR3, SEM_NORMAL));
    elements.Push(VertexElement(TYPE_VECTOR2, SEM_TEXCOORD));

    VertexBuffer::UpdateOffsets(elements);
    REQUIRE(VertexBuffer::GetVertexSize(elements) == sizeof(Vector3) * 2 + sizeof(Vector2));
    REQUIRE(elements[0].offset_ == 0);
    REQUIRE(elements[1].offset_ == sizeof(Vector3));
    REQUIRE(elements[2].offset_ == sizeof(Vector3) * 2);

    REQUIRE(VertexBuffer::GetElementOffset(elements, TYPE_VECTOR2, SEM_TEXCOORD) == sizeof(Vector3) * 2);
    REQUIRE(VertexBuffer::GetElementOffset(elements, TYPE_VECTOR4, SEM_TANGENT) == M_MAX_UNSIGNED);
    REQUIRE(VertexBuffer::HasElement(elements, TYPE_VECTOR3, SEM_NORMAL));
    REQUIRE_FALSE(VertexBuffer::HasElement(elements, TYPE_VECTOR3, SEM_TANGENT));

    SECTION("a legacy element mask expands into the same layout")
    {
        const PODVector<VertexElement> fromMask =
            VertexBuffer::GetElements(MASK_POSITION | MASK_NORMAL | MASK_TEXCOORD1);
        REQUIRE(fromMask.Size() == 3);
        REQUIRE(fromMask[0].semantic_ == SEM_POSITION);
        REQUIRE(VertexBuffer::GetVertexSize(MASK_POSITION | MASK_NORMAL | MASK_TEXCOORD1)
            == VertexBuffer::GetVertexSize(fromMask));
        REQUIRE(VertexBuffer::HasElement(fromMask, TYPE_VECTOR3, SEM_NORMAL));
        REQUIRE_FALSE(VertexBuffer::HasElement(fromMask, TYPE_VECTOR4, SEM_TANGENT));
    }

    SECTION("elements compare by type, semantic, and index")
    {
        REQUIRE(VertexElement(TYPE_VECTOR3, SEM_POSITION) == VertexElement(TYPE_VECTOR3, SEM_POSITION));
        REQUIRE(VertexElement(TYPE_VECTOR3, SEM_POSITION) != VertexElement(TYPE_VECTOR3, SEM_NORMAL));
        REQUIRE(VertexElement(TYPE_VECTOR2, SEM_TEXCOORD, 0) != VertexElement(TYPE_VECTOR2, SEM_TEXCOORD, 1));
    }
}

#include "TestUtils.h"
#include <Urho3D/IO/VectorBuffer.h>
#include <Urho3D/UI/UIElement.h>

using namespace Urho3D;
using namespace U3DTest;

TEST_CASE("UIElement manages hierarchy, lookup, tags, and variables", "[UI]")
{
    Context* context = HeadlessContext();
    SharedPtr<UIElement> root(new UIElement(context));
    root->SetName("Root");
    UIElement* first = root->CreateChild<UIElement>("First");
    UIElement* second = root->CreateChild<UIElement>("Second", 0);
    UIElement* nested = first->CreateChild<UIElement>("Nested");
    REQUIRE(root->GetNumChildren() == 2);
    REQUIRE(root->GetNumChildren(true) == 3);
    REQUIRE(root->GetChild(0) == second);
    REQUIRE(root->GetChild("Nested", true) == nested);
    REQUIRE(root->GetChild("Nested", false) == nullptr);

    nested->AddTag("target");
    nested->SetVar("kind", "leaf");
    REQUIRE(nested->HasTag("target"));
    REQUIRE(nested->GetVar("kind").GetString() == "leaf");
    REQUIRE(root->GetChildrenWithTag("target", true).Size() == 1);
    REQUIRE(root->GetChild(StringHash("kind"), "leaf", true) == nested);
    REQUIRE(nested->RemoveTag("target"));
    root->RemoveChild(second);
    REQUIRE(root->GetNumChildren() == 1);
}

TEST_CASE("UIElement constrains geometry and derives state", "[UI]")
{
    Context* context = HeadlessContext();
    SharedPtr<UIElement> parent(new UIElement(context));
    parent->SetPosition(10, 20);
    parent->SetSize(200, 100);
    UIElement* child = parent->CreateChild<UIElement>();
    child->SetMinSize(20, 10);
    child->SetMaxSize(80, 60);
    child->SetSize(5, 100);
    REQUIRE(child->GetSize() == IntVector2(20, 60));
    child->SetPosition(3, 4);
    REQUIRE(child->GetScreenPosition() == IntVector2(13, 24));
    REQUIRE(child->ScreenToElement(IntVector2(15, 30)) == IntVector2(2, 6));
    REQUIRE(child->ElementToScreen(IntVector2(2, 6)) == IntVector2(15, 30));
    REQUIRE(child->IsInside(IntVector2(0, 0), false));
    REQUIRE(child->IsInside(IntVector2(19, 59), false));
    REQUIRE_FALSE(child->IsInside(IntVector2(-1, 0), false));
    REQUIRE_FALSE(child->IsInside(IntVector2(0, -1), false));
    REQUIRE_FALSE(child->IsInside(IntVector2(20, 0), false));
    REQUIRE_FALSE(child->IsInside(IntVector2(0, 60), false));

    parent->SetOpacity(0.5f);
    child->SetOpacity(0.5f);
    REQUIRE_EQ_F(child->GetDerivedOpacity(), 0.25f);
    parent->SetVisible(false);
    REQUIRE_FALSE(child->IsVisibleEffective());
    parent->SetVisible(true);
    parent->SetEnabled(false);
    REQUIRE_FALSE(child->IsEnabled());
}

TEST_CASE("UIElement layouts and XML round trip", "[UI]")
{
    Context* context = HeadlessContext();
    SharedPtr<UIElement> source(new UIElement(context));
    source->SetName("Panel");
    source->SetLayout(LM_HORIZONTAL, 5, IntRect(2, 3, 4, 5));
    source->SetSize(100, 30);
    UIElement* a = source->CreateChild<UIElement>("A");
    UIElement* b = source->CreateChild<UIElement>("B");
    a->SetMinSize(10, 10);
    b->SetMinSize(20, 10);
    source->UpdateLayout();
    REQUIRE(source->GetLayoutMode() == LM_HORIZONTAL);
    REQUIRE(b->GetPosition().x_ >= a->GetPosition().x_ + a->GetWidth());

    VectorBuffer buffer;
    REQUIRE(source->SaveXML(buffer));
    buffer.Seek(0);
    SharedPtr<UIElement> restored(new UIElement(context));
    REQUIRE(restored->LoadXML(buffer));
    REQUIRE(restored->GetName() == "Panel");
    REQUIRE(restored->GetNumChildren() == 2);
    REQUIRE(restored->GetChild(String("A")) != nullptr);
}

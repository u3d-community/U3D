#include "TestUtils.h"

#include <Urho3D/IO/VectorBuffer.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Resource/XMLFile.h>
#include <Urho3D/UI/BorderImage.h>
#include <Urho3D/UI/Button.h>
#include <Urho3D/UI/Font.h>
#include <Urho3D/UI/CheckBox.h>
#include <Urho3D/UI/DropDownList.h>
#include <Urho3D/UI/LineEdit.h>
#include <Urho3D/UI/ListView.h>
#include <Urho3D/UI/Menu.h>
#include <Urho3D/UI/ScrollView.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/UI/UI.h>
#include <Urho3D/UI/UIElement.h>
#include <Urho3D/UI/UIEvents.h>
#include <Urho3D/UI/Window.h>

using namespace Urho3D;
using namespace U3DTest;

namespace
{

template <class T> SharedPtr<T> MakeWidget()
{
    return SharedPtr<T>(new T(HeadlessContext()));
}

}

TEST_CASE("BorderImage carries its image and border rectangles", "[UI]")
{
    SharedPtr<BorderImage> image = MakeWidget<BorderImage>();

    image->SetImageRect(IntRect(1, 2, 33, 44));
    REQUIRE(image->GetImageRect() == IntRect(1, 2, 33, 44));

    image->SetBorder(IntRect(2, 3, 4, 5));
    REQUIRE(image->GetBorder() == IntRect(2, 3, 4, 5));

    image->SetImageBorder(IntRect(6, 7, 8, 9));
    REQUIRE(image->GetImageBorder() == IntRect(6, 7, 8, 9));

    image->SetBlendMode(BLEND_ADD);
    REQUIRE(image->GetBlendMode() == BLEND_ADD);

    image->SetTiled(true);
    REQUIRE(image->IsTiled());

    image->SetHoverOffset(3, 4);
    REQUIRE(image->GetHoverOffset() == IntVector2(3, 4));

    SECTION("a full image rectangle needs a texture to measure against")
    {
        REQUIRE(image->GetTexture() == nullptr);
        image->SetFullImageRect();
        REQUIRE(image->GetImageRect() == IntRect(1, 2, 33, 44));
    }
}

TEST_CASE("Button tracks its pressed state and offsets", "[UI]")
{
    SharedPtr<Button> button = MakeWidget<Button>();
    button->SetSize(100, 40);

    REQUIRE_FALSE(button->IsPressed());
    REQUIRE(button->IsEnabled());

    button->SetPressedOffset(2, 3);
    button->SetPressedChildOffset(-1, -2);
    REQUIRE(button->GetPressedOffset() == IntVector2(2, 3));
    REQUIRE(button->GetPressedChildOffset() == IntVector2(-1, -2));

    button->SetRepeat(0.5f, 0.1f);
    REQUIRE_EQ_F(button->GetRepeatDelay(), 0.5f);
    REQUIRE_EQ_F(button->GetRepeatRate(), 0.1f);

    button->SetRepeat(-1.0f, -1.0f);
    REQUIRE(button->GetRepeatDelay() >= 0.0f);
    REQUIRE(button->GetRepeatRate() >= 0.0f);

    SECTION("a press inside the button latches it until release")
    {
        button->OnClickBegin(IntVector2(10, 10), IntVector2(10, 10), MOUSEB_LEFT, MOUSEB_LEFT, QUAL_NONE, nullptr);
        REQUIRE(button->IsPressed());

        button->OnClickEnd(IntVector2(10, 10), IntVector2(10, 10), MOUSEB_LEFT, MOUSEB_NONE, QUAL_NONE, nullptr, button);
        REQUIRE_FALSE(button->IsPressed());
    }

    SECTION("a disabled button ignores hovering")
    {
        button->SetEnabled(false);
        REQUIRE_FALSE(button->IsEnabled());
        REQUIRE_FALSE(button->IsHovering());
    }
}

TEST_CASE("CheckBox toggles when clicked", "[UI]")
{
    SharedPtr<CheckBox> box = MakeWidget<CheckBox>();
    box->SetSize(20, 20);

    REQUIRE_FALSE(box->IsChecked());
    box->SetChecked(true);
    REQUIRE(box->IsChecked());

    box->SetCheckedOffset(5, 6);
    REQUIRE(box->GetCheckedOffset() == IntVector2(5, 6));

    box->OnClickBegin(IntVector2(5, 5), IntVector2(5, 5), MOUSEB_LEFT, MOUSEB_LEFT, QUAL_NONE, nullptr);
    REQUIRE_FALSE(box->IsChecked());

    box->OnClickBegin(IntVector2(5, 5), IntVector2(5, 5), MOUSEB_LEFT, MOUSEB_LEFT, QUAL_NONE, nullptr);
    REQUIRE(box->IsChecked());
}

TEST_CASE("LineEdit edits text and clamps its cursor", "[UI]")
{
    SharedPtr<LineEdit> edit = MakeWidget<LineEdit>();
    edit->SetSize(200, 20);

    REQUIRE(edit->GetText().Empty());
    REQUIRE(edit->GetTextElement() != nullptr);
    REQUIRE(edit->GetCursor() != nullptr);

    edit->SetText("hello");
    REQUIRE(edit->GetText() == "hello");
    REQUIRE(edit->GetCursorPosition() == 5);

    edit->SetCursorPosition(3);
    REQUIRE(edit->GetCursorPosition() == 3);

    edit->SetCursorPosition(99);
    REQUIRE(edit->GetCursorPosition() == 5);

    edit->SetMaxLength(3);
    REQUIRE(edit->GetMaxLength() == 3);
    edit->SetText("abcdefgh");
    REQUIRE(edit->GetText().Length() == 3);

    SECTION("the echo character hides the text without changing it")
    {
        edit->SetMaxLength(0);
        edit->SetText("secret");
        edit->SetEchoCharacter('*');
        REQUIRE(edit->GetEchoCharacter() == '*');
        REQUIRE(edit->GetText() == "secret");
        REQUIRE(edit->GetTextElement()->GetText() != "secret");
    }

    SECTION("editing flags round trip")
    {
        edit->SetCursorMovable(false);
        edit->SetTextSelectable(false);
        edit->SetTextCopyable(false);
        REQUIRE_FALSE(edit->IsCursorMovable());
        REQUIRE_FALSE(edit->IsTextSelectable());
        REQUIRE_FALSE(edit->IsTextCopyable());

        edit->SetCursorBlinkRate(-1.0f);
        REQUIRE(edit->GetCursorBlinkRate() >= 0.0f);
    }

    SECTION("setting the same text again is a no op")
    {
        edit->SetMaxLength(0);
        edit->SetText("stable");
        edit->SetCursorPosition(2);
        edit->SetText("stable");
        REQUIRE(edit->GetCursorPosition() == 2);
    }
}

TEST_CASE("Text keeps its layout settings without a font face", "[UI]")
{
    Context* context = HeadlessContext();
    auto* font = context->GetSubsystem<ResourceCache>()->GetResource<Font>("Fonts/Anonymous Pro.ttf");
    REQUIRE(font != nullptr);

    REQUIRE(font->GetFontType() == FONT_NONE);
    REQUIRE(font->GetFace(16.0f) == nullptr);

    SharedPtr<Text> text = MakeWidget<Text>();
    REQUIRE(text->SetFont(font, 16));
    REQUIRE(text->GetFont() == font);
    REQUIRE_EQ_F(text->GetFontSize(), 16.0f);

    text->SetText("Hello");
    REQUIRE(text->GetText() == "Hello");
    REQUIRE(text->GetNumRows() == 0);

    SECTION("the point size is clamped to what FreeType can rasterise")
    {
        text->SetFontSize(0.0f);
        REQUIRE(text->GetFontSize() >= 1.0f);
    }

    SECTION("alignment, spacing, and effects round trip")
    {
        text->SetTextAlignment(HA_CENTER);
        REQUIRE(text->GetTextAlignment() == HA_CENTER);

        text->SetRowSpacing(2.0f);
        REQUIRE_EQ_F(text->GetRowSpacing(), 2.0f);
        text->SetRowSpacing(-1.0f);
        REQUIRE(text->GetRowSpacing() > 0.0f);

        text->SetWordwrap(true);
        REQUIRE(text->GetWordwrap());

        text->SetTextEffect(TE_SHADOW);
        text->SetEffectShadowOffset(IntVector2(2, 2));
        text->SetEffectColor(Color::BLACK);
        REQUIRE(text->GetTextEffect() == TE_SHADOW);
        REQUIRE(text->GetEffectShadowOffset() == IntVector2(2, 2));
        REQUIRE(text->GetEffectColor() == Color::BLACK);

        text->SetEffectStrokeThickness(3);
        REQUIRE(text->GetEffectStrokeThickness() == 3);
    }

    SECTION("the selection range is clamped to what is left of the text")
    {
        text->SetSelection(1, 2);
        REQUIRE(text->GetSelectionStart() == 1);
        REQUIRE(text->GetSelectionLength() == 2);

        text->SetSelection(2, 4);
        REQUIRE(text->GetSelectionStart() == 2);
        REQUIRE(text->GetSelectionLength() == 3);

        text->ClearSelection();
        REQUIRE(text->GetSelectionLength() == 0);
    }

    SECTION("auto localisation of the text can be turned on")
    {
        text->SetAutoLocalizable(true);
        REQUIRE(text->GetAutoLocalizable());
        text->SetAutoLocalizable(false);
        REQUIRE_FALSE(text->GetAutoLocalizable());
    }
}

TEST_CASE("Window drags, resizes, and constrains its size", "[UI]")
{
    SharedPtr<Window> window = MakeWidget<Window>();
    window->SetSize(200, 150);
    window->SetPosition(10, 20);

    REQUIRE_FALSE(window->IsMovable());
    window->SetMovable(true);
    REQUIRE(window->IsMovable());

    window->SetResizable(true);
    REQUIRE(window->IsResizable());

    window->SetResizeBorder(IntRect(4, 4, 4, 4));
    REQUIRE(window->GetResizeBorder() == IntRect(4, 4, 4, 4));

    SECTION("the modal settings round trip")
    {
        window->SetModalShadeColor(Color::GRAY);
        REQUIRE(window->GetModalShadeColor() == Color::GRAY);
        window->SetModalAutoDismiss(false);
        REQUIRE_FALSE(window->GetModalAutoDismiss());
        window->SetModalFrameColor(Color::RED);
        REQUIRE(window->GetModalFrameColor() == Color::RED);
    }

    SECTION("dragging moves the window by the cursor delta")
    {
        window->SetResizable(false);
        window->OnDragBegin(IntVector2(40, 30), IntVector2(50, 50), MOUSEB_LEFT, QUAL_NONE, nullptr);
        window->OnDragMove(IntVector2(45, 35), IntVector2(55, 55), IntVector2(5, 5),
            MOUSEB_LEFT, QUAL_NONE, nullptr);
        REQUIRE(window->GetPosition() == IntVector2(15, 25));
        window->OnDragEnd(IntVector2(45, 35), IntVector2(55, 55), MOUSEB_LEFT, MOUSEB_NONE, nullptr);
    }

    SECTION("an immovable window ignores the drag")
    {
        window->SetResizable(false);
        window->SetMovable(false);
        const IntVector2 before = window->GetPosition();
        window->OnDragBegin(IntVector2(40, 30), IntVector2(50, 50), MOUSEB_LEFT, QUAL_NONE, nullptr);
        window->OnDragMove(IntVector2(45, 35), IntVector2(55, 55), IntVector2(5, 5),
            MOUSEB_LEFT, QUAL_NONE, nullptr);
        REQUIRE(window->GetPosition() == before);
    }
}

TEST_CASE("ScrollView clamps its view position to the content", "[UI]")
{
    SharedPtr<ScrollView> view = MakeWidget<ScrollView>();
    view->SetSize(100, 100);

    REQUIRE(view->GetHorizontalScrollBar() != nullptr);
    REQUIRE(view->GetVerticalScrollBar() != nullptr);
    REQUIRE(view->GetScrollPanel() != nullptr);

    auto* content = new UIElement(HeadlessContext());
    content->SetSize(400, 400);
    view->SetContentElement(content);
    REQUIRE(view->GetContentElement() == content);

    view->SetViewPosition(50, 60);
    REQUIRE(view->GetViewPosition() == IntVector2(50, 60));

    view->SetViewPosition(-100, -100);
    REQUIRE(view->GetViewPosition() == IntVector2::ZERO);

    view->SetViewPosition(10000, 10000);
    REQUIRE(view->GetViewPosition().x_ <= 400);
    REQUIRE(view->GetViewPosition().y_ <= 400);

    SECTION("scroll steps and speeds are kept non negative")
    {
        view->SetScrollStep(-1.0f);
        REQUIRE(view->GetScrollStep() >= 0.0f);
        view->SetPageStep(-1.0f);
        REQUIRE(view->GetPageStep() >= 0.0f);

        view->SetScrollStep(0.2f);
        REQUIRE_EQ_F(view->GetScrollStep(), 0.2f);
    }

    SECTION("the scroll bars can be hidden when they are not needed")
    {
        view->SetScrollBarsAutoVisible(false);
        REQUIRE_FALSE(view->GetScrollBarsAutoVisible());
    }
}

TEST_CASE("ListView manages items and selection", "[UI]")
{
    SharedPtr<ListView> list = MakeWidget<ListView>();
    list->SetSize(200, 200);

    REQUIRE(list->GetNumItems() == 0);
    REQUIRE(list->GetSelection() == M_MAX_UNSIGNED);

    for (int i = 0; i < 4; ++i)
    {
        auto* item = new UIElement(HeadlessContext());
        item->SetName("Item" + String(i));
        item->SetSize(180, 20);
        list->AddItem(item);
    }

    REQUIRE(list->GetNumItems() == 4);
    REQUIRE(list->GetItem(0) != nullptr);
    REQUIRE(list->GetItem(99) == nullptr);
    REQUIRE(list->FindItem(list->GetItem(2)) == 2);

    list->SetSelection(2);
    REQUIRE(list->GetSelection() == 2);
    REQUIRE(list->GetSelectedItem() == list->GetItem(2));

    list->ClearSelection();
    REQUIRE(list->GetSelection() == M_MAX_UNSIGNED);
    REQUIRE(list->GetSelectedItem() == nullptr);

    SECTION("multiselect keeps several indices at once")
    {
        list->SetMultiselect(true);
        REQUIRE(list->GetMultiselect());

        PODVector<unsigned> wanted;
        wanted.Push(0);
        wanted.Push(3);
        list->SetSelections(wanted);
        REQUIRE(list->GetSelections().Size() == 2);
        REQUIRE(list->GetSelectedItems().Size() == 2);
        REQUIRE(list->IsSelected(0));
        REQUIRE(list->IsSelected(3));
        REQUIRE_FALSE(list->IsSelected(1));

        list->ToggleSelection(0);
        REQUIRE_FALSE(list->IsSelected(0));
    }

    SECTION("removing an item shrinks the list")
    {
        UIElement* second = list->GetItem(1);
        list->RemoveItem(second);
        REQUIRE(list->GetNumItems() == 3);
        REQUIRE(list->FindItem(second) == M_MAX_UNSIGNED);

        list->RemoveItem(0u);
        REQUIRE(list->GetNumItems() == 2);

        list->RemoveAllItems();
        REQUIRE(list->GetNumItems() == 0);
    }

    SECTION("inserting places the item at the requested index")
    {
        auto* inserted = new UIElement(HeadlessContext());
        inserted->SetName("Inserted");
        list->InsertItem(1, inserted);
        REQUIRE(list->GetNumItems() == 5);
        REQUIRE(list->GetItem(1) == inserted);
    }

    SECTION("a hierarchy mode list expands and collapses a child item")
    {
        list->RemoveAllItems();
        list->SetHierarchyMode(true);
        REQUIRE(list->GetHierarchyMode());

        auto* parent = new UIElement(HeadlessContext());
        parent->SetSize(180, 20);
        list->AddItem(parent);

        auto* child = new UIElement(HeadlessContext());
        child->SetSize(180, 20);
        list->InsertItem(1, child, parent);
        REQUIRE(list->GetNumItems() == 2);
        REQUIRE(child->IsVisible());

        list->Expand(0, false);
        REQUIRE_FALSE(list->IsExpanded(0));
        REQUIRE_FALSE(child->IsVisible());

        list->Expand(0, true);
        REQUIRE(list->IsExpanded(0));
        REQUIRE(child->IsVisible());
    }
}

TEST_CASE("DropDownList shows a popup list", "[UI]")
{
    SharedPtr<DropDownList> drop = MakeWidget<DropDownList>();
    drop->SetSize(120, 24);

    REQUIRE(drop->GetListView() != nullptr);
    REQUIRE(drop->GetNumItems() == 0);
    REQUIRE(drop->GetSelection() == M_MAX_UNSIGNED);

    for (int i = 0; i < 3; ++i)
    {
        auto* item = new UIElement(HeadlessContext());
        item->SetName("Option" + String(i));
        item->SetSize(100, 20);
        drop->AddItem(item);
    }

    REQUIRE(drop->GetNumItems() == 3);
    drop->SetSelection(1);
    REQUIRE(drop->GetSelection() == 1);
    REQUIRE(drop->GetSelectedItem() == drop->GetItem(1));

    drop->SetResizePopup(true);
    REQUIRE(drop->GetResizePopup());

    SECTION("showing and hiding the popup flips the flag")
    {
        drop->ShowPopup(true);
        REQUIRE(drop->GetShowPopup());
        drop->ShowPopup(false);
        REQUIRE_FALSE(drop->GetShowPopup());
    }

    SECTION("clearing removes every option")
    {
        drop->RemoveAllItems();
        REQUIRE(drop->GetNumItems() == 0);
        REQUIRE(drop->GetSelectedItem() == nullptr);
    }
}

TEST_CASE("Menu attaches a popup and an accelerator", "[UI]")
{
    SharedPtr<Menu> menu = MakeWidget<Menu>();
    menu->SetSize(100, 20);

    SharedPtr<UIElement> plain(new UIElement(HeadlessContext()));
    menu->SetPopup(plain);
    REQUIRE(menu->GetPopup() == nullptr);

    auto* popup = new Window(HeadlessContext());
    popup->SetSize(100, 60);
    menu->SetPopup(popup);
    REQUIRE(menu->GetPopup() == popup);

    menu->SetPopupOffset(2, 3);
    REQUIRE(menu->GetPopupOffset() == IntVector2(2, 3));

    menu->SetAccelerator(KEY_S, QUAL_CTRL);
    REQUIRE(menu->GetAcceleratorKey() == KEY_S);
    REQUIRE(menu->GetAcceleratorQualifiers() == QUAL_CTRL);

    menu->ShowPopup(true);
    REQUIRE(menu->GetShowPopup());
    menu->ShowPopup(false);
    REQUIRE_FALSE(menu->GetShowPopup());
}

TEST_CASE("UI subsystem clamps its settings and resolves elements by position", "[UI]")
{
    UI* ui = HeadlessContext()->GetSubsystem<UI>();
    REQUIRE(ui != nullptr);
    REQUIRE(ui->GetRoot() != nullptr);

    ui->SetDoubleClickInterval(-1.0f);
    REQUIRE(ui->GetDoubleClickInterval() >= 0.0f);
    ui->SetDoubleClickInterval(0.4f);
    REQUIRE_EQ_F(ui->GetDoubleClickInterval(), 0.4f);

    ui->SetDragBeginInterval(-1.0f);
    REQUIRE(ui->GetDragBeginInterval() >= 0.0f);

    ui->SetDragBeginDistance(-5);
    REQUIRE(ui->GetDragBeginDistance() >= 0);

    ui->SetDefaultToolTipDelay(-1.0f);
    REQUIRE(ui->GetDefaultToolTipDelay() >= 0.0f);

    ui->SetMaxFontTextureSize(1);
    REQUIRE(ui->GetMaxFontTextureSize() >= 128);

    ui->SetScale(0.0f);
    REQUIRE(ui->GetScale() > 0.0f);
    ui->SetScale(1.0f);

    ui->SetNonFocusedMouseWheel(true);
    REQUIRE(ui->IsNonFocusedMouseWheel());
    ui->SetNonFocusedMouseWheel(false);

    SECTION("hit testing finds the topmost enabled element under a given root")
    {
        SharedPtr<UIElement> root(new UIElement(HeadlessContext()));
        root->SetSize(400, 400);

        auto* panel = root->CreateChild<BorderImage>("Panel");
        panel->SetPosition(10, 10);
        panel->SetSize(100, 100);
        panel->SetEnabled(true);

        REQUIRE(ui->GetElementAt(root, IntVector2(50, 50)) == panel);
        REQUIRE(ui->GetElementAt(root, IntVector2(300, 300)) == nullptr);

        auto* front = root->CreateChild<BorderImage>("Front");
        front->SetPosition(20, 20);
        front->SetSize(50, 50);
        front->SetEnabled(true);
        REQUIRE(ui->GetElementAt(root, IntVector2(50, 50)) == front);

        panel->SetEnabled(false);
        front->SetEnabled(false);
        REQUIRE(ui->GetElementAt(root, IntVector2(50, 50), true) == nullptr);
        REQUIRE(ui->GetElementAt(root, IntVector2(50, 50), false) == front);
    }

    SECTION("focus can be set and cleared")
    {
        UIElement* root = ui->GetRoot();
        auto* edit = root->CreateChild<LineEdit>("Focusable");
        edit->SetSize(100, 20);
        edit->SetFocusMode(FM_FOCUSABLE);

        ui->SetFocusElement(edit);
        REQUIRE(ui->GetFocusElement() == edit);
        REQUIRE(edit->HasFocus());

        ui->SetFocusElement(nullptr);
        REQUIRE(ui->GetFocusElement() == nullptr);

        root->RemoveChild(edit);
    }

    SECTION("a layout round trips through XML")
    {
        SharedPtr<UIElement> source(new UIElement(HeadlessContext()));
        source->SetName("Saved");
        source->SetSize(64, 32);
        source->CreateChild<Button>("Child");

        VectorBuffer buffer;
        REQUIRE(ui->SaveLayout(buffer, source));
        buffer.Seek(0);

        SharedPtr<UIElement> loaded = ui->LoadLayout(buffer);
        REQUIRE(loaded.NotNull());
        REQUIRE(loaded->GetName() == "Saved");
        REQUIRE(loaded->GetNumChildren() == 1);
        REQUIRE(loaded->GetChild(String("Child")) != nullptr);
    }
}

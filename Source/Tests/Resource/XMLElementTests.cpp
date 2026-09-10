#include "TestUtils.h"

#include <Urho3D/IO/VectorBuffer.h>
#include <Urho3D/Resource/XMLElement.h>
#include <Urho3D/Resource/XMLFile.h>

using namespace Urho3D;
using namespace U3DTest;

namespace
{

SharedPtr<XMLFile> MakeDocument(Context* context, XMLElement& root)
{
    SharedPtr<XMLFile> document(new XMLFile(context));
    root = document->CreateRoot("root");
    return document;
}

}

TEST_CASE("XMLElement writes and reads every scalar attribute type", "[Resource]")
{
    TestContext context;
    XMLElement root;
    SharedPtr<XMLFile> document = MakeDocument(context, root);

    REQUIRE(root.SetBool("bool", true));
    REQUIRE(root.SetInt("int", -42));
    REQUIRE(root.SetUInt("uint", 42u));
    REQUIRE(root.SetInt64("int64", -1234567890123LL));
    REQUIRE(root.SetUInt64("uint64", 1234567890123ULL));
    REQUIRE(root.SetFloat("float", 1.5f));
    REQUIRE(root.SetDouble("double", 2.5));
    REQUIRE(root.SetString("string", "hello"));

    REQUIRE(root.GetBool("bool"));
    REQUIRE(root.GetInt("int") == -42);
    REQUIRE(root.GetUInt("uint") == 42u);
    REQUIRE(root.GetInt64("int64") == -1234567890123LL);
    REQUIRE(root.GetUInt64("uint64") == 1234567890123ULL);
    REQUIRE_EQ_F(root.GetFloat("float"), 1.5f);
    REQUIRE(root.GetDouble("double") == 2.5);
    REQUIRE(root.GetAttribute("string") == "hello");

    SECTION("a missing attribute reads back as a zero value")
    {
        REQUIRE_FALSE(root.HasAttribute("absent"));
        REQUIRE_FALSE(root.GetBool("absent"));
        REQUIRE(root.GetInt("absent") == 0);
        REQUIRE(root.GetUInt("absent") == 0u);
        REQUIRE_EQ_F(root.GetFloat("absent"), 0.0f);
        REQUIRE(root.GetAttribute("absent").Empty());
    }

    SECTION("attribute names are listed and can be read case insensitively")
    {
        REQUIRE(root.HasAttribute("int"));
        REQUIRE(root.GetAttributeNames().Size() >= 8);
        REQUIRE(root.GetAttributeLower("string") == "hello");
        REQUIRE(root.GetAttributeUpper("string") == "HELLO");
    }

    SECTION("removing an attribute takes it out of the element")
    {
        REQUIRE(root.RemoveAttribute("int"));
        REQUIRE_FALSE(root.HasAttribute("int"));
    }
}

TEST_CASE("XMLElement writes and reads the maths types", "[Resource]")
{
    TestContext context;
    XMLElement root;
    SharedPtr<XMLFile> document = MakeDocument(context, root);

    const Vector2 vector2(1.0f, 2.0f);
    const Vector3 vector3(1.0f, 2.0f, 3.0f);
    const Vector4 vector4(1.0f, 2.0f, 3.0f, 4.0f);
    const IntVector2 intVector2(1, 2);
    const IntVector3 intVector3(1, 2, 3);
    const IntRect intRect(1, 2, 3, 4);
    const Rect rect(1.0f, 2.0f, 3.0f, 4.0f);
    const Color color(0.25f, 0.5f, 0.75f, 1.0f);
    const Quaternion quaternion(30.0f, Vector3::UP);

    REQUIRE(root.SetVector2("vector2", vector2));
    REQUIRE(root.SetVector3("vector3", vector3));
    REQUIRE(root.SetVector4("vector4", vector4));
    REQUIRE(root.SetIntVector2("intVector2", intVector2));
    REQUIRE(root.SetIntVector3("intVector3", intVector3));
    REQUIRE(root.SetIntRect("intRect", intRect));
    REQUIRE(root.SetRect("rect", rect));
    REQUIRE(root.SetColor("color", color));
    REQUIRE(root.SetQuaternion("quaternion", quaternion));
    REQUIRE(root.SetMatrix3("matrix3", Matrix3::IDENTITY));
    REQUIRE(root.SetMatrix3x4("matrix3x4", Matrix3x4::IDENTITY));
    REQUIRE(root.SetMatrix4("matrix4", Matrix4::IDENTITY));

    REQUIRE(root.GetVector2("vector2") == vector2);
    REQUIRE(root.GetVector3("vector3") == vector3);
    REQUIRE(root.GetVector4("vector4") == vector4);
    REQUIRE(root.GetIntVector2("intVector2") == intVector2);
    REQUIRE(root.GetIntVector3("intVector3") == intVector3);
    REQUIRE(root.GetIntRect("intRect") == intRect);
    REQUIRE(root.GetRect("rect") == rect);
    REQUIRE(root.GetColor("color") == color);
    REQUIRE_NEAR(Abs(root.GetQuaternion("quaternion").DotProduct(quaternion)), 1.0f, 0.0001f);
    REQUIRE(root.GetMatrix3("matrix3").Equals(Matrix3::IDENTITY));
    REQUIRE(root.GetMatrix3x4("matrix3x4").Equals(Matrix3x4::IDENTITY));
    REQUIRE(root.GetMatrix4("matrix4").Equals(Matrix4::IDENTITY));

    SECTION("a bounding box round trips through its own child element")
    {
        XMLElement child = root.CreateChild("box");
        const BoundingBox box(Vector3(-1.0f, -2.0f, -3.0f), Vector3(4.0f, 5.0f, 6.0f));
        REQUIRE(child.SetBoundingBox(box));

        const BoundingBox restored = child.GetBoundingBox();
        REQUIRE(restored.min_.Equals(box.min_));
        REQUIRE(restored.max_.Equals(box.max_));
    }

    SECTION("a vector variant is written as a bare number list")
    {
        REQUIRE(root.SetVectorVariant("vectorVariant", Variant(vector3)));
        REQUIRE(root.GetVectorVariant("vectorVariant").GetVector3() == vector3);
    }
}

TEST_CASE("XMLElement round trips the container variant types", "[Resource]")
{
    TestContext context;
    XMLElement root;
    SharedPtr<XMLFile> document = MakeDocument(context, root);

    SECTION("a variant carries its own type")
    {
        XMLElement child = root.CreateChild("variant");
        REQUIRE(child.SetVariant(Variant(Vector3(1.0f, 2.0f, 3.0f))));

        const Variant restored = child.GetVariant();
        REQUIRE(restored.GetType() == VAR_VECTOR3);
        REQUIRE(restored.GetVector3() == Vector3(1.0f, 2.0f, 3.0f));
    }

    SECTION("a buffer round trips as a byte list")
    {
        PODVector<unsigned char> buffer;
        buffer.Push(1);
        buffer.Push(127);
        buffer.Push(255);

        REQUIRE(root.SetBuffer("buffer", buffer));
        REQUIRE(root.GetBuffer("buffer") == buffer);

        PODVector<unsigned char> destination(3);
        REQUIRE(root.GetBuffer("buffer", destination.Buffer(), destination.Size()));
        REQUIRE(destination == buffer);
    }

    SECTION("a resource ref of a registered type round trips")
    {
        XMLElement registeredRoot;
        SharedPtr<XMLFile> registeredDocument = MakeDocument(HeadlessContext(), registeredRoot);
        XMLElement child = registeredRoot.CreateChild("ref");

        const ResourceRef reference(StringHash("Model"), "Models/Box.mdl");
        REQUIRE(child.SetResourceRef(reference));

        const ResourceRef restored = child.GetResourceRef();
        REQUIRE(restored.type_ == reference.type_);
        REQUIRE(restored.name_ == reference.name_);
    }

    SECTION("an unregistered type leaves the ref unreadable")
    {
        XMLElement child = root.CreateChild("ref");
        REQUIRE(child.SetResourceRef(ResourceRef(StringHash("NeverRegisteredType"), "Models/Box.mdl")));

        const ResourceRef restored = child.GetResourceRef();
        REQUIRE(restored.name_.Empty());
    }

    SECTION("a resource ref list of a registered type round trips")
    {
        XMLElement registeredRoot;
        SharedPtr<XMLFile> registeredDocument = MakeDocument(HeadlessContext(), registeredRoot);
        XMLElement child = registeredRoot.CreateChild("refList");

        ResourceRefList list(StringHash("Material"));
        list.names_.Push("Materials/A.xml");
        list.names_.Push("Materials/B.xml");
        REQUIRE(child.SetResourceRefList(list));

        const ResourceRefList restored = child.GetResourceRefList();
        REQUIRE(restored.type_ == list.type_);
        REQUIRE(restored.names_ == list.names_);
    }

    SECTION("a variant vector round trips element by element")
    {
        XMLElement child = root.CreateChild("vector");
        VariantVector values;
        values.Push(Variant(1));
        values.Push(Variant("two"));
        values.Push(Variant(Vector3::ONE));
        REQUIRE(child.SetVariantVector(values));

        const VariantVector restored = child.GetVariantVector();
        REQUIRE(restored.Size() == 3);
        REQUIRE(restored[0].GetInt() == 1);
        REQUIRE(restored[1].GetString() == "two");
        REQUIRE(restored[2].GetVector3() == Vector3::ONE);
    }

    SECTION("a string vector round trips")
    {
        XMLElement child = root.CreateChild("strings");
        StringVector strings;
        strings.Push("one");
        strings.Push("two");
        REQUIRE(child.SetStringVector(strings));
        REQUIRE(child.GetStringVector() == strings);
    }

    SECTION("a variant map round trips by hashed key")
    {
        XMLElement child = root.CreateChild("map");
        VariantMap map;
        map[StringHash("first")] = 1;
        map[StringHash("second")] = "two";
        REQUIRE(child.SetVariantMap(map));

        const VariantMap restored = child.GetVariantMap();
        REQUIRE(restored.Size() == 2);
        REQUIRE(restored[StringHash("first")]->GetInt() == 1);
        REQUIRE(restored[StringHash("second")]->GetString() == "two");
    }
}

TEST_CASE("XMLElement navigates and edits a document tree", "[Resource]")
{
    TestContext context;
    XMLElement root;
    SharedPtr<XMLFile> document = MakeDocument(context, root);

    REQUIRE_FALSE(root.IsNull());
    REQUIRE(static_cast<bool>(root));
    REQUIRE(root.GetName() == "root");
    REQUIRE(root.GetFile() == document);

    XMLElement first = root.CreateChild("child");
    XMLElement second = root.CreateChild("child");
    XMLElement other = root.CreateChild("other");

    first.SetInt("index", 0);
    second.SetInt("index", 1);

    REQUIRE(root.GetChild("child").GetInt("index") == 0);
    REQUIRE(root.GetChild("child").GetNext("child").GetInt("index") == 1);
    REQUIRE(root.GetChild("missing").IsNull());

    REQUIRE(first.GetParent().GetName() == "root");
    REQUIRE(second.GetNext("child").IsNull());
    REQUIRE(root.GetChild().GetName() == "child");

    SECTION("an existing child is reused rather than duplicated")
    {
        XMLElement fetched = root.GetOrCreateChild("other");
        REQUIRE_FALSE(fetched.IsNull());
        REQUIRE(fetched.GetName() == "other");

        XMLElement created = root.GetOrCreateChild("brandNew");
        REQUIRE(created.GetName() == "brandNew");
        REQUIRE_FALSE(root.GetChild("brandNew").IsNull());
    }

    SECTION("children can be removed one at a time or by name")
    {
        REQUIRE(root.RemoveChild(other));
        REQUIRE(root.GetChild("other").IsNull());

        REQUIRE(root.RemoveChildren("child"));
        REQUIRE(root.GetChild("child").IsNull());
    }

    SECTION("element text is read and written separately from attributes")
    {
        XMLElement text = root.CreateChild("text");
        REQUIRE(text.SetValue("some content"));
        REQUIRE(text.GetValue() == "some content");
    }

    SECTION("a null element is inert rather than a crash")
    {
        XMLElement null = XMLElement::EMPTY;
        REQUIRE(null.IsNull());
        REQUIRE_FALSE(static_cast<bool>(null));
        REQUIRE(null.GetName().Empty());
        REQUIRE(null.GetChild("anything").IsNull());
        REQUIRE(null.GetAttribute("anything").Empty());
        REQUIRE_FALSE(null.SetInt("anything", 1));
        REQUIRE(null.GetInt("anything") == 0);
    }
}

TEST_CASE("XMLFile parses, saves, and patches a document", "[Resource]")
{
    TestContext context;
    SharedPtr<XMLFile> document(new XMLFile(context));

    REQUIRE(document->FromString("<root><child index=\"7\">text</child></root>"));
    REQUIRE(document->GetRoot().GetName() == "root");
    REQUIRE(document->GetRoot("root").GetName() == "root");
    REQUIRE(document->GetRoot("wrong").IsNull());
    REQUIRE(document->GetRoot().GetChild("child").GetInt("index") == 7);

    SECTION("a malformed document is rejected")
    {
        SharedPtr<XMLFile> bad(new XMLFile(context));
        REQUIRE_FALSE(bad->FromString("<root><unclosed>"));
        REQUIRE_FALSE(bad->FromString(""));
    }

    SECTION("the document serialises back to text and reparses")
    {
        const String text = document->ToString();
        REQUIRE(text.Contains("index=\"7\""));

        SharedPtr<XMLFile> reparsed(new XMLFile(context));
        REQUIRE(reparsed->FromString(text));
        REQUIRE(reparsed->GetRoot().GetChild("child").GetInt("index") == 7);
    }

    SECTION("the document round trips through a binary stream")
    {
        VectorBuffer buffer;
        REQUIRE(document->Save(buffer));
        buffer.Seek(0);

        SharedPtr<XMLFile> loaded(new XMLFile(context));
        REQUIRE(loaded->Load(buffer));
        REQUIRE(loaded->GetRoot().GetChild("child").GetInt("index") == 7);
    }

    SECTION("a patch adds, replaces, and removes nodes")
    {
        SharedPtr<XMLFile> patch(new XMLFile(context));
        REQUIRE(patch->FromString(
            "<patch>"
            "<add sel=\"/root\"><added value=\"1\" /></add>"
            "<replace sel=\"/root/child/@index\">9</replace>"
            "</patch>"));

        document->Patch(patch);
        REQUIRE_FALSE(document->GetRoot().GetChild("added").IsNull());
        REQUIRE(document->GetRoot().GetChild("child").GetInt("index") == 9);

        SharedPtr<XMLFile> removal(new XMLFile(context));
        REQUIRE(removal->FromString("<patch><remove sel=\"/root/added\" /></patch>"));
        document->Patch(removal);
        REQUIRE(document->GetRoot().GetChild("added").IsNull());
    }
}

TEST_CASE("XMLElement selects nodes with an XPath query", "[Resource]")
{
    TestContext context;
    SharedPtr<XMLFile> document(new XMLFile(context));
    REQUIRE(document->FromString(
        "<root>"
        "<item name=\"a\" value=\"1\" />"
        "<item name=\"b\" value=\"2\" />"
        "<group><item name=\"c\" value=\"3\" /></group>"
        "</root>"));

    XMLElement root = document->GetRoot();

    XPathResultSet direct = root.Select("item");
    REQUIRE(direct.Size() == 2);
    REQUIRE(direct[0].GetAttribute("name") == "a");
    REQUIRE_FALSE(direct.Empty());

    XPathResultSet all = root.Select("//item");
    REQUIRE(all.Size() == 3);

    XMLElement single = root.SelectSingle("item[@name='b']");
    REQUIRE_FALSE(single.IsNull());
    REQUIRE(single.GetInt("value") == 2);

    REQUIRE(root.SelectSingle("item[@name='zzz']").IsNull());

    SECTION("a prepared query can be reused")
    {
        XPathQuery query("item[@value>1]");
        XPathResultSet matched = root.SelectPrepared(query);
        REQUIRE(matched.Size() == 1);
        REQUIRE(matched[0].GetAttribute("name") == "b");

        query.SetQuery("item");
        REQUIRE(root.SelectPrepared(query).Size() == 2);
    }

    SECTION("a result set can be walked from the front")
    {
        XPathResultSet results = root.Select("//item");
        XMLElement current = results.FirstResult();
        unsigned seen = 0;
        while (!current.IsNull())
        {
            ++seen;
            current = current.NextResult();
        }
        REQUIRE(seen == 3);
    }
}

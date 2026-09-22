#include "TestUtils.h"

#include <Urho3D/IO/VectorBuffer.h>
#include <Urho3D/Resource/JSONFile.h>
#include <Urho3D/Resource/JSONValue.h>
#include <Urho3D/Resource/Localization.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Resource/XMLElement.h>
#include <Urho3D/Resource/XMLFile.h>

using namespace Urho3D;
using namespace U3DTest;

TEST_CASE("JSONValue scalar types", "[Resource]")
{
    JSONValue value;
    REQUIRE(value.IsNull());
    REQUIRE(value.GetValueType() == JSON_NULL);

    value = 42;
    REQUIRE(value.IsNumber());
    REQUIRE(value.GetInt() == 42);

    value = 2.5;
    REQUIRE(value.GetDouble() == 2.5);
    REQUIRE_EQ_F(value.GetFloat(), 2.5f);

    value = true;
    REQUIRE(value.IsBool());
    REQUIRE(value.GetBool());

    value = "text";
    REQUIRE(value.IsString());
    REQUIRE(value.GetString() == "text");

    SECTION("reading as the wrong type returns a default")
    {
        const JSONValue text("abc");
        REQUIRE(text.GetInt() == 0);
        REQUIRE_FALSE(text.GetBool());

        const JSONValue number(7);
        REQUIRE(number.GetString().Empty());
    }
}

TEST_CASE("JSONValue objects", "[Resource]")
{
    JSONValue object;
    object.Set("name", "test");
    object.Set("count", 3);
    object.Set("enabled", true);

    REQUIRE(object.IsObject());
    REQUIRE(object.Size() == 3);
    REQUIRE(object.Contains("name"));
    REQUIRE_FALSE(object.Contains("missing"));

    REQUIRE(object.Get("name").GetString() == "test");
    REQUIRE(object.Get("count").GetInt() == 3);
    REQUIRE(object.Get("missing").IsNull());

    SECTION("Erase removes a member")
    {
        object.Erase("count");
        REQUIRE(object.Size() == 2);
        REQUIRE_FALSE(object.Contains("count"));
    }

    SECTION("nested objects round trip")
    {
        JSONValue inner;
        inner.Set("x", 1);
        object.Set("inner", inner);

        REQUIRE(object.Get("inner").IsObject());
        REQUIRE(object.Get("inner").Get("x").GetInt() == 1);
    }

    SECTION("Clear empties the object")
    {
        object.Clear();
        REQUIRE(object.Size() == 0);
    }
}

TEST_CASE("JSONValue arrays", "[Resource]")
{
    JSONValue array;
    array.Push(1);
    array.Push("two");
    array.Push(true);

    REQUIRE(array.IsArray());
    REQUIRE(array.Size() == 3);
    REQUIRE(array[0].GetInt() == 1);
    REQUIRE(array[1].GetString() == "two");
    REQUIRE(array[2].GetBool());

    array.Pop();
    REQUIRE(array.Size() == 2);

    SECTION("Insert places a value at an index")
    {
        array.Insert(0, 99);
        REQUIRE(array.Size() == 3);
        REQUIRE(array[0].GetInt() == 99);
    }

    SECTION("Erase removes a range")
    {
        array.Erase(0, 1);
        REQUIRE(array.Size() == 1);
        REQUIRE(array[0].GetString() == "two");
    }

    SECTION("Resize grows with nulls")
    {
        array.Resize(5);
        REQUIRE(array.Size() == 5);
        REQUIRE(array[4].IsNull());
    }
}

TEST_CASE("JSONValue variant round trip", "[Resource]")
{
    JSONValue value;

    value.SetVariant(Variant(42));
    REQUIRE(value.GetVariant().GetInt() == 42);

    value.SetVariant(Variant(Vector3::ONE));
    REQUIRE(value.GetVariant().GetVector3() == Vector3::ONE);

    SECTION("variant maps round trip")
    {
        VariantMap map;
        map["a"] = 1;
        map["b"] = "text";

        JSONValue mapValue;
        mapValue.SetVariantMap(map);

        const VariantMap restored = mapValue.GetVariantMap();
        REQUIRE(restored.Size() == 2);
        REQUIRE(restored[StringHash("a")]->GetInt() == 1);
    }
}

TEST_CASE("JSONFile parses and serialises", "[Resource]")
{
    Context* context = HeadlessContext();
    SharedPtr<JSONFile> file(new JSONFile(context));

    const String source = "{\"name\":\"value\",\"number\":7,\"list\":[1,2,3],\"nested\":{\"flag\":true}}";
    REQUIRE(file->FromString(source));

    const JSONValue& root = file->GetRoot();
    REQUIRE(root.IsObject());
    REQUIRE(root.Get("name").GetString() == "value");
    REQUIRE(root.Get("number").GetInt() == 7);
    REQUIRE(root.Get("list").Size() == 3);
    REQUIRE(root.Get("list")[1].GetInt() == 2);
    REQUIRE(root.Get("nested").Get("flag").GetBool());

    SECTION("serialising and reparsing preserves the document")
    {
        const String text = file->ToString();
        REQUIRE_FALSE(text.Empty());

        SharedPtr<JSONFile> reparsed(new JSONFile(context));
        REQUIRE(reparsed->FromString(text));
        REQUIRE(reparsed->GetRoot().Get("number").GetInt() == 7);
        REQUIRE(reparsed->GetRoot().Get("nested").Get("flag").GetBool());
    }

    SECTION("saving to a buffer and loading it back works")
    {
        VectorBuffer buffer;
        REQUIRE(file->Save(buffer));
        buffer.Seek(0);

        SharedPtr<JSONFile> loaded(new JSONFile(context));
        REQUIRE(loaded->Load(buffer));
        REQUIRE(loaded->GetRoot().Get("name").GetString() == "value");
    }

    SECTION("malformed JSON is rejected")
    {
        SharedPtr<JSONFile> bad(new JSONFile(context));
        REQUIRE_FALSE(bad->FromString("{not valid json"));
    }

    SECTION("an empty string is rejected")
    {
        SharedPtr<JSONFile> empty(new JSONFile(context));
        REQUIRE_FALSE(empty->FromString(""));
    }
}

TEST_CASE("XMLFile parses and serialises", "[Resource]")
{
    Context* context = HeadlessContext();
    SharedPtr<XMLFile> file(new XMLFile(context));

    const String source =
        "<root attr=\"value\" number=\"7\">"
        "<child name=\"first\"/>"
        "<child name=\"second\"/>"
        "</root>";

    REQUIRE(file->FromString(source));

    XMLElement root = file->GetRoot();
    REQUIRE_FALSE(root.IsNull());
    REQUIRE(root.GetName() == "root");
    REQUIRE(root.GetAttribute("attr") == "value");
    REQUIRE(root.GetInt("number") == 7);
    REQUIRE(root.HasAttribute("attr"));
    REQUIRE_FALSE(root.HasAttribute("missing"));

    XMLElement child = root.GetChild("child");
    REQUIRE_FALSE(child.IsNull());
    REQUIRE(child.GetAttribute("name") == "first");

    child = child.GetNext("child");
    REQUIRE_FALSE(child.IsNull());
    REQUIRE(child.GetAttribute("name") == "second");

    REQUIRE(child.GetNext("child").IsNull());

    SECTION("building a document from scratch round trips")
    {
        SharedPtr<XMLFile> built(new XMLFile(context));
        XMLElement created = built->CreateRoot("document");
        created.SetAttribute("kind", "test");
        created.SetInt("count", 3);
        created.SetVector3("position", Vector3::ONE);
        created.SetBool("flag", true);

        XMLElement item = created.CreateChild("item");
        item.SetAttribute("id", "1");

        const String text = built->ToString();
        REQUIRE(text.Contains("document"));

        SharedPtr<XMLFile> reparsed(new XMLFile(context));
        REQUIRE(reparsed->FromString(text));

        XMLElement reparsedRoot = reparsed->GetRoot();
        REQUIRE(reparsedRoot.GetAttribute("kind") == "test");
        REQUIRE(reparsedRoot.GetInt("count") == 3);
        REQUIRE(reparsedRoot.GetVector3("position") == Vector3::ONE);
        REQUIRE(reparsedRoot.GetBool("flag"));
        REQUIRE(reparsedRoot.GetChild("item").GetAttribute("id") == "1");
    }

    SECTION("GetRoot with a name filter checks the root's tag")
    {
        REQUIRE_FALSE(file->GetRoot("root").IsNull());
        REQUIRE(file->GetRoot("wrong").IsNull());
    }

    SECTION("malformed XML is rejected")
    {
        SharedPtr<XMLFile> bad(new XMLFile(context));
        REQUIRE_FALSE(bad->FromString("<unclosed>"));
    }

    SECTION("saving to a buffer and loading it back works")
    {
        VectorBuffer buffer;
        REQUIRE(file->Save(buffer));
        buffer.Seek(0);

        SharedPtr<XMLFile> loaded(new XMLFile(context));
        REQUIRE(loaded->Load(buffer));
        REQUIRE(loaded->GetRoot().GetAttribute("attr") == "value");
    }
}

TEST_CASE("XMLElement typed attribute accessors", "[Resource]")
{
    Context* context = HeadlessContext();
    SharedPtr<XMLFile> file(new XMLFile(context));
    XMLElement root = file->CreateRoot("root");

    root.SetFloat("f", 2.5f);
    root.SetUInt("u", 42u);
    root.SetColor("colour", Color::RED);
    root.SetQuaternion("rotation", Quaternion::IDENTITY);
    root.SetIntRect("rect", IntRect(1, 2, 3, 4));
    root.SetVector2("v2", Vector2(1.0f, 2.0f));
    root.SetVector4("v4", Vector4(1.0f, 2.0f, 3.0f, 4.0f));

    REQUIRE_EQ_F(root.GetFloat("f"), 2.5f);
    REQUIRE(root.GetUInt("u") == 42u);
    REQUIRE(root.GetColor("colour") == Color::RED);
    REQUIRE(root.GetQuaternion("rotation").Equals(Quaternion::IDENTITY));
    REQUIRE(root.GetIntRect("rect") == IntRect(1, 2, 3, 4));
    REQUIRE(root.GetVector2("v2") == Vector2(1.0f, 2.0f));
    REQUIRE(root.GetVector4("v4") == Vector4(1.0f, 2.0f, 3.0f, 4.0f));

    SECTION("a missing attribute reads back as a default")
    {
        REQUIRE(root.GetInt("absent") == 0);
        REQUIRE(root.GetAttribute("absent").Empty());
        REQUIRE(root.GetVector3("absent") == Vector3::ZERO);
    }

    SECTION("attribute names can be enumerated")
    {
        const Vector<String> names = root.GetAttributeNames();
        REQUIRE(names.Size() == 7);
        REQUIRE(names.Contains("f"));
    }
}

TEST_CASE("ResourceCache resolves real files from the source tree", "[Resource]")
{
    Context* context = HeadlessContext();
    auto* cache = context->GetSubsystem<ResourceCache>();
    REQUIRE(cache != nullptr);

    SECTION("an existing resource loads and is cached")
    {
        auto* first = cache->GetResource<XMLFile>("UI/DefaultStyle.xml");
        REQUIRE(first != nullptr);
        REQUIRE(cache->Exists("UI/DefaultStyle.xml"));

        auto* second = cache->GetResource<XMLFile>("UI/DefaultStyle.xml");
        REQUIRE(second == first);
    }

    SECTION("a missing resource returns null rather than throwing")
    {
        REQUIRE(cache->GetResource<XMLFile>("Does/Not/Exist.xml", false) == nullptr);
        REQUIRE_FALSE(cache->Exists("Does/Not/Exist.xml"));
    }

    SECTION("resource directories are registered")
    {
        REQUIRE_FALSE(cache->GetResourceDirs().Empty());
    }
}

TEST_CASE("Localization stores translations", "[Resource]")
{
    Context* context = HeadlessContext();
    SharedPtr<Localization> localization(new Localization(context));

    REQUIRE(localization->GetNumLanguages() == 0);
    REQUIRE(localization->GetLanguageIndex() == 0);
    REQUIRE(localization->GetLanguageIndex("nope") == -1);

    SharedPtr<JSONFile> file(new JSONFile(context));
    REQUIRE(file->FromString("{\"greeting\":{\"en\":\"Hello\",\"fr\":\"Bonjour\"}}"));
    localization->LoadMultipleLanguageJSON(file->GetRoot());

    REQUIRE(localization->GetNumLanguages() == 2);
    REQUIRE(localization->GetLanguageIndex("en") == 0);
    REQUIRE(localization->GetLanguageIndex("fr") == 1);

    localization->SetLanguage("en");
    REQUIRE(localization->GetLanguage() == "en");
    REQUIRE(localization->Get("greeting") == "Hello");

    localization->SetLanguage("fr");
    REQUIRE(localization->Get("greeting") == "Bonjour");

    SECTION("an unknown string returns the key itself")
    {
        REQUIRE(localization->Get("unknown") == "unknown");
    }

    SECTION("an empty key returns an empty string")
    {
        REQUIRE(localization->Get("").Empty());
    }
}

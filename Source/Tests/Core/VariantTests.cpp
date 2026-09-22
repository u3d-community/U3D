#include "TestUtils.h"

#include <Urho3D/Core/Variant.h>
#include <Urho3D/IO/VectorBuffer.h>

using namespace Urho3D;

TEST_CASE("Variant default is empty", "[Core]")
{
    const Variant v;
    REQUIRE(v.GetType() == VAR_NONE);
    REQUIRE(v.IsEmpty());
    REQUIRE(v == Variant::EMPTY);
    REQUIRE(v.GetTypeName() == "None");
}

TEST_CASE("Variant holds scalars", "[Core]")
{
    SECTION("int")
    {
        const Variant v(42);
        REQUIRE(v.GetType() == VAR_INT);
        REQUIRE(v.GetInt() == 42);
        REQUIRE(v.GetTypeName() == "Int");
        REQUIRE_FALSE(v.IsEmpty());
    }

    SECTION("unsigned")
    {
        const Variant v(42u);
        REQUIRE(v.GetType() == VAR_INT);
        REQUIRE(v.GetUInt() == 42u);
    }

    SECTION("bool")
    {
        REQUIRE(Variant(true).GetBool());
        REQUIRE_FALSE(Variant(false).GetBool());
        REQUIRE(Variant(true).GetType() == VAR_BOOL);
    }

    SECTION("float and double")
    {
        const Variant f(2.5f);
        REQUIRE(f.GetType() == VAR_FLOAT);
        REQUIRE_EQ_F(f.GetFloat(), 2.5f);

        const Variant d(2.5);
        REQUIRE(d.GetType() == VAR_DOUBLE);
        REQUIRE(d.GetDouble() == 2.5);
    }

    SECTION("64 bit integers")
    {
        const long long big = 1234567890123LL;
        const Variant v(big);
        REQUIRE(v.GetType() == VAR_INT64);
        REQUIRE(v.GetInt64() == big);
    }

    SECTION("string")
    {
        const Variant v("hello");
        REQUIRE(v.GetType() == VAR_STRING);
        REQUIRE(v.GetString() == "hello");
    }
}

TEST_CASE("Variant holds math types", "[Core]")
{
    REQUIRE(Variant(Vector2(1.0f, 2.0f)).GetVector2() == Vector2(1.0f, 2.0f));
    REQUIRE(Variant(Vector3(1.0f, 2.0f, 3.0f)).GetVector3() == Vector3(1.0f, 2.0f, 3.0f));
    REQUIRE(Variant(Vector4(1.0f, 2.0f, 3.0f, 4.0f)).GetVector4() == Vector4(1.0f, 2.0f, 3.0f, 4.0f));
    REQUIRE(Variant(IntVector2(1, 2)).GetIntVector2() == IntVector2(1, 2));
    REQUIRE(Variant(IntVector3(1, 2, 3)).GetIntVector3() == IntVector3(1, 2, 3));
    REQUIRE(Variant(Quaternion(1.0f, 0.0f, 0.0f, 0.0f)).GetQuaternion() == Quaternion::IDENTITY);
    REQUIRE(Variant(Color::RED).GetColor() == Color::RED);
    REQUIRE(Variant(Rect(1.0f, 2.0f, 3.0f, 4.0f)).GetRect() == Rect(1.0f, 2.0f, 3.0f, 4.0f));
    REQUIRE(Variant(IntRect(1, 2, 3, 4)).GetIntRect() == IntRect(1, 2, 3, 4));
    REQUIRE(Variant(Matrix3::IDENTITY).GetMatrix3() == Matrix3::IDENTITY);
    REQUIRE(Variant(Matrix3x4::IDENTITY).GetMatrix3x4() == Matrix3x4::IDENTITY);
    REQUIRE(Variant(Matrix4::IDENTITY).GetMatrix4() == Matrix4::IDENTITY);

    REQUIRE(Variant(Vector3::ONE).GetType() == VAR_VECTOR3);
    REQUIRE(Variant(Color::RED).GetTypeName() == "Color");
}

TEST_CASE("Variant type mismatch returns a default rather than throwing", "[Core]")
{
    const Variant v(42);
    REQUIRE(v.GetInt() == 42);
    REQUIRE_EQ_F(v.GetFloat(), 42.0f);
    REQUIRE(v.GetVector3() == Vector3::ZERO);
    REQUIRE(v.GetString().Empty());
    REQUIRE_FALSE(v.GetBool());
    REQUIRE_FALSE(Variant(1).GetBool());
    REQUIRE(Variant(true).GetBool());

    const Variant text("not a number");
    REQUIRE(text.GetInt() == 0);
    REQUIRE(text.GetVector3() == Vector3::ZERO);
}

TEST_CASE("Variant assignment and copy", "[Core]")
{
    Variant v;
    v = 42;
    REQUIRE(v.GetInt() == 42);

    v = "text";
    REQUIRE(v.GetType() == VAR_STRING);
    REQUIRE(v.GetString() == "text");

    v = Vector3::ONE;
    REQUIRE(v.GetType() == VAR_VECTOR3);

    const Variant copy(v);
    REQUIRE(copy == v);

    Variant assigned;
    assigned = v;
    REQUIRE(assigned == v);

    v.Clear();
    REQUIRE(v.IsEmpty());
    REQUIRE(v.GetType() == VAR_NONE);
}

TEST_CASE("Variant equality", "[Core]")
{
    REQUIRE(Variant(1) == Variant(1));
    REQUIRE(Variant(1) != Variant(2));
    REQUIRE(Variant(1) != Variant("1"));
    REQUIRE(Variant("a") == Variant("a"));
    REQUIRE(Variant(Vector3::ONE) == Variant(Vector3::ONE));
    REQUIRE(Variant(Vector3::ONE) != Variant(Vector3::ZERO));

    SECTION("comparison against a raw value of the same type")
    {
        REQUIRE(Variant(5) == 5);
        REQUIRE(Variant(2.5f) == 2.5f);
        REQUIRE(Variant("x").GetString() == "x");
    }
}

TEST_CASE("Variant string round trip", "[Core]")
{
    SECTION("FromString restores the value for each type")
    {
        Variant v;

        v.FromString(VAR_INT, "42");
        REQUIRE(v.GetInt() == 42);

        v.FromString(VAR_BOOL, "true");
        REQUIRE(v.GetBool());

        v.FromString(VAR_FLOAT, "2.5");
        REQUIRE_EQ_F(v.GetFloat(), 2.5f);

        v.FromString(VAR_VECTOR3, "1 2 3");
        REQUIRE(v.GetVector3() == Vector3(1.0f, 2.0f, 3.0f));

        v.FromString(VAR_COLOR, "1 0 0 1");
        REQUIRE(v.GetColor() == Color::RED);
    }

    SECTION("ToString matches the underlying type's own formatting")
    {
        REQUIRE(Variant(42).ToString() == "42");
        REQUIRE(Variant(Vector3(1.0f, 2.0f, 3.0f)).ToString() == "1 2 3");
        REQUIRE(Variant(true).ToString() == "true");
        REQUIRE(Variant("text").ToString() == "text");
    }

    SECTION("type names round trip through GetTypeFromName")
    {
        REQUIRE(Variant::GetTypeFromName("Int") == VAR_INT);
        REQUIRE(Variant::GetTypeFromName("Vector3") == VAR_VECTOR3);
        REQUIRE(Variant::GetTypeFromName("NotAType") == VAR_NONE);
        REQUIRE(Variant::GetTypeName(VAR_COLOR) == "Color");
    }
}

TEST_CASE("VariantMap stores keyed variants", "[Core]")
{
    VariantMap map;
    map["a"] = 1;
    map["b"] = Vector3::ONE;

    REQUIRE(map.Size() == 2);
    REQUIRE(map["a"].GetInt() == 1);
    REQUIRE(map["b"].GetVector3() == Vector3::ONE);

    const Variant wrapper(map);
    REQUIRE(wrapper.GetType() == VAR_VARIANTMAP);
    REQUIRE(wrapper.GetVariantMap().Size() == 2);

    SECTION("string hash keys and string keys agree")
    {
        REQUIRE(map[StringHash("a")].GetInt() == 1);
    }
}

TEST_CASE("VariantVector stores ordered variants", "[Core]")
{
    VariantVector vector;
    vector.Push(Variant(1));
    vector.Push(Variant("two"));
    vector.Push(Variant(Vector3::ONE));

    const Variant wrapper(vector);
    REQUIRE(wrapper.GetType() == VAR_VARIANTVECTOR);
    REQUIRE(wrapper.GetVariantVector().Size() == 3);
    REQUIRE(wrapper.GetVariantVector()[1].GetString() == "two");
}

TEST_CASE("Variant holds a string vector", "[Core]")
{
    StringVector strings;
    strings.Push("one");
    strings.Push("two");

    const Variant v(strings);
    REQUIRE(v.GetType() == VAR_STRINGVECTOR);
    REQUIRE(v.GetStringVector().Size() == 2);
    REQUIRE(v.GetStringVector()[0] == "one");
}

TEST_CASE("Variant holds a raw byte buffer", "[Core]")
{
    PODVector<unsigned char> buffer;
    for (unsigned char i = 0; i < 10; ++i)
        buffer.Push(i);

    const Variant v(buffer);
    REQUIRE(v.GetType() == VAR_BUFFER);
    REQUIRE(v.GetBuffer().Size() == 10);
    REQUIRE(v.GetBuffer()[5] == 5);

    SECTION("the buffer can be read back as a VectorBuffer")
    {
        VectorBuffer reader = v.GetVectorBuffer();
        REQUIRE(reader.GetSize() == 10);
    }
}

TEST_CASE("Variant holds resource references", "[Core]")
{
    ResourceRef ref(StringHash("Model"), "Models/Box.mdl");
    const Variant v(ref);

    REQUIRE(v.GetType() == VAR_RESOURCEREF);
    REQUIRE(v.GetResourceRef().name_ == "Models/Box.mdl");
    REQUIRE(v.GetResourceRef().type_ == StringHash("Model"));

    ResourceRefList list(StringHash("Material"));
    list.names_.Push("Materials/Stone.xml");
    list.names_.Push("Materials/Grass.xml");

    const Variant listVariant(list);
    REQUIRE(listVariant.GetType() == VAR_RESOURCEREFLIST);
    REQUIRE(listVariant.GetResourceRefList().names_.Size() == 2);

    SECTION("a resource ref list parses from a semicolon separated string")
    {
        Variant parsed;
        parsed.FromString(VAR_RESOURCEREFLIST, "Material;Materials/Stone.xml;Materials/Grass.xml");
        REQUIRE(parsed.GetType() == VAR_RESOURCEREFLIST);
        REQUIRE(parsed.GetResourceRefList().type_ == StringHash("Material"));
        REQUIRE(parsed.GetResourceRefList().names_.Size() == 2);
        REQUIRE(parsed.GetResourceRefList().names_[0] == "Materials/Stone.xml");
        REQUIRE(parsed.GetResourceRefList().names_[1] == "Materials/Grass.xml");
    }

    SECTION("a type with no names still yields an empty but typed list")
    {
        Variant parsed;
        parsed.FromString(VAR_RESOURCEREFLIST, "Material");
        REQUIRE(parsed.GetType() == VAR_RESOURCEREFLIST);
        REQUIRE(parsed.GetResourceRefList().type_ == StringHash("Material"));
        REQUIRE(parsed.GetResourceRefList().names_.Empty());
    }

    SECTION("a resource ref parses from a semicolon separated string")
    {
        Variant parsed;
        parsed.FromString(VAR_RESOURCEREF, "Model;Models/Box.mdl");
        REQUIRE(parsed.GetResourceRef().type_ == StringHash("Model"));
        REQUIRE(parsed.GetResourceRef().name_ == "Models/Box.mdl");
    }

    SECTION("resource refs compare by type and name")
    {
        REQUIRE(ResourceRef(StringHash("Model"), "a") == ResourceRef(StringHash("Model"), "a"));
        REQUIRE(ResourceRef(StringHash("Model"), "a") != ResourceRef(StringHash("Model"), "b"));
    }
}

TEST_CASE("Variant holds a raw void pointer", "[Core]")
{
    int target = 5;
    const Variant v(&target);

    REQUIRE(v.GetType() == VAR_VOIDPTR);
    REQUIRE(v.GetVoidPtr() == &target);

    REQUIRE(Variant().GetVoidPtr() == nullptr);
}

TEST_CASE("Variant stores a StringHash as its underlying integer", "[Core]")
{
    const Variant v(StringHash("Node").Value());
    REQUIRE(v.GetType() == VAR_INT);
    REQUIRE(v.GetStringHash() == StringHash("Node"));
    REQUIRE(Variant(0u).GetStringHash() == StringHash::ZERO);
}

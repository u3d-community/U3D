#include "TestUtils.h"

#include <Urho3D/Container/RefCounted.h>
#include <Urho3D/Core/Variant.h>
#include <Urho3D/IO/VectorBuffer.h>

using namespace Urho3D;

namespace
{

struct Payload
{
    int a_;
    int b_;

    bool operator ==(const Payload& rhs) const { return a_ == rhs.a_ && b_ == rhs.b_; }
};

struct BigPayload
{
    double values_[16];

    bool operator ==(const BigPayload& rhs) const { return values_[0] == rhs.values_[0]; }
};

}

namespace Urho3D
{

template <> struct CustomVariantValueTraits<Payload>
{
    static bool Compare(const Payload& lhs, const Payload& rhs) { return lhs == rhs; }
    static bool IsZero(const Payload& value) { return value.a_ == 0 && value.b_ == 0; }
    static String ToString(const Payload& value) { return Urho3D::ToString("%d/%d", value.a_, value.b_); }
};

}

namespace
{

VariantVector EveryType()
{
    VariantMap map;
    map["key"] = 1;

    VariantVector inner;
    inner.Push(Variant(1));

    StringVector strings;
    strings.Push("one");

    PODVector<unsigned char> buffer;
    buffer.Push(1);
    buffer.Push(2);

    ResourceRefList refList(StringHash("Material"));
    refList.names_.Push("a.xml");

    VariantVector all;
    all.Push(Variant());
    all.Push(Variant(42));
    all.Push(Variant(1234567890123LL));
    all.Push(Variant(true));
    all.Push(Variant(2.5f));
    all.Push(Variant(2.5));
    all.Push(Variant(Vector2(1.0f, 2.0f)));
    all.Push(Variant(Vector3(1.0f, 2.0f, 3.0f)));
    all.Push(Variant(Vector4(1.0f, 2.0f, 3.0f, 4.0f)));
    all.Push(Variant(Quaternion(1.0f, 0.0f, 0.0f, 0.0f)));
    all.Push(Variant(Color::RED));
    all.Push(Variant("text"));
    all.Push(Variant(buffer));
    all.Push(Variant(ResourceRef(StringHash("Model"), "Models/Box.mdl")));
    all.Push(Variant(refList));
    all.Push(Variant(inner));
    all.Push(Variant(map));
    all.Push(Variant(IntRect(1, 2, 3, 4)));
    all.Push(Variant(IntVector2(1, 2)));
    all.Push(Variant(IntVector3(1, 2, 3)));
    all.Push(Variant(Matrix3::IDENTITY));
    all.Push(Variant(Matrix3x4::IDENTITY));
    all.Push(Variant(Matrix4::IDENTITY));
    all.Push(Variant(strings));
    all.Push(Variant(Rect(1.0f, 2.0f, 3.0f, 4.0f)));
    return all;
}

}

TEST_CASE("Variant copy construction preserves every type", "[Core]")
{
    const VariantVector all = EveryType();

    for (unsigned i = 0; i < all.Size(); ++i)
    {
        const Variant copy(all[i]);
        REQUIRE(copy.GetType() == all[i].GetType());
        REQUIRE(copy == all[i]);
    }
}

TEST_CASE("Variant assignment preserves every type", "[Core]")
{
    const VariantVector all = EveryType();

    for (unsigned i = 0; i < all.Size(); ++i)
    {
        Variant target;
        target = all[i];
        REQUIRE(target.GetType() == all[i].GetType());
        REQUIRE(target == all[i]);
    }

    SECTION("assigning over a populated variant releases the old type")
    {
        for (unsigned i = 0; i < all.Size(); ++i)
        {
            Variant target(Matrix4::IDENTITY);
            target = all[i];
            REQUIRE(target == all[i]);

            Variant other(all[i]);
            other = Variant(Matrix4::IDENTITY);
            REQUIRE(other.GetType() == VAR_MATRIX4);
        }
    }
}

TEST_CASE("Variant equality distinguishes every type", "[Core]")
{
    const VariantVector all = EveryType();

    for (unsigned i = 0; i < all.Size(); ++i)
    {
        REQUIRE(all[i] == all[i]);

        for (unsigned j = 0; j < all.Size(); ++j)
        {
            if (i != j)
                REQUIRE(all[i] != all[j]);
        }
    }
}

TEST_CASE("Variant ToString and FromString round trip", "[Core]")
{
    struct Case
    {
        VariantType type_;
        Variant value_;
    };

    const Case cases[] = {
        {VAR_INT, Variant(42)},
        {VAR_INT64, Variant(1234567890123LL)},
        {VAR_BOOL, Variant(true)},
        {VAR_FLOAT, Variant(2.5f)},
        {VAR_DOUBLE, Variant(2.5)},
        {VAR_VECTOR2, Variant(Vector2(1.0f, 2.0f))},
        {VAR_VECTOR3, Variant(Vector3(1.0f, 2.0f, 3.0f))},
        {VAR_VECTOR4, Variant(Vector4(1.0f, 2.0f, 3.0f, 4.0f))},
        {VAR_QUATERNION, Variant(Quaternion(1.0f, 0.0f, 0.0f, 0.0f))},
        {VAR_COLOR, Variant(Color::RED)},
        {VAR_STRING, Variant("text")},
        {VAR_INTRECT, Variant(IntRect(1, 2, 3, 4))},
        {VAR_INTVECTOR2, Variant(IntVector2(1, 2))},
        {VAR_INTVECTOR3, Variant(IntVector3(1, 2, 3))},
        {VAR_MATRIX3, Variant(Matrix3::IDENTITY)},
        {VAR_MATRIX3X4, Variant(Matrix3x4::IDENTITY)},
        {VAR_MATRIX4, Variant(Matrix4::IDENTITY)},
        {VAR_RECT, Variant(Rect(1.0f, 2.0f, 3.0f, 4.0f))},
    };

    for (const Case& item : cases)
    {
        const String text = item.value_.ToString();
        REQUIRE_FALSE(text.Empty());

        Variant restored;
        restored.FromString(item.type_, text);
        REQUIRE(restored.GetType() == item.type_);
        REQUIRE(restored == item.value_);

        Variant byName;
        byName.FromString(Variant::GetTypeName(item.type_), text);
        REQUIRE(byName == item.value_);
    }
}

TEST_CASE("Variant ToString for the aggregate types", "[Core]")
{
    PODVector<unsigned char> buffer;
    buffer.Push(1);
    buffer.Push(255);

    const Variant bufferVariant(buffer);
    const String bufferText = bufferVariant.ToString();
    REQUIRE_FALSE(bufferText.Empty());

    Variant restoredBuffer;
    restoredBuffer.FromString(VAR_BUFFER, bufferText);
    REQUIRE(restoredBuffer.GetBuffer() == buffer);

    SECTION("resource references parse from their textual form")
    {
        Variant restored;
        restored.FromString(VAR_RESOURCEREF, "Model;Models/Box.mdl");
        REQUIRE(restored.GetResourceRef().type_ == StringHash("Model"));
        REQUIRE(restored.GetResourceRef().name_ == "Models/Box.mdl");

        Variant restoredList;
        restoredList.FromString(VAR_RESOURCEREFLIST, "Material;a.xml;b.xml");
        REQUIRE(restoredList.GetResourceRefList().type_ == StringHash("Material"));
        REQUIRE(restoredList.GetResourceRefList().names_.Size() == 2);

        REQUIRE(Variant(ResourceRef(StringHash("Model"), "Models/Box.mdl")).ToString().Empty());
        REQUIRE(Variant(ResourceRefList(StringHash("Material"))).ToString().Empty());
    }

    SECTION("the container types have no textual form")
    {
        VariantVector vector;
        vector.Push(Variant(1));
        REQUIRE(Variant(vector).ToString().Empty());

        VariantMap map;
        map["a"] = 1;
        REQUIRE(Variant(map).ToString().Empty());

        StringVector strings;
        strings.Push("one");
        REQUIRE(Variant(strings).ToString().Empty());
    }

    SECTION("an empty variant stringifies to nothing")
    {
        REQUIRE(Variant::EMPTY.ToString().Empty());
    }

    SECTION("a void pointer stringifies as a placeholder")
    {
        int target = 0;
        REQUIRE_FALSE(Variant(&target).ToString().Empty());
    }
}

TEST_CASE("Variant assignment from a VectorBuffer", "[Core]")
{
    VectorBuffer source;
    source.WriteInt(7);
    source.WriteString("payload");

    Variant variant;
    variant = source;

    REQUIRE(variant.GetType() == VAR_BUFFER);
    REQUIRE(variant.GetBuffer() == source.GetBuffer());

    VectorBuffer restored = variant.GetVectorBuffer();
    REQUIRE(restored.ReadInt() == 7);
    REQUIRE(restored.ReadString() == "payload");
}

TEST_CASE("Variant holds a weak RefCounted pointer", "[Core]")
{
    class Target : public RefCounted
    {
    };

    SharedPtr<Target> target(new Target());

    Variant variant(target.Get());
    REQUIRE(variant.GetType() == VAR_PTR);
    REQUIRE(variant.GetPtr() == target.Get());

    const Variant copy(variant);
    REQUIRE(copy.GetPtr() == target.Get());
    REQUIRE(copy == variant);

    Variant assigned;
    assigned = variant;
    REQUIRE(assigned.GetPtr() == target.Get());

    SECTION("the pointer reads back as null once the target is gone")
    {
        target.Reset();
        REQUIRE(variant.GetPtr() == nullptr);
    }

    SECTION("two variants pointing at different objects differ")
    {
        SharedPtr<Target> other(new Target());
        REQUIRE(Variant(other.Get()) != variant);
    }
}

TEST_CASE("Variant carries a custom value", "[Core]")
{
    Variant variant;
    variant.SetCustom(Payload{1, 2});

    REQUIRE(variant.GetType() == VAR_CUSTOM_STACK);
    REQUIRE(variant.GetCustomPtr<Payload>() != nullptr);
    REQUIRE(variant.GetCustomPtr<Payload>()->a_ == 1);
    REQUIRE(variant.GetCustom<Payload>() == Payload{1, 2});

    SECTION("custom values never compare equal to built-in values")
    {
        REQUIRE(variant != Variant(0));
        REQUIRE(Variant(0) != variant);
    }

    SECTION("reading it as the wrong type yields null")
    {
        REQUIRE(variant.GetCustomPtr<int>() == nullptr);
    }

    SECTION("copies compare equal and are independent")
    {
        Variant copy(variant);
        REQUIRE(copy.GetCustomPtr<Payload>() != nullptr);
        REQUIRE(copy == variant);
        REQUIRE(copy.GetCustomPtr<Payload>() != variant.GetCustomPtr<Payload>());
    }

    SECTION("assignment carries the custom value across")
    {
        Variant assigned;
        assigned = variant;
        REQUIRE(assigned.GetCustom<Payload>() == Payload{1, 2});
    }

    SECTION("a different payload compares unequal")
    {
        Variant other;
        other.SetCustom(Payload{3, 4});
        REQUIRE(other != variant);
    }

    SECTION("clearing releases the custom value")
    {
        variant.Clear();
        REQUIRE(variant.GetType() == VAR_NONE);
        REQUIRE(variant.GetCustomPtr<Payload>() == nullptr);
    }

    SECTION("a payload too large for the inline buffer goes on the heap")
    {
        BigPayload big{};
        big.values_[0] = 3.5;

        Variant heap;
        heap.SetCustom(big);
        REQUIRE(heap.GetType() == VAR_CUSTOM_HEAP);
        REQUIRE(heap.GetCustomPtr<BigPayload>() != nullptr);
        REQUIRE(heap.GetCustom<BigPayload>().values_[0] == 3.5);

        Variant heapCopy(heap);
        REQUIRE(heapCopy != heap);
        REQUIRE(heapCopy.GetCustom<BigPayload>().values_[0] == 3.5);
        REQUIRE(heapCopy.GetCustomPtr<BigPayload>() != heap.GetCustomPtr<BigPayload>());

        heap.Clear();
        REQUIRE(heap.GetType() == VAR_NONE);
    }
}

TEST_CASE("Variant numeric conversions", "[Core]")
{
    REQUIRE_EQ_F(Variant(42).GetFloat(), 42.0f);
    REQUIRE(Variant(42).GetDouble() == 42.0);
    REQUIRE(Variant(42).GetInt64() == 42);

    REQUIRE(Variant(2.5f).GetInt() == 2);
    REQUIRE(Variant(2.5f).GetDouble() == 2.5);
    REQUIRE(Variant(2.5f).GetInt64() == 2);

    REQUIRE(Variant(2.5).GetInt() == 2);
    REQUIRE_EQ_F(Variant(2.5).GetFloat(), 2.5f);
    REQUIRE(Variant(2.5).GetInt64() == 2);

    REQUIRE(Variant(1234567890123LL).GetInt() == 0);
    REQUIRE(Variant(1234567890123LL).GetDouble() > 0.0);
    REQUIRE_EQ_F(Variant(1234567890123LL).GetFloat(), 1234567890123.0f);

    SECTION("a bool converts to its numeric form")
    {
        REQUIRE(Variant(true).GetInt() == 0);
        REQUIRE(Variant(true).GetBool());
    }

    SECTION("a non numeric type reads back as zero")
    {
        REQUIRE(Variant("text").GetFloat() == 0.0f);
        REQUIRE(Variant("text").GetDouble() == 0.0);
        REQUIRE(Variant("text").GetInt64() == 0);
    }
}

TEST_CASE("Variant SetType releases the previous payload", "[Core]")
{
    const VariantVector all = EveryType();

    Variant variant;
    for (unsigned i = 0; i < all.Size(); ++i)
    {
        variant = all[i];
        REQUIRE(variant.GetType() == all[i].GetType());
    }

    variant.Clear();
    REQUIRE(variant.IsEmpty());
}

#include "TestUtils.h"

#include <Urho3D/Core/StringUtils.h>
#include <Urho3D/Core/Variant.h>

using namespace Urho3D;

TEST_CASE("ToBool recognises words and digits", "[Core]")
{
    REQUIRE(ToBool("true"));
    REQUIRE(ToBool("TRUE"));
    REQUIRE(ToBool("True"));
    REQUIRE(ToBool("1"));

    REQUIRE_FALSE(ToBool("false"));
    REQUIRE_FALSE(ToBool("FALSE"));
    REQUIRE_FALSE(ToBool("0"));
    REQUIRE_FALSE(ToBool(""));
    REQUIRE_FALSE(ToBool("nonsense"));
}

TEST_CASE("ToInt and ToUInt parse integers", "[Core]")
{
    REQUIRE(ToInt("42") == 42);
    REQUIRE(ToInt("-42") == -42);
    REQUIRE(ToInt("") == 0);
    REQUIRE(ToInt("abc") == 0);
    REQUIRE(ToInt("42abc") == 42);

    REQUIRE(ToUInt("42") == 42u);
    REQUIRE(ToUInt("") == 0u);

    SECTION("an explicit base is honoured")
    {
        REQUIRE(ToInt("ff", 16) == 255);
        REQUIRE(ToInt("10", 2) == 2);
        REQUIRE(ToUInt("ff", 16) == 255u);
    }

    SECTION("base zero auto detects the prefix")
    {
        REQUIRE(ToInt("0xff", 0) == 255);
        REQUIRE(ToInt("10", 0) == 10);
    }

    SECTION("an out of range base falls back to autodetect rather than reaching strtol")
    {
        REQUIRE(ToInt("0xff", 99) == 255);
        REQUIRE(ToInt("0xff", 1) == 255);
        REQUIRE(ToInt("0xff", -5) == 255);
        REQUIRE(ToInt("10", 37) == 10);
        REQUIRE(ToInt64("0xff", 99) == 255);
        REQUIRE(ToInt64("ff", 16) == 255);
        REQUIRE(ToInt64("10", 2) == 2);
        REQUIRE(ToUInt64("ff", 16) == 255u);
        REQUIRE(ToUInt("0xff", 99) == 255u);
    }

    SECTION("64 bit parsing keeps the full range")
    {
        REQUIRE(ToInt64("9007199254740993") == 9007199254740993LL);
        REQUIRE(ToInt64("") == 0);
        REQUIRE(ToUInt64("18446744073709551615") == 18446744073709551615ULL);
    }
}

TEST_CASE("ToFloat and ToDouble parse reals", "[Core]")
{
    REQUIRE_EQ_F(ToFloat("2.5"), 2.5f);
    REQUIRE_EQ_F(ToFloat("-2.5"), -2.5f);
    REQUIRE_EQ_F(ToFloat(""), 0.0f);
    REQUIRE_EQ_F(ToFloat("abc"), 0.0f);

    REQUIRE(ToDouble("2.5") == 2.5);
    REQUIRE(ToDouble("") == 0.0);
}

TEST_CASE("ToVector parsers handle missing components", "[Core]")
{
    REQUIRE(ToVector2("1 2") == Vector2(1.0f, 2.0f));
    REQUIRE(ToVector3("1 2 3") == Vector3(1.0f, 2.0f, 3.0f));
    REQUIRE(ToVector4("1 2 3 4") == Vector4(1.0f, 2.0f, 3.0f, 4.0f));
    REQUIRE(ToIntVector2("1 2") == IntVector2(1, 2));
    REQUIRE(ToIntVector3("1 2 3") == IntVector3(1, 2, 3));

    SECTION("too few components yields a zero vector rather than a partial one")
    {
        REQUIRE(ToVector3("1 2") == Vector3::ZERO);
        REQUIRE(ToVector3("") == Vector3::ZERO);
        REQUIRE(ToVector4("1") == Vector4::ZERO);
        REQUIRE(ToVector2("1") == Vector2::ZERO);
    }

    SECTION("extra components beyond the expected count are ignored")
    {
        REQUIRE(ToVector3("1 2 3 4 5") == Vector3(1.0f, 2.0f, 3.0f));
    }

    SECTION("extra whitespace is tolerated")
    {
        REQUIRE(ToVector3("  1   2   3  ") == Vector3(1.0f, 2.0f, 3.0f));
    }
}

TEST_CASE("ToColor parses three and four component strings", "[Core]")
{
    REQUIRE(ToColor("1 0 0 1") == Color::RED);
    REQUIRE(ToColor("1 0 0") == Color(1.0f, 0.0f, 0.0f, 1.0f));
    REQUIRE(ToColor("") == Color::WHITE);
}

TEST_CASE("ToRect and ToIntRect", "[Core]")
{
    REQUIRE(ToRect("1 2 3 4") == Rect(1.0f, 2.0f, 3.0f, 4.0f));
    REQUIRE(ToIntRect("1 2 3 4") == IntRect(1, 2, 3, 4));
    REQUIRE(ToIntRect("") == IntRect::ZERO);
}

TEST_CASE("ToQuaternion accepts Euler angles and raw components", "[Core]")
{
    const Quaternion fromComponents = ToQuaternion("1 0 0 0");
    REQUIRE(fromComponents.Equals(Quaternion::IDENTITY));

    const Quaternion fromEuler = ToQuaternion("0 90 0");
    REQUIRE(fromEuler.Equals(Quaternion(0.0f, 90.0f, 0.0f)));

    REQUIRE(ToQuaternion("").Equals(Quaternion::IDENTITY));
}

TEST_CASE("ToMatrix parsers", "[Core]")
{
    REQUIRE(ToMatrix3("1 0 0 0 1 0 0 0 1").Equals(Matrix3::IDENTITY));
    REQUIRE(ToMatrix3x4("1 0 0 0 0 1 0 0 0 0 1 0").Equals(Matrix3x4::IDENTITY));
    REQUIRE(ToMatrix4("1 0 0 0 0 1 0 0 0 0 1 0 0 0 0 1").Equals(Matrix4::IDENTITY));
}

TEST_CASE("ToString formats a printf style string", "[Core]")
{
    REQUIRE(ToString("%d", 42) == "42");
    REQUIRE(ToString("%s-%d", "x", 7) == "x-7");
}

TEST_CASE("ToStringHex formats a fixed width hex value", "[Core]")
{
    REQUIRE(ToStringHex(255) == "000000ff");
    REQUIRE(ToStringHex(0) == "00000000");
}

TEST_CASE("IsDigit and IsAlpha", "[Core]")
{
    REQUIRE(IsDigit('0'));
    REQUIRE(IsDigit('9'));
    REQUIRE_FALSE(IsDigit('a'));

    REQUIRE(IsAlpha('a'));
    REQUIRE(IsAlpha('Z'));
    REQUIRE_FALSE(IsAlpha('0'));
}

TEST_CASE("ToUpper and ToLower on characters", "[Core]")
{
    REQUIRE(ToUpper('a') == 'A');
    REQUIRE(ToUpper('A') == 'A');
    REQUIRE(ToLower('A') == 'a');
    REQUIRE(ToLower('a') == 'a');
    REQUIRE(ToLower('1') == '1');
}

TEST_CASE("GetStringListIndex finds a value in a list", "[Core]")
{
    const char* list[] = {"alpha", "bravo", "charlie", nullptr};

    REQUIRE(GetStringListIndex("alpha", list, 99) == 0);
    REQUIRE(GetStringListIndex("charlie", list, 99) == 2);
    REQUIRE(GetStringListIndex("missing", list, 99) == 99);

    SECTION("the search is case insensitive by default")
    {
        REQUIRE(GetStringListIndex("ALPHA", list, 99) == 0);
        REQUIRE(GetStringListIndex("ALPHA", list, 99, true) == 99);
    }
}

TEST_CASE("GetFileSizeString scales the unit", "[Core]")
{
    REQUIRE(GetFileSizeString(512) == "512 b");
    REQUIRE(GetFileSizeString(2048) == "2.0 k");
    REQUIRE(GetFileSizeString(5ull * 1024 * 1024) == "5.0 M");
    REQUIRE(GetFileSizeString(3ull * 1024 * 1024 * 1024) == "3.0 G");
}

TEST_CASE("Buffer and string conversion round trip", "[Core]")
{
    PODVector<unsigned char> buffer;
    for (unsigned char i = 0; i < 8; ++i)
        buffer.Push(static_cast<unsigned char>(i * 16));

    String text;
    BufferToString(text, buffer.Buffer(), buffer.Size());
    REQUIRE_FALSE(text.Empty());

    PODVector<unsigned char> restored;
    StringToBuffer(restored, text);

    REQUIRE(restored.Size() == buffer.Size());
    for (unsigned i = 0; i < buffer.Size(); ++i)
        REQUIRE(restored[i] == buffer[i]);

    SECTION("an empty buffer round trips to an empty buffer")
    {
        String empty;
        BufferToString(empty, nullptr, 0);
        PODVector<unsigned char> none;
        StringToBuffer(none, empty);
        REQUIRE(none.Empty());
    }
}

TEST_CASE("ToVectorVariant picks a type from the component count", "[Core]")
{
    REQUIRE(ToVectorVariant("1").GetType() == VAR_FLOAT);
    REQUIRE(ToVectorVariant("1 2").GetType() == VAR_VECTOR2);
    REQUIRE(ToVectorVariant("1 2 3").GetType() == VAR_VECTOR3);
    REQUIRE(ToVectorVariant("1 2 3 4").GetType() == VAR_VECTOR4);
    REQUIRE(ToVectorVariant("").GetType() == VAR_NONE);

    REQUIRE(ToVectorVariant("1 2 3").GetVector3() == Vector3(1.0f, 2.0f, 3.0f));
}

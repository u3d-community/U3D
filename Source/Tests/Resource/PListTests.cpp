#include "TestUtils.h"

#include <Urho3D/IO/VectorBuffer.h>
#include <Urho3D/Resource/PListFile.h>

using namespace Urho3D;
using namespace U3DTest;

TEST_CASE("PListValue owns and converts values", "[Resource]")
{
    PListValue value;
    REQUIRE_FALSE(static_cast<bool>(value));
    REQUIRE(value.GetInt() == 0);
    REQUIRE(value.GetString().Empty());
    REQUIRE(value.GetValueMap().Empty());

    value.SetInt(12);
    REQUIRE(value.GetType() == PLVT_INT);
    REQUIRE(value.GetInt() == 12);
    value.SetBool(true);
    REQUIRE(value.GetType() == PLVT_BOOL);
    REQUIRE(value.GetBool());
    value.SetFloat(1.25f);
    REQUIRE_EQ_F(value.GetFloat(), 1.25f);
    value.SetString("hello");
    REQUIRE(value.GetString() == "hello");

    SECTION("Apple geometry strings are converted")
    {
        value.SetString("{{1,2},{30,40}}");
        REQUIRE(value.GetIntRect() == IntRect(1, 2, 31, 42));
        value.SetString("{3,4}");
        REQUIRE(value.GetIntVector2() == IntVector2(3, 4));
        value.SetString("{5,6,7}");
        REQUIRE(value.GetIntVector3() == IntVector3(5, 6, 7));
    }

    SECTION("copying compound values does not alias storage")
    {
        PListValueMap& map = value.ConvertToValueMap();
        map["number"].SetInt(5);
        PListValue copy(value);
        map["number"].SetInt(9);
        REQUIRE(copy.GetValueMap()["number"]->GetInt() == 5);

        PListValue assigned;
        assigned = copy;
        REQUIRE(assigned.GetValueMap()["number"]->GetInt() == 5);
    }

    SECTION("vectors can be built after holding another type")
    {
        PListValueVector& vector = value.ConvertToValueVector();
        vector.Push(PListValue(1));
        vector.Push(PListValue(String("two")));
        REQUIRE(value.GetType() == PLVT_VALUEVECTOR);
        REQUIRE(value.GetValueVector().Size() == 2);
        REQUIRE(value.GetValueVector()[1].GetString() == "two");
    }
}

TEST_CASE("PListFile parses nested property lists", "[Resource]")
{
    TestContext context;
    PListFile file(context);
    const String xml =
        "<?xml version=\"1.0\"?><plist version=\"1.0\"><dict>"
        "<key>name</key><string>sprite</string>"
        "<key>count</key><integer>3</integer>"
        "<key>ratio</key><real>1.5</real>"
        "<key>visible</key><true/>"
        "<key>items</key><array><string>a</string><false/><integer>8</integer></array>"
        "<key>nested</key><dict><key>point</key><string>{2,4}</string></dict>"
        "</dict></plist>";
    VectorBuffer source(xml.CString(), xml.Length());

    REQUIRE(file.Load(source));
    const PListValueMap& root = file.GetRoot();
    REQUIRE(root.Size() == 6);
    REQUIRE(root["name"]->GetString() == "sprite");
    REQUIRE(root["count"]->GetInt() == 3);
    REQUIRE_EQ_F(root["ratio"]->GetFloat(), 1.5f);
    REQUIRE(root["visible"]->GetBool());
    REQUIRE(root["items"]->GetValueVector().Size() == 3);
    REQUIRE_FALSE(root["items"]->GetValueVector()[1].GetBool());
    REQUIRE(root["nested"]->GetValueMap()["point"]->GetIntVector2() == IntVector2(2, 4));

    SECTION("invalid roots and unsupported values fail cleanly")
    {
        PListFile invalid(context);
        VectorBuffer badRoot("<root/>", 7);
        REQUIRE_FALSE(invalid.Load(badRoot));

        const char* unsupported = "<plist><dict><key>x</key><date>today</date></dict></plist>";
        VectorBuffer badValue(unsupported, String(unsupported).Length());
        REQUIRE_FALSE(invalid.Load(badValue));
    }
}

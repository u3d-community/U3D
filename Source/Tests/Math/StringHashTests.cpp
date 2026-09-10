#include "TestUtils.h"

#include <Urho3D/Core/StringHashRegister.h>
#include <Urho3D/Math/StringHash.h>

using namespace Urho3D;

TEST_CASE("StringHash construction", "[Math]")
{
    REQUIRE(StringHash().Value() == 0);
    REQUIRE(StringHash::ZERO.Value() == 0);
    REQUIRE(StringHash(1234u).Value() == 1234u);

    const StringHash fromLiteral("Node");
    const StringHash fromString(String("Node"));
    REQUIRE(fromLiteral == fromString);

    StringHash copy(fromLiteral);
    REQUIRE(copy == fromLiteral);

    StringHash assigned;
    assigned = fromLiteral;
    REQUIRE(assigned == fromLiteral);
}

TEST_CASE("StringHash is deterministic and distinguishes inputs", "[Math]")
{
    REQUIRE(StringHash("Position") == StringHash("Position"));
    REQUIRE(StringHash("Position") != StringHash("Rotation"));
    REQUIRE(StringHash("a") != StringHash("b"));
    REQUIRE(StringHash("ab") != StringHash("ba"));

    SECTION("an empty string hashes to zero")
    {
        REQUIRE(StringHash("") == StringHash::ZERO);
        REQUIRE_FALSE(static_cast<bool>(StringHash("")));
    }

    SECTION("hashing is case sensitive")
    {
        REQUIRE(StringHash("Node") != StringHash("node"));
        REQUIRE(StringHash("NODE") != StringHash("node"));
    }
}

TEST_CASE("StringHash Calculate matches construction", "[Math]")
{
    REQUIRE(StringHash::Calculate("Scene") == StringHash("Scene").Value());
    REQUIRE(StringHash::Calculate("") == 0u);

    SECTION("an initial hash seeds the accumulation")
    {
        const unsigned seeded = StringHash::Calculate("b", StringHash::Calculate("a"));
        REQUIRE(seeded == StringHash::Calculate("ab"));
    }
}

TEST_CASE("StringHash operators", "[Math]")
{
    const StringHash a(10u);
    const StringHash b(20u);

    REQUIRE((a + b).Value() == 30u);

    StringHash acc(5u);
    acc += StringHash(7u);
    REQUIRE(acc.Value() == 12u);

    REQUIRE(a < b);
    REQUIRE(b > a);
    REQUIRE_FALSE(a > b);
    REQUIRE_FALSE(b < a);
    REQUIRE(a != b);

    REQUIRE(static_cast<bool>(a));
    REQUIRE_FALSE(static_cast<bool>(StringHash::ZERO));

    REQUIRE(a.ToHash() == a.Value());
}

TEST_CASE("StringHash string representation", "[Math]")
{
    const StringHash hash(0xdeadbeefu);
    REQUIRE(hash.ToString() == "DEADBEEF");
    REQUIRE(StringHash(0x1u).ToString() == "00000001");

    SECTION("Reverse only resolves when hash reversing is compiled in")
    {
#ifdef URHO3D_HASH_DEBUG
        REQUIRE(StringHash("Node").Reverse() == "Node");
#else
        REQUIRE(StringHash("Node").Reverse().Empty());
#endif
    }
}

TEST_CASE("StringHashRegister round trips names", "[Math]")
{
    StringHashRegister registerInstance(false);

    const StringHash hash = registerInstance.RegisterString("MyCustomName");
    REQUIRE(hash == StringHash("MyCustomName"));
    REQUIRE(registerInstance.GetStringCopy(hash) == "MyCustomName");

    SECTION("registering the same string twice is stable")
    {
        REQUIRE(registerInstance.RegisterString("MyCustomName") == hash);
        REQUIRE(registerInstance.GetInternalMap().Size() == 1);
    }

    SECTION("an unknown hash resolves to an empty string")
    {
        REQUIRE(registerInstance.GetStringCopy(StringHash(0x12345678u)).Empty());
    }
}

TEST_CASE("StringHash Calculate tolerates a null pointer", "[Math]")
{
    REQUIRE(StringHash::Calculate(nullptr) == 0u);
    REQUIRE(StringHash::Calculate(nullptr, 1234u) == 1234u);
}

TEST_CASE("StringHash global register presence follows the build option", "[Math]")
{
    StringHashRegister* global = StringHash::GetGlobalStringHashRegister();
#ifdef URHO3D_HASH_DEBUG
    REQUIRE(global != nullptr);
#else
    REQUIRE(global == nullptr);
#endif
}

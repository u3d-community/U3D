#include "TestUtils.h"

#include <Urho3D/Container/Str.h>
#include <Urho3D/Container/Vector.h>

using namespace Urho3D;

static const unsigned kNpos = String::NPOS;

TEST_CASE("String construction", "[Container]")
{
    REQUIRE(String().Empty());
    REQUIRE(String().Length() == 0);

    const String fromLiteral("hello");
    REQUIRE(fromLiteral.Length() == 5);
    REQUIRE_FALSE(fromLiteral.Empty());
    REQUIRE(fromLiteral == "hello");

    REQUIRE(String("hello", 3) == "hel");
    REQUIRE(String(fromLiteral) == "hello");

    REQUIRE(String('x', 3) == "xxx");
    REQUIRE(String('x', 0).Empty());

    SECTION("numeric constructors")
    {
        REQUIRE(String(42) == "42");
        REQUIRE(String(-42) == "-42");
        REQUIRE(String(42u) == "42");
        REQUIRE(String(true) == "true");
        REQUIRE(String(false) == "false");
        REQUIRE(String('c') == "c");
    }

    SECTION("assignment")
    {
        String target;
        target = "abc";
        REQUIRE(target == "abc");
        target = String("def");
        REQUIRE(target == "def");
        target = String('z');
        REQUIRE(target == "z");
    }
}

TEST_CASE("String comparison", "[Container]")
{
    REQUIRE(String("abc") == String("abc"));
    REQUIRE(String("abc") == "abc");
    REQUIRE(String("abc") != "abd");
    REQUIRE(String("abc") < String("abd"));
    REQUIRE(String("abd") > String("abc"));

    REQUIRE(String("abc").Compare("abc") == 0);
    REQUIRE(String("abc").Compare("abd") < 0);
    REQUIRE(String("abd").Compare("abc") > 0);

    SECTION("case insensitive comparison")
    {
        REQUIRE(String("ABC").Compare("abc", false) == 0);
        REQUIRE(String("ABC").Compare("abc", true) != 0);
    }

    SECTION("shorter strings sort before their own prefixes extended")
    {
        REQUIRE(String("ab").Compare("abc") < 0);
        REQUIRE(String("abc").Compare("ab") > 0);
    }
}

TEST_CASE("String concatenation", "[Container]")
{
    REQUIRE(String("foo") + String("bar") == "foobar");
    REQUIRE(String("foo") + "bar" == "foobar");
    REQUIRE("foo" + String("bar") == "foobar");

    String acc("foo");
    acc += "bar";
    REQUIRE(acc == "foobar");
    acc += String("!");
    REQUIRE(acc == "foobar!");
    acc += 'z';
    REQUIRE(acc == "foobar!z");

    SECTION("appending numbers converts them")
    {
        String number("n=");
        number += 42;
        REQUIRE(number == "n=42");
    }

    SECTION("Append variants")
    {
        String s("a");
        s.Append("bc");
        REQUIRE(s == "abc");
        s.Append(String("de"));
        REQUIRE(s == "abcde");
        s.Append('f');
        REQUIRE(s == "abcdef");
        s.Append("ghXX", 2);
        REQUIRE(s == "abcdefgh");
    }

    SECTION("AppendWithFormat handles the single character specifiers")
    {
        String s;
        s.AppendWithFormat("%d-%s-%f", 7, "x", 2.5f);
        REQUIRE(s == "7-x-2.5");

        String more;
        more.AppendWithFormat("%u %c", 42u, 'z');
        REQUIRE(more == "42 z");
    }

    SECTION("precision specifiers are not supported and leak into the output")
    {
        String s;
        s.AppendWithFormat("%.1f", 2.5f);
        REQUIRE(s == "1f");
    }
}

TEST_CASE("String indexing", "[Container]")
{
    String s("hello");

    REQUIRE(s[0] == 'h');
    REQUIRE(s[4] == 'o');
    REQUIRE(s.At(1) == 'e');
    REQUIRE(s.Front() == 'h');
    REQUIRE(s.Back() == 'o');

    s[0] = 'H';
    REQUIRE(s == "Hello");
    s.At(4) = 'O';
    REQUIRE(s == "HellO");

    const String constString("abc");
    REQUIRE(constString[1] == 'b');
    REQUIRE(constString.At(2) == 'c');

    REQUIRE(s.CString()[0] == 'H');
    REQUIRE(s.CString()[s.Length()] == '\0');
}

TEST_CASE("String search", "[Container]")
{
    const String s("hello world hello");

    REQUIRE(s.Find('h') == 0);
    REQUIRE(s.Find('w') == 6);
    REQUIRE(s.Find('z') == kNpos);
    REQUIRE(s.Find('h', 1) == 12);

    REQUIRE(s.Find("hello") == 0);
    REQUIRE(s.Find("hello", 1) == 12);
    REQUIRE(s.Find("nothing") == kNpos);

    REQUIRE(s.FindLast('h') == 12);
    REQUIRE(s.FindLast("hello") == 12);
    REQUIRE(s.FindLast('z') == kNpos);

    REQUIRE(s.Contains('w'));
    REQUIRE(s.Contains("world"));
    REQUIRE_FALSE(s.Contains("WORLD"));
    REQUIRE(s.Contains("WORLD", false));

    REQUIRE(s.StartsWith("hello"));
    REQUIRE_FALSE(s.StartsWith("world"));
    REQUIRE(s.StartsWith("HELLO", false));

    REQUIRE(s.EndsWith("hello"));
    REQUIRE_FALSE(s.EndsWith("world"));
    REQUIRE(s.EndsWith("HELLO", false));

    SECTION("searching an empty string")
    {
        REQUIRE(String().Find('a') == kNpos);
        REQUIRE(String().FindLast('a') == kNpos);
        REQUIRE_FALSE(String().StartsWith("a"));
    }

    SECTION("case insensitive character search")
    {
        REQUIRE(String("ABC").Find('b', 0, false) == 1);
        REQUIRE(String("ABC").Find('b', 0, true) == kNpos);
    }
}

TEST_CASE("String Substring", "[Container]")
{
    const String s("hello world");

    REQUIRE(s.Substring(0) == "hello world");
    REQUIRE(s.Substring(6) == "world");
    REQUIRE(s.Substring(0, 5) == "hello");
    REQUIRE(s.Substring(6, 5) == "world");

    SECTION("a position past the end yields an empty string")
    {
        REQUIRE(s.Substring(100).Empty());
        REQUIRE(s.Substring(100, 5).Empty());
    }

    SECTION("a length past the end is clamped")
    {
        REQUIRE(s.Substring(6, 100) == "world");
    }
}

TEST_CASE("String case conversion and trimming", "[Container]")
{
    REQUIRE(String("Hello").ToLower() == "hello");
    REQUIRE(String("Hello").ToUpper() == "HELLO");
    REQUIRE(String("123").ToLower() == "123");

    REQUIRE(String("  hello  ").Trimmed() == "hello");
    REQUIRE(String("hello").Trimmed() == "hello");

    REQUIRE(String("\thello\t").Trimmed() == "hello");
    REQUIRE(String("\nhello\r").Trimmed() == "\nhello\r");
    REQUIRE(String("   ").Trimmed().Empty());
    REQUIRE(String().Trimmed().Empty());
}

TEST_CASE("String Replace", "[Container]")
{
    SECTION("character replacement")
    {
        String s("banana");
        s.Replace('a', 'o');
        REQUIRE(s == "bonono");
        REQUIRE(String("banana").Replaced('a', 'o') == "bonono");
    }

    SECTION("substring replacement")
    {
        String s("one two one");
        s.Replace("one", "three");
        REQUIRE(s == "three two three");
        REQUIRE(String("one two one").Replaced("one", "three") == "three two three");
    }

    SECTION("replacing with a longer and a shorter string")
    {
        REQUIRE(String("aaa").Replaced("a", "bb") == "bbbbbb");
        REQUIRE(String("aabbaa").Replaced("aa", "c") == "cbbc");
    }

    SECTION("replacing a range by position")
    {
        String s("hello world");
        s.Replace(0, 5, "goodbye");
        REQUIRE(s == "goodbye world");
    }

    SECTION("case insensitive replacement")
    {
        REQUIRE(String("Hello").Replaced("HELLO", "bye", false) == "bye");
        REQUIRE(String("Hello").Replaced("HELLO", "bye", true) == "Hello");
    }

    SECTION("replacing something absent leaves the string alone")
    {
        REQUIRE(String("abc").Replaced("z", "y") == "abc");
    }
}

TEST_CASE("String Insert and Erase", "[Container]")
{
    String s("hello");
    s.Insert(5, " world");
    REQUIRE(s == "hello world");

    s.Insert(0, '>');
    REQUIRE(s == ">hello world");

    s.Erase(0, 1);
    REQUIRE(s == "hello world");

    s.Erase(5, 6);
    REQUIRE(s == "hello");

    s.Erase(0);
    REQUIRE(s == "ello");

    SECTION("inserting past the end appends")
    {
        String target("ab");
        target.Insert(100, "c");
        REQUIRE(target == "abc");
    }
}

TEST_CASE("String Resize, Reserve, Compact and Clear", "[Container]")
{
    String s("hello");

    s.Reserve(100);
    REQUIRE(s.Capacity() >= 100);
    REQUIRE(s == "hello");

    s.Compact();
    REQUIRE(s == "hello");

    s.Resize(3);
    REQUIRE(s == "hel");
    REQUIRE(s.Length() == 3);

    s.Resize(5);
    REQUIRE(s.Length() == 5);

    s.Clear();
    REQUIRE(s.Empty());
    REQUIRE(s.Length() == 0);
}

TEST_CASE("String Swap", "[Container]")
{
    String a("first");
    String b("second");
    a.Swap(b);
    REQUIRE(a == "second");
    REQUIRE(b == "first");
}

TEST_CASE("String Split and Join", "[Container]")
{
    const Vector<String> parts = String("a,b,c").Split(',');
    REQUIRE(parts.Size() == 3);
    REQUIRE(parts[0] == "a");
    REQUIRE(parts[1] == "b");
    REQUIRE(parts[2] == "c");

    SECTION("empty fields are dropped by default and kept on request")
    {
        REQUIRE(String("a,,b").Split(',').Size() == 2);
        REQUIRE(String("a,,b").Split(',', true).Size() == 3);
    }

    SECTION("a string with no separator yields one field")
    {
        const Vector<String> single = String("abc").Split(',');
        REQUIRE(single.Size() == 1);
        REQUIRE(single[0] == "abc");
    }

    SECTION("Join glues the fields back together")
    {
        String joined;
        joined.Join(parts, ",");
        REQUIRE(joined == "a,b,c");

        String empty;
        empty.Join(Vector<String>(), ",");
        REQUIRE(empty.Empty());
    }

    SECTION("the static Joined helper matches")
    {
        REQUIRE(String::Joined(parts, "-") == "a-b-c");
    }
}

TEST_CASE("String iteration", "[Container]")
{
    String s("abc");

    unsigned count = 0;
    for (String::Iterator i = s.Begin(); i != s.End(); ++i)
        ++count;
    REQUIRE(count == 3);

    const String constString("abcd");
    count = 0;
    for (String::ConstIterator i = constString.Begin(); i != constString.End(); ++i)
        ++count;
    REQUIRE(count == 4);
}

TEST_CASE("String ToHash", "[Container]")
{
    REQUIRE(String("abc").ToHash() == String("abc").ToHash());
    REQUIRE(String("abc").ToHash() != String("abd").ToHash());
    REQUIRE(String().ToHash() == 0);
}

TEST_CASE("String static helpers", "[Container]")
{
    REQUIRE(String::EMPTY.Empty());

    REQUIRE(String::CStringLength("hello") == 5);
    REQUIRE(String::CStringLength("") == 0);
    REQUIRE(String::CStringLength(nullptr) == 0);

    SECTION("Comparison of raw pointers")
    {
        REQUIRE(String::Compare("abc", "abc", true) == 0);
        REQUIRE(String::Compare("ABC", "abc", false) == 0);
        REQUIRE(String::Compare("abc", "abd", true) < 0);
        REQUIRE(String::Compare(nullptr, "abc", true) < 0);
        REQUIRE(String::Compare("abc", nullptr, true) > 0);
        REQUIRE(String::Compare(nullptr, nullptr, true) == 0);
    }
}

TEST_CASE("String UTF8 handling", "[Container]")
{
    String s;
    s.AppendUTF8(0x41);
    REQUIRE(s == "A");

    s.AppendUTF8(0x00e9);
    REQUIRE(s.LengthUTF8() == 2);
    REQUIRE(s.Length() > 2);

    REQUIRE(s.AtUTF8(0) == 0x41u);
    REQUIRE(s.AtUTF8(1) == 0x00e9u);

    SECTION("byte offsets map to character offsets")
    {
        REQUIRE(s.ByteOffsetUTF8(0) == 0);
        REQUIRE(s.ByteOffsetUTF8(1) == 1);
    }

    SECTION("SubstringUTF8 works in characters")
    {
        REQUIRE(s.SubstringUTF8(0, 1) == "A");
        REQUIRE(s.SubstringUTF8(1).LengthUTF8() == 1);
    }

    SECTION("SetUTF8FromLatin1 round trips ASCII")
    {
        String latin;
        latin.SetUTF8FromLatin1("abc");
        REQUIRE(latin == "abc");
    }
}

TEST_CASE("WString wraps a wide character buffer", "[Container]")
{
    const WString wide(String("hello"));
    REQUIRE(wide.Length() == 5);
    REQUIRE_FALSE(wide.Empty());
    REQUIRE(wide.CString() != nullptr);

    WString empty;
    REQUIRE(empty.Empty());
    REQUIRE(empty.Length() == 0);

    WString resized(String("abc"));
    resized.Resize(2);
    REQUIRE(resized.Length() == 2);
}

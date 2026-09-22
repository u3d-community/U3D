#pragma once

#include <catch.hpp>

#include <Urho3D/Container/Pair.h>
#include <Urho3D/Core/Context.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/Math/MathDefs.h>
#include <Urho3D/Math/StringHash.h>
#include <Urho3D/Math/Vector2.h>
#include <Urho3D/Math/Vector3.h>

namespace Urho3D
{

class Engine;

}

namespace U3DTest
{

class TestContext
{
public:
    TestContext();
    ~TestContext();

    Urho3D::Context* operator->() const { return context_; }
    operator Urho3D::Context*() const { return context_; }
    Urho3D::Context* Get() const { return context_; }

private:
    Urho3D::SharedPtr<Urho3D::Context> context_;
};

Urho3D::Context* HeadlessContext();

Urho3D::String ScratchPath(const Urho3D::String& name);

void RemoveScratch(const Urho3D::String& name);

Urho3D::String ResourcePath(const Urho3D::String& name);

}

namespace Catch
{

template <> struct StringMaker<Urho3D::String>
{
    static std::string convert(const Urho3D::String& value) { return std::string(value.CString(), value.Length()); }
};

template <class T, class U> struct is_range<Urho3D::Pair<T, U>>
{
    static const bool value = false;
};

template <class T, class U> struct StringMaker<Urho3D::Pair<T, U>>
{
    static std::string convert(const Urho3D::Pair<T, U>& value)
    {
        return "(" + Detail::stringify(value.first_) + ", " + Detail::stringify(value.second_) + ")";
    }
};

template <> struct StringMaker<Urho3D::StringHash>
{
    static std::string convert(const Urho3D::StringHash& value)
    {
        return StringMaker<Urho3D::String>::convert(value.ToString());
    }
};

template <> struct StringMaker<Urho3D::Vector2>
{
    static std::string convert(const Urho3D::Vector2& value)
    {
        return StringMaker<Urho3D::String>::convert(value.ToString());
    }
};

template <> struct StringMaker<Urho3D::Vector3>
{
    static std::string convert(const Urho3D::Vector3& value)
    {
        return StringMaker<Urho3D::String>::convert(value.ToString());
    }
};

template <> struct StringMaker<Urho3D::IntVector2>
{
    static std::string convert(const Urho3D::IntVector2& value)
    {
        return StringMaker<Urho3D::String>::convert(value.ToString());
    }
};

}

#define REQUIRE_EQ_F(a, b) REQUIRE(Urho3D::Equals((a), (b)))
#define CHECK_EQ_F(a, b) CHECK(Urho3D::Equals((a), (b)))

#define REQUIRE_NEAR(a, b, eps) REQUIRE(Urho3D::Abs((a) - (b)) <= (eps))
#define CHECK_NEAR(a, b, eps) CHECK(Urho3D::Abs((a) - (b)) <= (eps))

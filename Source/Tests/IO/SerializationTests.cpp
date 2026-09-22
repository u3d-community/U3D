#include "TestUtils.h"

#include <Urho3D/IO/Compression.h>
#include <Urho3D/IO/File.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/IO/MemoryBuffer.h>
#include <Urho3D/IO/VectorBuffer.h>

using namespace Urho3D;
using namespace U3DTest;

TEST_CASE("VectorBuffer round trips every primitive", "[IO]")
{
    VectorBuffer buffer;

    buffer.WriteInt(-42);
    buffer.WriteUInt(42u);
    buffer.WriteShort(-7);
    buffer.WriteUShort(7);
    buffer.WriteByte(-1);
    buffer.WriteUByte(200);
    buffer.WriteBool(true);
    buffer.WriteBool(false);
    buffer.WriteFloat(2.5f);
    buffer.WriteDouble(3.5);
    buffer.WriteString("hello");
    buffer.WriteFileID("ABCD");
    buffer.WriteStringHash(StringHash("Node"));

    buffer.Seek(0);

    REQUIRE(buffer.ReadInt() == -42);
    REQUIRE(buffer.ReadUInt() == 42u);
    REQUIRE(buffer.ReadShort() == -7);
    REQUIRE(buffer.ReadUShort() == 7);
    REQUIRE(buffer.ReadByte() == -1);
    REQUIRE(buffer.ReadUByte() == 200);
    REQUIRE(buffer.ReadBool());
    REQUIRE_FALSE(buffer.ReadBool());
    REQUIRE_EQ_F(buffer.ReadFloat(), 2.5f);
    REQUIRE(buffer.ReadDouble() == 3.5);
    REQUIRE(buffer.ReadString() == "hello");
    REQUIRE(buffer.ReadFileID() == "ABCD");
    REQUIRE(buffer.ReadStringHash() == StringHash("Node"));

    REQUIRE(buffer.IsEof());
}

TEST_CASE("VectorBuffer round trips math types", "[IO]")
{
    VectorBuffer buffer;

    buffer.WriteVector2(Vector2(1.0f, 2.0f));
    buffer.WriteVector3(Vector3(1.0f, 2.0f, 3.0f));
    buffer.WriteVector4(Vector4(1.0f, 2.0f, 3.0f, 4.0f));
    buffer.WriteIntVector2(IntVector2(1, 2));
    buffer.WriteIntVector3(IntVector3(1, 2, 3));
    buffer.WriteQuaternion(Quaternion::IDENTITY);
    buffer.WriteColor(Color::RED);
    buffer.WriteBoundingBox(BoundingBox(-1.0f, 1.0f));
    buffer.WriteRect(Rect(1.0f, 2.0f, 3.0f, 4.0f));
    buffer.WriteIntRect(IntRect(1, 2, 3, 4));
    buffer.WriteMatrix3(Matrix3::IDENTITY);
    buffer.WriteMatrix3x4(Matrix3x4::IDENTITY);
    buffer.WriteMatrix4(Matrix4::IDENTITY);

    buffer.Seek(0);

    REQUIRE(buffer.ReadVector2() == Vector2(1.0f, 2.0f));
    REQUIRE(buffer.ReadVector3() == Vector3(1.0f, 2.0f, 3.0f));
    REQUIRE(buffer.ReadVector4() == Vector4(1.0f, 2.0f, 3.0f, 4.0f));
    REQUIRE(buffer.ReadIntVector2() == IntVector2(1, 2));
    REQUIRE(buffer.ReadIntVector3() == IntVector3(1, 2, 3));
    REQUIRE(buffer.ReadQuaternion() == Quaternion::IDENTITY);
    REQUIRE(buffer.ReadColor() == Color::RED);
    REQUIRE(buffer.ReadBoundingBox() == BoundingBox(-1.0f, 1.0f));
    REQUIRE(buffer.ReadRect() == Rect(1.0f, 2.0f, 3.0f, 4.0f));
    REQUIRE(buffer.ReadIntRect() == IntRect(1, 2, 3, 4));
    REQUIRE(buffer.ReadMatrix3().Equals(Matrix3::IDENTITY));
    REQUIRE(buffer.ReadMatrix3x4().Equals(Matrix3x4::IDENTITY));
    REQUIRE(buffer.ReadMatrix4().Equals(Matrix4::IDENTITY));
}

TEST_CASE("VectorBuffer round trips variants", "[IO]")
{
    VectorBuffer buffer;

    VariantMap map;
    map["key"] = 7;

    VariantVector vector;
    vector.Push(Variant(1));
    vector.Push(Variant("two"));

    buffer.WriteVariant(Variant(42));
    buffer.WriteVariant(Variant(Vector3::ONE));
    buffer.WriteVariantMap(map);
    buffer.WriteVariantVector(vector);

    buffer.Seek(0);

    REQUIRE(buffer.ReadVariant().GetInt() == 42);
    REQUIRE(buffer.ReadVariant().GetVector3() == Vector3::ONE);
    REQUIRE(buffer.ReadVariantMap()["key"].GetInt() == 7);

    const VariantVector restored = buffer.ReadVariantVector();
    REQUIRE(restored.Size() == 2);
    REQUIRE(restored[1].GetString() == "two");
}

TEST_CASE("VectorBuffer variable length integers", "[IO]")
{
    VectorBuffer buffer;

    buffer.WriteVLE(0);
    buffer.WriteVLE(127);
    buffer.WriteVLE(128);
    buffer.WriteVLE(16383);
    buffer.WriteVLE(16384);
    buffer.WriteVLE(1000000);

    buffer.Seek(0);

    REQUIRE(buffer.ReadVLE() == 0u);
    REQUIRE(buffer.ReadVLE() == 127u);
    REQUIRE(buffer.ReadVLE() == 128u);
    REQUIRE(buffer.ReadVLE() == 16383u);
    REQUIRE(buffer.ReadVLE() == 16384u);
    REQUIRE(buffer.ReadVLE() == 1000000u);

    SECTION("small values use fewer bytes than large ones")
    {
        VectorBuffer small;
        small.WriteVLE(1);
        VectorBuffer large;
        large.WriteVLE(1000000);
        REQUIRE(small.GetSize() < large.GetSize());
    }
}

TEST_CASE("VectorBuffer seeking and size", "[IO]")
{
    VectorBuffer buffer;
    buffer.WriteInt(1);
    buffer.WriteInt(2);
    buffer.WriteInt(3);

    REQUIRE(buffer.GetSize() == 12);
    REQUIRE(buffer.GetPosition() == 12);
    REQUIRE(buffer.IsEof());

    buffer.Seek(4);
    REQUIRE(buffer.GetPosition() == 4);
    REQUIRE_FALSE(buffer.IsEof());
    REQUIRE(buffer.ReadInt() == 2);

    SECTION("seeking past the end clamps to the end")
    {
        buffer.Seek(1000);
        REQUIRE(buffer.GetPosition() == buffer.GetSize());
        REQUIRE(buffer.IsEof());
    }

    SECTION("Clear empties the buffer")
    {
        buffer.Clear();
        REQUIRE(buffer.GetSize() == 0);
        REQUIRE(buffer.IsEof());
    }

    SECTION("Resize truncates the contents")
    {
        buffer.Resize(4);
        REQUIRE(buffer.GetSize() == 4);
        buffer.Seek(0);
        REQUIRE(buffer.ReadInt() == 1);
    }
}

TEST_CASE("VectorBuffer construction from existing data", "[IO]")
{
    PODVector<unsigned char> data;
    for (unsigned char i = 0; i < 16; ++i)
        data.Push(i);

    VectorBuffer buffer(data);
    REQUIRE(buffer.GetSize() == 16);
    REQUIRE(buffer.ReadUByte() == 0);
    REQUIRE(buffer.ReadUByte() == 1);

    VectorBuffer fromRaw(data.Buffer(), data.Size());
    REQUIRE(fromRaw.GetSize() == 16);

    REQUIRE(buffer.GetData() != nullptr);
    REQUIRE(buffer.GetBuffer().Size() == 16);
}

TEST_CASE("MemoryBuffer reads without owning", "[IO]")
{
    VectorBuffer source;
    source.WriteInt(7);
    source.WriteString("text");

    MemoryBuffer reader(source.GetData(), source.GetSize());
    REQUIRE(reader.GetSize() == source.GetSize());
    REQUIRE(reader.ReadInt() == 7);
    REQUIRE(reader.ReadString() == "text");
    REQUIRE(reader.IsEof());

    SECTION("an empty buffer reads nothing rather than crashing")
    {
        MemoryBuffer empty(static_cast<void*>(nullptr), 0);
        REQUIRE(empty.GetSize() == 0);
        REQUIRE(empty.IsEof());

        int target = 0;
        REQUIRE(empty.Read(&target, sizeof target) == 0);
        REQUIRE(target == 0);
    }

    SECTION("a read only buffer reports itself as such")
    {
        const unsigned char data[] = {1, 2, 3, 4};
        MemoryBuffer readOnly(data, 4);
        REQUIRE(readOnly.IsReadOnly());
        REQUIRE(readOnly.ReadUByte() == 1);
    }
}

TEST_CASE("Reading past the end is clamped", "[IO]")
{
    VectorBuffer buffer;
    buffer.WriteUByte(1);
    buffer.Seek(0);

    REQUIRE(buffer.ReadUByte() == 1);
    REQUIRE(buffer.IsEof());

    int target = 0;
    REQUIRE(buffer.Read(&target, sizeof target) == 0);
    REQUIRE(buffer.GetPosition() == buffer.GetSize());
    REQUIRE(buffer.IsEof());

    REQUIRE(buffer.ReadString().Empty());
}

TEST_CASE("Compression round trips a buffer", "[IO]")
{
    VectorBuffer source;
    for (int i = 0; i < 500; ++i)
        source.WriteInt(i % 7);

    source.Seek(0);
    VectorBuffer compressed = CompressVectorBuffer(source);
    REQUIRE(compressed.GetSize() > 0);

    compressed.Seek(0);
    VectorBuffer restored = DecompressVectorBuffer(compressed);

    REQUIRE(restored.GetSize() == source.GetSize());
    restored.Seek(0);
    for (int i = 0; i < 500; ++i)
        REQUIRE(restored.ReadInt() == i % 7);

    SECTION("highly repetitive data actually shrinks")
    {
        VectorBuffer repetitive;
        for (int i = 0; i < 2000; ++i)
            repetitive.WriteInt(0);
        repetitive.Seek(0);
        REQUIRE(CompressVectorBuffer(repetitive).GetSize() < repetitive.GetSize());
    }

    SECTION("an empty buffer compresses to a header only block and back to empty")
    {
        VectorBuffer empty;
        VectorBuffer compressedEmpty = CompressVectorBuffer(empty);
        REQUIRE(compressedEmpty.GetSize() == 8);

        compressedEmpty.Seek(0);
        REQUIRE(DecompressVectorBuffer(compressedEmpty).GetSize() == 0);
    }
}

TEST_CASE("File round trips through the filesystem", "[IO]")
{
    TestContext context;
    auto* fileSystem = context->GetSubsystem<FileSystem>();

    const String path = ScratchPath("io-roundtrip.bin");
    RemoveScratch("io-roundtrip.bin");

    {
        File file(context, path, FILE_WRITE);
        REQUIRE(file.IsOpen());
        REQUIRE(file.GetMode() == FILE_WRITE);
        file.WriteInt(1234);
        file.WriteString("payload");
        file.WriteVector3(Vector3::ONE);
        file.Close();
        REQUIRE_FALSE(file.IsOpen());
    }

    REQUIRE(fileSystem->FileExists(path));

    {
        File file(context, path, FILE_READ);
        REQUIRE(file.IsOpen());
        REQUIRE(file.GetName() == path);
        REQUIRE(file.GetSize() > 0);
        REQUIRE(file.ReadInt() == 1234);
        REQUIRE(file.ReadString() == "payload");
        REQUIRE(file.ReadVector3() == Vector3::ONE);
        REQUIRE(file.IsEof());
    }

    SECTION("the whole file can be read into a buffer")
    {
        File file(context, path, FILE_READ);
        PODVector<unsigned char> data(file.GetSize());
        REQUIRE(file.Read(data.Buffer(), file.GetSize()) == file.GetSize());
    }

    SECTION("opening a missing file fails rather than throwing")
    {
        File missing(context, ScratchPath("does-not-exist.bin"), FILE_READ);
        REQUIRE_FALSE(missing.IsOpen());
    }

    fileSystem->Delete(path);
    REQUIRE_FALSE(fileSystem->FileExists(path));
}

TEST_CASE("FileSystem path helpers", "[IO]")
{
    REQUIRE(GetPath("Models/Box.mdl") == "Models/");
    REQUIRE(GetFileName("Models/Box.mdl") == "Box");
    REQUIRE(GetExtension("Models/Box.mdl") == ".mdl");
    REQUIRE(GetFileNameAndExtension("Models/Box.mdl") == "Box.mdl");

    REQUIRE(GetExtension("Models/Box.MDL") == ".mdl");
    REQUIRE(GetExtension("Models/Box.MDL", false) == ".MDL");
    REQUIRE(GetExtension("noextension").Empty());

    REQUIRE(AddTrailingSlash("Models") == "Models/");
    REQUIRE(AddTrailingSlash("Models/") == "Models/");
    REQUIRE(RemoveTrailingSlash("Models/") == "Models");

    REQUIRE(GetParentPath("Models/Sub/") == "Models/");
    REQUIRE(GetInternalPath("Models\\Box.mdl") == "Models/Box.mdl");

    REQUIRE(ReplaceExtension("Models/Box.mdl", ".xml") == "Models/Box.xml");

    SECTION("absolute paths are recognised")
    {
        REQUIRE(IsAbsolutePath("/usr/lib"));
        REQUIRE_FALSE(IsAbsolutePath("relative/path"));
        REQUIRE_FALSE(IsAbsolutePath(""));
    }
}

TEST_CASE("FileSystem directory operations", "[IO]")
{
    TestContext context;
    auto* fileSystem = context->GetSubsystem<FileSystem>();

    const String directory = ScratchPath("io-dir-test");
    const String inner = directory + "/file.txt";
    if (fileSystem->FileExists(inner))
        fileSystem->Delete(inner);

    REQUIRE(fileSystem->CreateDir(directory));
    REQUIRE(fileSystem->DirExists(directory));

    {
        File file(context, inner, FILE_WRITE);
        file.WriteString("x");
    }
    REQUIRE(fileSystem->FileExists(inner));

    Vector<String> found;
    fileSystem->ScanDir(found, directory, "*", SCAN_FILES, false);
    REQUIRE(found.Size() == 1);
    REQUIRE(found[0] == "file.txt");

    REQUIRE(fileSystem->GetLastModifiedTime(inner) > 0);

    REQUIRE(fileSystem->Delete(inner));
    REQUIRE_FALSE(fileSystem->FileExists(inner));
    REQUIRE(fileSystem->DirExists(directory));
}

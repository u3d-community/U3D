#include "TestUtils.h"

#include <Urho3D/IO/Compression.h>
#include <Urho3D/IO/MemoryBuffer.h>
#include <Urho3D/IO/VectorBuffer.h>

#include <cstring>

using namespace Urho3D;

TEST_CASE("Serializer round trips extended scalar and collection types", "[IO]")
{
    VectorBuffer buffer;
    PODVector<unsigned char> bytes;
    bytes.Push(0);
    bytes.Push(127);
    bytes.Push(255);
    StringVector strings;
    strings.Push("one");
    strings.Push("");
    strings.Push("three");
    ResourceRef reference(StringHash("Texture2D"), "Textures/Test.png");
    ResourceRefList references(StringHash("Material"));
    references.names_.Push("Materials/A.xml");
    references.names_.Push("Materials/B.xml");

    REQUIRE(buffer.WriteInt64(-0x123456789LL));
    REQUIRE(buffer.WriteUInt64(0xfedcba9876543210ULL));
    REQUIRE(buffer.WriteBuffer(bytes));
    REQUIRE(buffer.WriteStringVector(strings));
    REQUIRE(buffer.WriteResourceRef(reference));
    REQUIRE(buffer.WriteResourceRefList(references));
    REQUIRE(buffer.WriteNetID(0xabcdef));
    REQUIRE(buffer.WriteLine("first"));
    REQUIRE(buffer.WriteLine("second"));

    buffer.Seek(0);
    REQUIRE(buffer.ReadInt64() == -0x123456789LL);
    REQUIRE(buffer.ReadUInt64() == 0xfedcba9876543210ULL);
    REQUIRE(buffer.ReadBuffer() == bytes);
    REQUIRE(buffer.ReadStringVector() == strings);
    const ResourceRef restoredReference = buffer.ReadResourceRef();
    REQUIRE(restoredReference.type_ == reference.type_);
    REQUIRE(restoredReference.name_ == reference.name_);
    const ResourceRefList restoredReferences = buffer.ReadResourceRefList();
    REQUIRE(restoredReferences.type_ == references.type_);
    REQUIRE(restoredReferences.names_ == references.names_);
    REQUIRE(buffer.ReadNetID() == 0xabcdef);
    REQUIRE(buffer.ReadLine() == "first");
    REQUIRE(buffer.ReadLine() == "second");
    REQUIRE(buffer.IsEof());
}

TEST_CASE("Packed vectors and quaternions retain bounded precision", "[IO]")
{
    const Vector3 vector(12.5f, -8.25f, 0.125f);
    const Quaternion quaternion(35.0f, Vector3(1.0f, 2.0f, 3.0f));
    VectorBuffer buffer;
    REQUIRE(buffer.WritePackedVector3(vector, 20.0f));
    REQUIRE(buffer.WritePackedQuaternion(quaternion));
    REQUIRE(buffer.GetSize() == 14);

    buffer.Seek(0);
    const Vector3 restoredVector = buffer.ReadPackedVector3(20.0f);
    const Quaternion restoredQuaternion = buffer.ReadPackedQuaternion();
    REQUIRE((restoredVector - vector).Length() < 0.002f);
    REQUIRE(Abs(restoredQuaternion.DotProduct(quaternion)) > 0.9999f);
}

TEST_CASE("Deserializer relative seeking clamps to stream bounds", "[IO]")
{
    const unsigned char data[] = {10, 20, 30, 40};
    MemoryBuffer buffer(data, sizeof data);
    REQUIRE(buffer.SeekRelative(2) == 2);
    REQUIRE(buffer.ReadUByte() == 30);
    REQUIRE(buffer.SeekRelative(-2) == 1);
    REQUIRE(buffer.ReadUByte() == 20);
    REQUIRE(buffer.SeekRelative(-100) == 0);
    REQUIRE(buffer.ReadUByte() == 10);
    REQUIRE(buffer.SeekRelative(100) == sizeof data);
    REQUIRE(buffer.IsEof());
    REQUIRE(buffer.GetName().Empty());
}

TEST_CASE("MemoryBuffer writes in place and clamps capacity", "[IO]")
{
    unsigned char storage[] = {1, 2, 3, 4};
    MemoryBuffer buffer(storage, sizeof storage);
    REQUIRE_FALSE(buffer.IsReadOnly());
    REQUIRE(buffer.WriteUInt(0x44332211u));
    REQUIRE(storage[0] == 0x11);
    REQUIRE(storage[3] == 0x44);
    REQUIRE(buffer.WriteUByte(9) == false);

    buffer.Seek(2);
    const unsigned char replacement[] = {8, 9, 10, 11};
    REQUIRE(buffer.Write(replacement, sizeof replacement) == 2);
    REQUIRE(storage[2] == 8);
    REQUIRE(storage[3] == 9);
}

TEST_CASE("Raw and streaming compression round trip", "[IO]")
{
    PODVector<unsigned char> source(4096);
    for (unsigned i = 0; i < source.Size(); ++i)
        source[i] = static_cast<unsigned char>((i * 17) % 251);

    PODVector<unsigned char> compressed(EstimateCompressBound(source.Size()));
    const unsigned compressedSize = CompressData(compressed.Buffer(), source.Buffer(), source.Size());
    REQUIRE(compressedSize > 0);
    REQUIRE(compressedSize <= compressed.Size());

    PODVector<unsigned char> restored(source.Size());
    REQUIRE(DecompressData(restored.Buffer(), compressed.Buffer(), restored.Size()) == compressedSize);
    REQUIRE(std::memcmp(source.Buffer(), restored.Buffer(), source.Size()) == 0);

    VectorBuffer input(source);
    VectorBuffer packed;
    REQUIRE(CompressStream(packed, input));
    REQUIRE(input.IsEof());
    packed.Seek(0);
    VectorBuffer output;
    REQUIRE(DecompressStream(output, packed));
    REQUIRE(output.GetBuffer() == source);
}

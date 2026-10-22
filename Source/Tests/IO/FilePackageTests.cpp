#include "TestUtils.h"

#include <Urho3D/IO/Compression.h>
#include <Urho3D/IO/File.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/IO/PackageFile.h>
#include <Urho3D/IO/VectorBuffer.h>

using namespace Urho3D;
using namespace U3DTest;

namespace
{

void WritePackage(Context* context, const String& path)
{
    const String entryName("Folder/hello.txt");
    const String payload("package payload");
    const unsigned headerSize = 4 + 4 + 4 + entryName.Length() + 1 + 4 + 4 + 4;

    VectorBuffer package;
    package.WriteFileID("UPAK");
    package.WriteUInt(1);
    package.WriteUInt(12345);
    package.WriteString(entryName);
    package.WriteUInt(headerSize);
    package.WriteUInt(payload.Length());
    package.WriteUInt(67890);
    package.Write(payload.CString(), payload.Length());
    package.WriteUInt(package.GetSize() + sizeof(unsigned));

    File output(context, path, FILE_WRITE);
    REQUIRE(output.IsOpen());
    REQUIRE(output.Write(package.GetData(), package.GetSize()) == package.GetSize());
}

const unsigned PACKAGE_BLOCK_SIZE = 32768;
const unsigned NO_CORRUPT_BLOCK = M_MAX_UNSIGNED;

String CompressiblePayload()
{
    String payload;
    for (unsigned i = 0; i < 20; ++i)
        payload += "lz4 block ";
    return payload;
}

String MultiBlockPayload()
{
    const char* words[] = {"scene ", "node ", "model ", "material ", "texture ", "shader ", "light ", "zone "};
    unsigned seed = 12345;
    String payload;
    while (payload.Length() < 80000)
    {
        seed = seed * 1103515245u + 12345u;
        payload += words[(seed >> 16) % 8];
        payload += String((seed >> 8) % 1000);
    }
    payload.Resize(80000);
    return payload;
}

unsigned WriteCompressedPackage(Context* context, const String& path, const String& payload, unsigned corruptBlock, unsigned firstBlockSize = PACKAGE_BLOCK_SIZE)
{
    const String entryName("packed.txt");
    const unsigned headerSize = 4 + 4 + 4 + entryName.Length() + 1 + 4 + 4 + 4;

    VectorBuffer package;
    package.WriteFileID("ULZ4");
    package.WriteUInt(1);
    package.WriteUInt(12345);
    package.WriteString(entryName);
    package.WriteUInt(headerSize);
    package.WriteUInt(payload.Length());
    package.WriteUInt(67890);

    PODVector<unsigned char> compressed(EstimateCompressBound(PACKAGE_BLOCK_SIZE));
    unsigned numBlocks = 0;
    for (unsigned pos = 0; pos < payload.Length();)
    {
        const unsigned blockSize = numBlocks ? PACKAGE_BLOCK_SIZE : firstBlockSize;
        const unsigned unpackedSize = Min(blockSize, payload.Length() - pos);
        const unsigned compressedSize = CompressData(compressed.Buffer(), payload.CString() + pos, unpackedSize);
        REQUIRE(compressedSize > 0);
        const bool corrupt = numBlocks == corruptBlock;
        package.WriteUShort((unsigned short)unpackedSize);
        package.WriteUShort((unsigned short)(corrupt ? compressedSize + 1 : compressedSize));
        package.Write(compressed.Buffer(), compressedSize);
        if (corrupt)
            package.WriteUByte(0x5A);
        pos += unpackedSize;
        ++numBlocks;
    }
    package.WriteUInt(package.GetSize() + sizeof(unsigned));

    File output(context, path, FILE_WRITE);
    REQUIRE(output.IsOpen());
    REQUIRE(output.Write(package.GetData(), package.GetSize()) == package.GetSize());
    return numBlocks;
}

}

TEST_CASE("File supports seek, overwrite, append, and checksums", "[IO]")
{
    TestContext context;
    const String path = ScratchPath("io-file-modes.bin");
    RemoveScratch("io-file-modes.bin");

    {
        File file(context, path, FILE_WRITE);
        REQUIRE(file.WriteString("abcdef"));
        REQUIRE(file.GetPosition() == 7);
        REQUIRE(file.Seek(2) == 2);
        REQUIRE(file.Write("XY", 2) == 2);
        file.Flush();
    }
    {
        File file(context, path, FILE_READWRITE);
        REQUIRE(file.IsOpen());
        REQUIRE(file.ReadString() == "abXYef");
        REQUIRE(file.Seek(file.GetSize()) == file.GetSize());
        REQUIRE(file.WriteUByte(42));
        REQUIRE(file.GetSize() == 8);
        const unsigned checksum = file.GetChecksum();
        REQUIRE(checksum != 0);
        REQUIRE(file.GetChecksum() == checksum);
        REQUIRE(file.GetPosition() == 8);
    }
    RemoveScratch("io-file-modes.bin");
}

TEST_CASE("PackageFile indexes entries and File reads their bounded data", "[IO]")
{
    TestContext context;
    const String path = ScratchPath("io-test.pak");
    RemoveScratch("io-test.pak");
    WritePackage(context, path);

    PackageFile package(context);
    REQUIRE(package.Open(path));
    REQUIRE(package.GetName() == path);
    REQUIRE(package.GetNameHash() == StringHash(path));
    REQUIRE(package.GetNumFiles() == 1);
    REQUIRE(package.GetChecksum() == 12345);
    REQUIRE_FALSE(package.IsCompressed());
    REQUIRE(package.Exists("Folder/hello.txt"));
    REQUIRE_FALSE(package.Exists("missing.txt"));
    REQUIRE(package.GetEntry("missing.txt") == nullptr);
    REQUIRE(package.GetEntryNames().Contains("Folder/hello.txt"));

    const PackageEntry* entry = package.GetEntry("Folder/hello.txt");
    REQUIRE(entry != nullptr);
    REQUIRE(entry->size_ == 15);
    REQUIRE(entry->checksum_ == 67890);
    REQUIRE(package.GetTotalDataSize() == entry->size_);

    File file(context, &package, "Folder/hello.txt");
    REQUIRE(file.IsOpen());
    REQUIRE(file.IsPackaged());
    REQUIRE(file.GetSize() == entry->size_);
    REQUIRE(file.GetChecksum() == entry->checksum_);
    String content;
    content.Resize(file.GetSize());
    REQUIRE(file.Read(&content[0], content.Length()) == content.Length());
    REQUIRE(content == "package payload");
    REQUIRE(file.ReadUByte() == 0);

    RemoveScratch("io-test.pak");
}

TEST_CASE("PackageFile rejects invalid and out-of-bounds directories", "[IO]")
{
    TestContext context;
    const String path = ScratchPath("io-invalid.pak");
    RemoveScratch("io-invalid.pak");

    {
        File file(context, path, FILE_WRITE);
        file.WriteFileID("NOPE");
        file.WriteUInt(0);
    }
    PackageFile package(context);
    REQUIRE_FALSE(package.Open(path));

    {
        File file(context, path, FILE_WRITE);
        file.WriteFileID("UPAK");
        file.WriteUInt(1);
        file.WriteUInt(0);
        file.WriteString("bad");
        file.WriteUInt(10000);
        file.WriteUInt(10);
        file.WriteUInt(0);
    }
    REQUIRE_FALSE(package.Open(path));
    RemoveScratch("io-invalid.pak");
}

TEST_CASE("Compressed package entries decompress block by block", "[IO]")
{
    TestContext context;
    const String path = ScratchPath("io-compressed.pak");
    RemoveScratch("io-compressed.pak");
    REQUIRE(WriteCompressedPackage(context, path, CompressiblePayload(), NO_CORRUPT_BLOCK) == 1);

    PackageFile package(context);
    REQUIRE(package.Open(path));
    REQUIRE(package.IsCompressed());

    File file(context, &package, "packed.txt");
    REQUIRE(file.IsOpen());
    REQUIRE(file.GetSize() == 200);
    String content;
    content.Resize(file.GetSize());
    REQUIRE(file.Read(&content[0], content.Length()) == 200);
    REQUIRE(content == CompressiblePayload());

    RemoveScratch("io-compressed.pak");
}

TEST_CASE("Compressed package read stops at a malformed block", "[IO]")
{
    TestContext context;
    const String path = ScratchPath("io-compressed-bad.pak");
    RemoveScratch("io-compressed-bad.pak");
    REQUIRE(WriteCompressedPackage(context, path, CompressiblePayload(), 0) == 1);

    PackageFile package(context);
    REQUIRE(package.Open(path));

    File file(context, &package, "packed.txt");
    REQUIRE(file.IsOpen());
    String content;
    content.Resize(file.GetSize());
    REQUIRE(file.Read(&content[0], content.Length()) == 0);

    RemoveScratch("io-compressed-bad.pak");
}

TEST_CASE("Compressed package entries spanning multiple blocks read and seek across block boundaries", "[IO]")
{
    TestContext context;
    const String path = ScratchPath("io-compressed-multi.pak");
    RemoveScratch("io-compressed-multi.pak");
    const String payload = MultiBlockPayload();
    REQUIRE(WriteCompressedPackage(context, path, payload, NO_CORRUPT_BLOCK) == 3);

    PackageFile package(context);
    REQUIRE(package.Open(path));
    REQUIRE(package.IsCompressed());

    File file(context, &package, "packed.txt");
    REQUIRE(file.IsOpen());
    REQUIRE(file.GetSize() == 80000);
    String content;
    content.Resize(80000);
    unsigned total = 0;
    while (total < 80000)
    {
        const unsigned read = file.Read(&content[total], Min(1000u, 80000 - total));
        REQUIRE(read > 0);
        total += read;
    }
    REQUIRE(content == payload);
    REQUIRE(file.IsEof());

    REQUIRE(file.Seek(0) == 0);
    REQUIRE(file.Seek(50000) == 50000);
    String slice;
    slice.Resize(100);
    REQUIRE(file.Read(&slice[0], 100) == 100);
    REQUIRE(slice == payload.Substring(50000, 100));

    REQUIRE(file.Seek(0) == 0);
    REQUIRE(file.Seek(32760) == 32760);
    String boundary;
    boundary.Resize(16);
    REQUIRE(file.Read(&boundary[0], 16) == 16);
    REQUIRE(boundary == payload.Substring(32760, 16));

    RemoveScratch("io-compressed-multi.pak");
}

TEST_CASE("Compressed package read and seek stop at a malformed middle block", "[IO]")
{
    TestContext context;
    const String path = ScratchPath("io-compressed-multi-bad.pak");
    RemoveScratch("io-compressed-multi-bad.pak");
    const String payload = MultiBlockPayload();
    REQUIRE(WriteCompressedPackage(context, path, payload, 1) == 3);

    PackageFile package(context);
    REQUIRE(package.Open(path));

    {
        File file(context, &package, "packed.txt");
        REQUIRE(file.IsOpen());
        String content;
        content.Resize(80000);
        REQUIRE(file.Read(&content[0], 80000) == PACKAGE_BLOCK_SIZE);
        REQUIRE(content.Substring(0, PACKAGE_BLOCK_SIZE) == payload.Substring(0, PACKAGE_BLOCK_SIZE));
    }
    {
        File file(context, &package, "packed.txt");
        REQUIRE(file.IsOpen());
        REQUIRE(file.Seek(50000) == PACKAGE_BLOCK_SIZE);
    }

    RemoveScratch("io-compressed-multi-bad.pak");
}

TEST_CASE("Compressed package reads a block larger than the first block", "[IO]")
{
    TestContext context;
    const String path = ScratchPath("io-compressed-growing.pak");
    RemoveScratch("io-compressed-growing.pak");
    const String payload = MultiBlockPayload().Substring(0, 30100);
    REQUIRE(WriteCompressedPackage(context, path, payload, NO_CORRUPT_BLOCK, 100) == 2);

    PackageFile package(context);
    REQUIRE(package.Open(path));

    File file(context, &package, "packed.txt");
    REQUIRE(file.IsOpen());
    String content;
    content.Resize(30100);
    REQUIRE(file.Read(&content[0], 30100) == 30100);
    REQUIRE(content == payload);

    RemoveScratch("io-compressed-growing.pak");
}

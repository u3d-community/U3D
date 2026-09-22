#include "TestUtils.h"

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

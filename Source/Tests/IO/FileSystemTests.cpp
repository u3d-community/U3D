#include "TestUtils.h"

#include <Urho3D/IO/File.h>
#include <Urho3D/IO/FileSystem.h>

using namespace Urho3D;
using namespace U3DTest;

TEST_CASE("FileSystem copies, renames, and recursively scans files", "[IO]")
{
    TestContext context;
    auto* fileSystem = context->GetSubsystem<FileSystem>();
    const String root = ScratchPath("io-tree");
    const String child = root + "/child";
    const String original = child + "/original.txt";
    const String copy = root + "/copy.txt";
    const String renamed = root + "/renamed.dat";

    fileSystem->Delete(original);
    fileSystem->Delete(copy);
    fileSystem->Delete(renamed);
    REQUIRE(fileSystem->CreateDir(root));
    REQUIRE(fileSystem->CreateDir(child));
    {
        File file(context, original, FILE_WRITE);
        REQUIRE(file.WriteString("payload"));
    }
    REQUIRE(fileSystem->Copy(original, copy));
    REQUIRE(fileSystem->FileExists(copy));
    REQUIRE(fileSystem->Rename(copy, renamed));
    REQUIRE_FALSE(fileSystem->FileExists(copy));
    REQUIRE(fileSystem->FileExists(renamed));

    Vector<String> files;
    fileSystem->ScanDir(files, root, "*.txt", SCAN_FILES, true);
    REQUIRE(files.Size() == 1);
    REQUIRE(files[0] == "child/original.txt");

    Vector<String> directories;
    fileSystem->ScanDir(directories, root, "*", SCAN_DIRS, false);
    REQUIRE(directories.Contains("child"));

    REQUIRE(fileSystem->Delete(original));
    REQUIRE(fileSystem->Delete(renamed));
}

TEST_CASE("FileSystem path helpers cover edge cases", "[IO]")
{
    REQUIRE(GetPath("file.txt").Empty());
    REQUIRE(GetFileName("archive.tar.gz") == "archive.tar");
    REQUIRE(GetFileNameAndExtension("path/archive.tar.gz") == "archive.tar.gz");
    REQUIRE(GetExtension("path.with.dot/file").Empty());
    REQUIRE(ReplaceExtension("file", ".txt") == "file.txt");
    REQUIRE(ReplaceExtension("file.old", "") == "file");
    REQUIRE(AddTrailingSlash("").Empty());
    REQUIRE(RemoveTrailingSlash("/").Empty());
    REQUIRE(GetParentPath("one/two/file.txt") == "one/two/");
    REQUIRE(GetInternalPath("one\\two\\") == "one/two/");
#ifndef _WIN32
    REQUIRE(GetNativePath("one/two") == "one/two");
#endif
}

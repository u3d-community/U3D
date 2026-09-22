#include "TestUtils.h"

#include <Urho3D/IO/File.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/IO/VectorBuffer.h>
#include <Urho3D/Resource/Image.h>
#include <Urho3D/Resource/ResourceCache.h>

using namespace Urho3D;
using namespace U3DTest;

namespace
{

SharedPtr<Image> MakeCorners(Context* context, unsigned components = 4)
{
    SharedPtr<Image> image(new Image(context));
    image->SetSize(2, 2, components);
    image->SetPixel(0, 0, Color::RED);
    image->SetPixel(1, 0, Color::GREEN);
    image->SetPixel(0, 1, Color::BLUE);
    image->SetPixel(1, 1, Color::WHITE);
    return image;
}

Image* LoadImage(const String& name)
{
    return HeadlessContext()->GetSubsystem<ResourceCache>()->GetResource<Image>(name);
}

}

TEST_CASE("Image loads a png from the data directory", "[Resource]")
{
    Image* image = LoadImage("Textures/Logo.png");
    REQUIRE(image != nullptr);
    REQUIRE(image->GetWidth() > 0);
    REQUIRE(image->GetHeight() > 0);
    REQUIRE(image->GetDepth() == 1);
    REQUIRE(image->GetComponents() >= 3);
    REQUIRE(image->GetData() != nullptr);
    REQUIRE_FALSE(image->IsCompressed());
    REQUIRE_FALSE(image->IsCubemap());
    REQUIRE_FALSE(image->IsArray());
    REQUIRE(image->GetMemoryUse() > 0);
}

TEST_CASE("Image loads a jpeg", "[Resource]")
{
    Image* image = LoadImage("Textures/Jack_face.jpg");
    REQUIRE(image != nullptr);
    REQUIRE(image->GetWidth() > 0);
    REQUIRE(image->GetComponents() == 3);
    REQUIRE_FALSE(image->HasAlphaChannel());
}

TEST_CASE("Image loads a compressed dds and reports its levels", "[Resource]")
{
    Image* image = LoadImage("Textures/Mushroom.dds");
    REQUIRE(image != nullptr);
    REQUIRE(image->IsCompressed());
    REQUIRE(image->GetCompressedFormat() != CF_NONE);
    REQUIRE(image->GetNumCompressedLevels() >= 1);
    REQUIRE(image->GetDepth() == 1);

    const CompressedLevel top = image->GetCompressedLevel(0);
    REQUIRE(top.data_ != nullptr);
    REQUIRE(top.width_ == image->GetWidth());
    REQUIRE(top.height_ == image->GetHeight());
    REQUIRE(top.dataSize_ > 0);
    REQUIRE(top.blockSize_ > 0);

    SECTION("an out of range level is empty")
    {
        const CompressedLevel missing = image->GetCompressedLevel(image->GetNumCompressedLevels() + 5);
        REQUIRE(missing.data_ == nullptr);
    }

    SECTION("smaller levels halve in size")
    {
        REQUIRE(image->GetNumCompressedLevels() > 1);

        const CompressedLevel second = image->GetCompressedLevel(1);
        REQUIRE(second.width_ == top.width_ / 2);
        REQUIRE(second.height_ == top.height_ / 2);
    }

    SECTION("the whole image can be decompressed to RGBA")
    {
        SharedPtr<Image> decompressed = image->GetDecompressedImage();
        REQUIRE(decompressed.NotNull());
        REQUIRE_FALSE(decompressed->IsCompressed());
        REQUIRE(decompressed->GetComponents() == 4);
        REQUIRE(decompressed->GetWidth() == image->GetWidth());
        REQUIRE(decompressed->GetHeight() == image->GetHeight());
    }

    SECTION("a single level decompresses into a caller supplied buffer")
    {
        CompressedLevel level = image->GetCompressedLevel(0);
        PODVector<unsigned char> rgba(level.width_ * level.height_ * 4);
        REQUIRE(level.Decompress(rgba.Buffer()));
        bool anySet = false;
        for (unsigned i = 0; i < rgba.Size(); ++i)
            anySet = anySet || rgba[i] != 0;
        REQUIRE(anySet);
    }

    SECTION("compressed data cannot be sampled or mipmapped, but it can be flipped")
    {
        REQUIRE(image->GetPixel(0, 0) == Color::BLACK);
        REQUIRE(image->GetNextLevel() == nullptr);

        SharedPtr<Image> before = image->GetDecompressedImage();

        REQUIRE(image->FlipHorizontal());
        SharedPtr<Image> flipped = image->GetDecompressedImage();

        bool anyDifference = false;
        const int width = before->GetWidth();
        for (int x = 0; x < width; ++x)
            anyDifference = anyDifference || !(flipped->GetPixel(x, 0) == before->GetPixel(x, 0));
        REQUIRE(anyDifference);

        REQUIRE(image->FlipHorizontal());
        SharedPtr<Image> restored = image->GetDecompressedImage();
        for (int x = 0; x < width; x += 37)
            REQUIRE(restored->GetPixel(x, 0) == before->GetPixel(x, 0));
    }
}

TEST_CASE("Image saves and reloads every uncompressed format", "[Resource]")
{
    Context* context = HeadlessContext();
    SharedPtr<Image> source = MakeCorners(context);

    struct Format
    {
        const char* name_;
        bool lossless_;
        bool keepsAlpha_;
    };

    const Format formats[] = {
        {"image-io.png", true, true},
        {"image-io.bmp", true, false},
        {"image-io.tga", true, true},
    };

    for (const Format& format : formats)
    {
        const String path = ScratchPath(format.name_);
        RemoveScratch(format.name_);

        const String extension = GetExtension(String(format.name_));
        if (extension == ".png")
            REQUIRE(source->SavePNG(path));
        else if (extension == ".bmp")
            REQUIRE(source->SaveBMP(path));
        else
            REQUIRE(source->SaveTGA(path));

        Image reloaded(context);
        File file(context, path, FILE_READ);
        REQUIRE(file.IsOpen());
        reloaded.SetName(path);
        REQUIRE(reloaded.Load(file));

        REQUIRE(reloaded.GetWidth() == 2);
        REQUIRE(reloaded.GetHeight() == 2);
        REQUIRE(reloaded.GetPixel(0, 0).Equals(Color::RED));
        REQUIRE(reloaded.GetPixel(1, 0).Equals(Color::GREEN));
        REQUIRE(reloaded.GetPixel(0, 1).Equals(Color::BLUE));

        RemoveScratch(format.name_);
    }
}

TEST_CASE("Image saves a jpeg whose quality affects the file size", "[Resource]")
{
    Context* context = HeadlessContext();
    SharedPtr<Image> image(new Image(context));
    REQUIRE(image->SetSize(64, 64, 3));

    for (int y = 0; y < 64; ++y)
        for (int x = 0; x < 64; ++x)
            image->SetPixel(x, y, Color(x / 64.0f, y / 64.0f, 0.5f));

    const String low = ScratchPath("image-io-low.jpg");
    const String high = ScratchPath("image-io-high.jpg");
    RemoveScratch("image-io-low.jpg");
    RemoveScratch("image-io-high.jpg");

    REQUIRE(image->SaveJPG(low, 10));
    REQUIRE(image->SaveJPG(high, 100));

    TestContext files;
    auto* fileSystem = files->GetSubsystem<FileSystem>();
    REQUIRE(fileSystem->FileExists(low));
    REQUIRE(fileSystem->FileExists(high));

    File lowFile(context, low, FILE_READ);
    File highFile(context, high, FILE_READ);
    REQUIRE(highFile.GetSize() > lowFile.GetSize());

    SECTION("the reloaded jpeg keeps the same dimensions")
    {
        Image reloaded(context);
        File source(context, high, FILE_READ);
        reloaded.SetName(high);
        REQUIRE(reloaded.Load(source));
        REQUIRE(reloaded.GetWidth() == 64);
        REQUIRE(reloaded.GetHeight() == 64);
    }

    RemoveScratch("image-io-low.jpg");
    RemoveScratch("image-io-high.jpg");
}

TEST_CASE("Image saves an uncompressed dds that loads back", "[Resource]")
{
    Context* context = HeadlessContext();
    SharedPtr<Image> source = MakeCorners(context);

    const String path = ScratchPath("image-io.dds");
    RemoveScratch("image-io.dds");
    REQUIRE(source->SaveDDS(path));

    Image reloaded(context);
    File file(context, path, FILE_READ);
    REQUIRE(file.IsOpen());
    reloaded.SetName(path);
    REQUIRE(reloaded.Load(file));

    REQUIRE(reloaded.GetWidth() == 2);
    REQUIRE(reloaded.GetHeight() == 2);
    REQUIRE(reloaded.GetDepth() == 1);
    REQUIRE(reloaded.GetComponents() == 4);

    REQUIRE(reloaded.IsCompressed());
    REQUIRE(reloaded.GetCompressedFormat() == CF_RGBA);
    REQUIRE(reloaded.GetPixel(0, 0) == Color::BLACK);

    SharedPtr<Image> decompressed = reloaded.GetDecompressedImage();
    REQUIRE(decompressed.NotNull());
    REQUIRE(decompressed->GetPixel(0, 0).Equals(Color::RED));
    REQUIRE(decompressed->GetPixel(1, 0).Equals(Color::GREEN));
    REQUIRE(decompressed->GetPixel(0, 1).Equals(Color::BLUE));

    RemoveScratch("image-io.dds");

    SECTION("a three component image cannot be written as dds")
    {
        SharedPtr<Image> rgb = MakeCorners(context, 3);
        REQUIRE_FALSE(rgb->SaveDDS(ScratchPath("image-io-rgb.dds")));
    }
}

TEST_CASE("Image writes itself through the generic serializer", "[Resource]")
{
    Context* context = HeadlessContext();
    SharedPtr<Image> source = MakeCorners(context);

    VectorBuffer buffer;
    REQUIRE(source->Save(buffer));
    REQUIRE(buffer.GetSize() > 0);

    buffer.Seek(0);
    Image reloaded(context);
    reloaded.SetName("saved.png");
    REQUIRE(reloaded.Load(buffer));
    REQUIRE(reloaded.GetWidth() == 2);
    REQUIRE(reloaded.GetPixel(1, 1).Equals(Color::WHITE));

    SECTION("an empty image has nothing to write")
    {
        Image empty(context);
        VectorBuffer target;
        REQUIRE_FALSE(empty.Save(target));
    }
}

TEST_CASE("Image rejects data that is not an image", "[Resource]")
{
    Context* context = HeadlessContext();
    Image image(context);

    const String rubbish("this is not an image");
    VectorBuffer buffer(rubbish.CString(), rubbish.Length());
    image.SetName("rubbish.png");
    REQUIRE_FALSE(image.Load(buffer));

    SECTION("a truncated dds header is refused")
    {
        Image dds(context);
        VectorBuffer shortHeader("DDS ", 4);
        dds.SetName("short.dds");
        REQUIRE_FALSE(dds.Load(shortHeader));
    }

    SECTION("a header that claims a format it does not hold is refused")
    {
        Image ktx(context);
        VectorBuffer shortHeader("\xABKTX 11\xBB\r\n\x1A\n", 12);
        ktx.SetName("short.ktx");
        REQUIRE_FALSE(ktx.Load(shortHeader));

        Image pvr(context);
        VectorBuffer pvrHeader("PVR\x03", 4);
        pvr.SetName("short.pvr");
        REQUIRE_FALSE(pvr.Load(pvrHeader));
    }
}

TEST_CASE("Image resizes with bilinear filtering", "[Resource]")
{
    Context* context = HeadlessContext();
    SharedPtr<Image> image(new Image(context));
    REQUIRE(image->SetSize(4, 4, 4));
    image->Clear(Color::BLACK);
    for (int x = 0; x < 4; ++x)
        image->SetPixel(x, 0, Color::WHITE);

    REQUIRE(image->Resize(8, 8));
    REQUIRE(image->GetWidth() == 8);
    REQUIRE(image->GetHeight() == 8);
    REQUIRE(image->GetComponents() == 4);
    REQUIRE(image->GetPixel(0, 0).r_ > 0.5f);
    REQUIRE(image->GetPixel(0, 7).r_ < 0.5f);

    SECTION("shrinking works as well")
    {
        REQUIRE(image->Resize(2, 2));
        REQUIRE(image->GetWidth() == 2);
        REQUIRE(image->GetHeight() == 2);
    }

    SECTION("a resize to nothing is refused")
    {
        REQUIRE_FALSE(image->Resize(0, 8));
        REQUIRE(image->GetWidth() == 8);
    }

    SECTION("a compressed image cannot be resized")
    {
        Image* compressed = LoadImage("Textures/Mushroom.dds");
        REQUIRE(compressed != nullptr);
        REQUIRE_FALSE(compressed->Resize(4, 4));
    }
}

TEST_CASE("Image builds and caches its mip chain", "[Resource]")
{
    Context* context = HeadlessContext();
    SharedPtr<Image> image(new Image(context));
    REQUIRE(image->SetSize(8, 8, 4));
    image->Clear(Color::GRAY);

    image->PrecalculateLevels();

    PODVector<Image*> levels;
    image->GetLevels(levels);
    REQUIRE(levels.Size() == 4);
    REQUIRE(levels[0] == image);
    REQUIRE(levels[1]->GetWidth() == 4);
    REQUIRE(levels[2]->GetWidth() == 2);
    REQUIRE(levels[3]->GetWidth() == 1);

    for (unsigned i = 1; i < levels.Size(); ++i)
        REQUIRE_NEAR(levels[i]->GetPixel(0, 0).r_, Color::GRAY.r_, 1.0f / 255.0f);

    SECTION("the const overload walks the same chain")
    {
        const Image* constImage = image;
        PODVector<const Image*> constLevels;
        constImage->GetLevels(constLevels);
        REQUIRE(constLevels.Size() == 4);
    }

    SECTION("cleaning up drops the cached levels")
    {
        image->CleanupLevels();
        PODVector<Image*> afterCleanup;
        image->GetLevels(afterCleanup);
        REQUIRE(afterCleanup.Size() == 1);
    }

    SECTION("resizing invalidates the cached chain")
    {
        REQUIRE(image->Resize(4, 4));
        PODVector<Image*> afterResize;
        image->GetLevels(afterResize);
        REQUIRE(afterResize.Size() == 1);
    }
}

TEST_CASE("Image sets raw pixel data in bulk", "[Resource]")
{
    Context* context = HeadlessContext();
    SharedPtr<Image> image(new Image(context));
    REQUIRE(image->SetSize(2, 1, 4));

    const unsigned char pixels[] = {
        255, 0, 0, 255,
        0, 255, 0, 128,
    };
    image->SetData(pixels);

    REQUIRE(image->GetPixel(0, 0).Equals(Color::RED));
    REQUIRE_NEAR(image->GetPixel(1, 0).g_, 1.0f, 0.01f);
    REQUIRE_NEAR(image->GetPixel(1, 0).a_, 128.0f / 255.0f, 0.01f);
    REQUIRE(image->HasAlphaChannel());

    SECTION("a null pointer clears the image to zero")
    {
        image->SetData(nullptr);
        REQUIRE(image->GetPixel(0, 0).Equals(Color(0.0f, 0.0f, 0.0f, 0.0f)));
    }

    SECTION("three component data has no alpha channel")
    {
        SharedPtr<Image> rgb(new Image(context));
        REQUIRE(rgb->SetSize(1, 1, 3));
        REQUIRE_FALSE(rgb->HasAlphaChannel());
    }
}

TEST_CASE("Image loads a colour lookup table into a volume", "[Resource]")
{
    Context* context = HeadlessContext();
    SharedPtr<Image> strip(new Image(context));

    REQUIRE(strip->SetSize(COLOR_LUT_SIZE * COLOR_LUT_SIZE, COLOR_LUT_SIZE, 3));
    strip->Clear(Color::RED);

    const String path = ScratchPath("image-io-lut.png");
    RemoveScratch("image-io-lut.png");
    REQUIRE(strip->SavePNG(path));

    Image lut(context);
    File file(context, path, FILE_READ);
    REQUIRE(file.IsOpen());
    lut.SetName(path);
    REQUIRE(lut.LoadColorLUT(file));

    REQUIRE(lut.GetWidth() == COLOR_LUT_SIZE);
    REQUIRE(lut.GetHeight() == COLOR_LUT_SIZE);
    REQUIRE(lut.GetDepth() == COLOR_LUT_SIZE);
    REQUIRE(lut.GetComponents() == 3);
    REQUIRE(lut.GetPixel(0, 0, 0).Equals(Color::RED));

    RemoveScratch("image-io-lut.png");

    SECTION("a compressed source is refused")
    {
        File source(context, ResourcePath("Data/Textures/Mushroom.dds"), FILE_READ);
        REQUIRE(source.IsOpen());

        Image bad(context);
        bad.SetName("Mushroom.dds");
        REQUIRE_FALSE(bad.LoadColorLUT(source));
    }

    SECTION("a four component source is refused")
    {
        SharedPtr<Image> rgba(new Image(context));
        REQUIRE(rgba->SetSize(COLOR_LUT_SIZE * COLOR_LUT_SIZE, COLOR_LUT_SIZE, 4));
        rgba->Clear(Color::RED);

        const String rgbaPath = ScratchPath("image-io-lut-rgba.png");
        RemoveScratch("image-io-lut-rgba.png");
        REQUIRE(rgba->SavePNG(rgbaPath));

        Image bad(context);
        File source(context, rgbaPath, FILE_READ);
        bad.SetName(rgbaPath);
        REQUIRE_FALSE(bad.LoadColorLUT(source));

        RemoveScratch("image-io-lut-rgba.png");
    }
}

TEST_CASE("Image loads a single cube map face as a plain 2D image", "[Resource]")
{
    Image* face = LoadImage("Textures/PartlyCloudy_PosX.png");
    REQUIRE(face != nullptr);
    REQUIRE(face->GetWidth() > 0);
    REQUIRE_FALSE(face->IsCubemap());

    SECTION("a subimage of a face is an independent copy")
    {
        SharedPtr<Image> quarter(face->GetSubimage(IntRect(0, 0, face->GetWidth() / 2, face->GetHeight() / 2)));
        REQUIRE(quarter.NotNull());
        REQUIRE(quarter->GetWidth() == face->GetWidth() / 2);
        REQUIRE(quarter->GetComponents() == face->GetComponents());
    }
}

TEST_CASE("Image handles a volume of pixels", "[Resource]")
{
    Context* context = HeadlessContext();
    SharedPtr<Image> volume(new Image(context));
    REQUIRE(volume->SetSize(4, 4, 4, 4));
    REQUIRE(volume->GetDepth() == 4);

    volume->Clear(Color::BLACK);
    volume->SetPixel(2, 2, 2, Color::WHITE);
    REQUIRE(volume->GetPixel(2, 2, 2).Equals(Color::WHITE));
    REQUIRE(volume->GetPixel(0, 0, 0).Equals(Color::BLACK));

    REQUIRE(volume->GetPixel(0, 0, 99).Equals(Color::BLACK));
    REQUIRE(volume->GetPixel(0, 0, -1).Equals(Color::BLACK));

    SECTION("a mip level of a volume halves every axis")
    {
        SharedPtr<Image> level = volume->GetNextLevel();
        REQUIRE(level->GetWidth() == 2);
        REQUIRE(level->GetHeight() == 2);
        REQUIRE(level->GetDepth() == 2);
    }

    SECTION("a subimage cannot be taken from a volume")
    {
        REQUIRE(volume->GetSubimage(IntRect(0, 0, 2, 2)) == nullptr);
    }
}

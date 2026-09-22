#include "TestUtils.h"

#include <Urho3D/Resource/Decompress.h>
#include <Urho3D/Resource/Image.h>

#include <cstring>

using namespace Urho3D;
using namespace U3DTest;

TEST_CASE("Image validates dimensions and provides pixel operations", "[Resource]")
{
    TestContext context;
    Image image(context);

    REQUIRE_FALSE(image.SetSize(0, 2, 4));
    REQUIRE_FALSE(image.SetSize(2, -1, 4));
    REQUIRE(image.SetSize(2, 2, 0));
    REQUIRE_FALSE(image.SetSize(2, 2, 5));
    REQUIRE(image.SetSize(2, 2, 4));
    REQUIRE(image.GetWidth() == 2);
    REQUIRE(image.GetHeight() == 2);
    REQUIRE(image.GetDepth() == 1);
    REQUIRE(image.HasAlphaChannel());

    image.Clear(Color::BLACK);
    image.SetPixel(0, 0, Color::RED);
    image.SetPixel(1, 0, Color::GREEN);
    image.SetPixel(0, 1, Color::BLUE);
    image.SetPixel(1, 1, Color::WHITE);
    REQUIRE(image.GetPixel(0, 0).Equals(Color::RED));
    REQUIRE(image.GetPixel(1, 1).Equals(Color::WHITE));

    SECTION("integer pixels use R in the least significant byte")
    {
        image.SetPixelInt(0, 0, 0x44332211u);
        REQUIRE(image.GetPixelInt(0, 0) == 0x44332211u);
    }

    SECTION("flips move pixels along the requested axis")
    {
        REQUIRE(image.FlipHorizontal());
        REQUIRE(image.GetPixel(0, 0).Equals(Color::GREEN));
        REQUIRE(image.GetPixel(1, 1).Equals(Color::BLUE));
        REQUIRE(image.FlipVertical());
        REQUIRE(image.GetPixel(0, 0).Equals(Color::WHITE));
    }

    SECTION("subimages can be extracted and copied")
    {
        SharedPtr<Image> sub(image.GetSubimage(IntRect(1, 0, 2, 2)));
        REQUIRE(sub.NotNull());
        REQUIRE(sub->GetWidth() == 1);
        REQUIRE(sub->GetPixel(0, 0).Equals(Color::GREEN));
        REQUIRE(sub->GetPixel(0, 1).Equals(Color::WHITE));
        REQUIRE(image.GetSubimage(IntRect(-1, 0, 1, 1)) == nullptr);

        Image destination(context);
        REQUIRE(destination.SetSize(4, 4, 4));
        destination.Clear(Color::BLACK);
        REQUIRE(destination.SetSubimage(sub, IntRect(1, 1, 3, 3)));
        REQUIRE(destination.GetPixel(1, 1).Equals(Color::GREEN));
        REQUIRE(destination.GetPixel(2, 2).Equals(Color::WHITE));
        REQUIRE_FALSE(destination.SetSubimage(sub, IntRect(-1, 0, 1, 1)));
        REQUIRE_FALSE(destination.SetSubimage(sub, IntRect(0, -1, 1, 1)));
        REQUIRE_FALSE(destination.SetSubimage(sub, IntRect(3, 0, 5, 1)));
        REQUIRE_FALSE(destination.SetSubimage(sub, IntRect(0, 3, 1, 5)));
        REQUIRE_FALSE(destination.SetSubimage(sub, IntRect(0, 0, 0, 1)));
    }

    SECTION("mipmap and RGBA conversion preserve useful content")
    {
        SharedPtr<Image> level = image.GetNextLevel();
        REQUIRE(level->GetWidth() == 1);
        REQUIRE(level->GetHeight() == 1);
        REQUIRE(level->GetComponents() == 4);
        const Color average = level->GetPixel(0, 0);
        REQUIRE_NEAR(average.r_, 0.5f, 0.01f);
        REQUIRE_NEAR(average.g_, 0.5f, 0.01f);
        REQUIRE_NEAR(average.b_, 0.5f, 0.01f);

        Image rgb(context);
        REQUIRE(rgb.SetSize(1, 1, 3));
        rgb.SetPixel(0, 0, Color(0.25f, 0.5f, 0.75f));
        SharedPtr<Image> rgba = rgb.ConvertToRGBA();
        REQUIRE(rgba->GetComponents() == 4);
        REQUIRE_EQ_F(rgba->GetPixel(0, 0).a_, 1.0f);
    }
}

TEST_CASE("Image mipmaps average nonuniform 1D and 3D pixels", "[Resource]")
{
    TestContext context;
    Image line(context);
    REQUIRE(line.SetSize(4, 1, 3));
    line.SetPixel(0, 0, Color(0.0f, 0.1f, 0.2f));
    line.SetPixel(1, 0, Color(0.2f, 0.3f, 0.4f));
    line.SetPixel(2, 0, Color(0.6f, 0.7f, 0.8f));
    line.SetPixel(3, 0, Color(1.0f, 0.9f, 0.8f));
    SharedPtr<Image> lineMip = line.GetNextLevel();
    REQUIRE(lineMip->GetWidth() == 2);
    REQUIRE_NEAR(lineMip->GetPixel(0, 0).r_, 0.1f, 0.01f);
    REQUIRE_NEAR(lineMip->GetPixel(1, 0).r_, 0.8f, 0.01f);

    Image volume(context);
    REQUIRE(volume.SetSize(2, 2, 2, 4));
    volume.Clear(Color::BLACK);
    volume.SetPixel(1, 1, 1, Color::WHITE);
    SharedPtr<Image> volumeMip = volume.GetNextLevel();
    REQUIRE(volumeMip->GetWidth() == 1);
    REQUIRE(volumeMip->GetHeight() == 1);
    REQUIRE(volumeMip->GetDepth() == 1);
    REQUIRE_NEAR(volumeMip->GetPixel(0, 0).r_, 0.125f, 0.01f);
}

TEST_CASE("Image supports 3D sampling", "[Resource]")
{
    TestContext context;
    Image image(context);
    REQUIRE(image.SetSize(2, 2, 2, 1));
    image.ClearInt(0);
    image.SetPixelInt(1, 1, 1, 255);
    REQUIRE(image.GetPixelInt(1, 1, 1) == 0xffffffffu);
    REQUIRE_NEAR(image.GetPixelTrilinear(0.5f, 0.5f, 0.5f).r_, 0.125f, 0.001f);
    REQUIRE_EQ_F(image.GetPixelTrilinear(1.0f, 1.0f, 1.0f).r_, 1.0f);
    REQUIRE_FALSE(image.FlipHorizontal());
    REQUIRE_FALSE(image.FlipVertical());
}

TEST_CASE("Compressed DXT blocks decompress and flip deterministically", "[Resource]")
{
    const unsigned char block[16] = {0x00, 0xf8, 0x00, 0xf8, 0, 0, 0, 0, 0xff, 0xff, 0xff, 0xff, 0, 0, 0, 0};
    unsigned char rgba[4 * 4 * 4]{};
    DecompressImageDXT(rgba, block, 4, 4, 1, CF_DXT1);
    REQUIRE(rgba[0] == 255);
    REQUIRE(rgba[1] == 0);
    REQUIRE(rgba[2] == 0);
    REQUIRE(rgba[3] == 255);

    for (CompressedFormat format : {CF_DXT1, CF_DXT3, CF_DXT5})
    {
        const unsigned size = format == CF_DXT1 ? 8 : 16;
        unsigned char once[16]{};
        unsigned char twice[16]{};
        FlipBlockVertical(once, block, format);
        FlipBlockVertical(twice, once, format);
        REQUIRE(std::memcmp(block, twice, size) == 0);
        FlipBlockHorizontal(once, block, format);
        FlipBlockHorizontal(twice, once, format);
        REQUIRE(std::memcmp(block, twice, size) == 0);
    }
}

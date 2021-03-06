#pragma once

namespace ImageProcessing
{

#pragma pack(push)
#pragma pack(1)
struct Color_rgb
{
    // Empty default constructor is used to prevent zero-init when creating a std::vector<Color_rgb>.
    // These vectors are used as write targets for reads from disk, where zero-init would be inefficient.
    explicit Color_rgb() {}
    Color_rgb(uint8_t r, uint8_t g, uint8_t b) : red(r), green(g), blue(b) {}

    uint8_t red;
    uint8_t green;
    uint8_t blue;
};
#pragma pack(pop)

struct Bitmap
{
    std::vector<uint8_t> bitmap;
    unsigned int width;
    unsigned int height;
    bool filtered;
};

Bitmap resize_bitmap_point_sampled(const Bitmap& unscaled_bitmap, unsigned int scaled_width, unsigned int scaled_height);

}


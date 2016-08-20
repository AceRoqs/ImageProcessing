#include "PreCompile.h"
#include "Bitmap.h"         // Pick up forward declarations to ensure correctness.
#include "pcx.h"
#include "targa.h"

namespace ImageProcessing
{

// This code is fine, but it is currently unused.
#if 0
static void generate_grid_texture_rgb(
    _Out_writes_(width * height) uint8_t* bitmap,
    unsigned int width,
    unsigned int height) noexcept
{
    for(unsigned int iy = 0; iy < height; ++iy)
    {
        for(unsigned int ix = 0; ix < width; ++ix)
        {
            if((ix < (width / 2)) ^ (iy >= (height / 2)))
            {
                bitmap[0] = 0xc0;
                bitmap[1] = 0xc0;
                bitmap[2] = 0xc0;
            }
            else
            {
                bitmap[0] = 0xff;
                bitmap[1] = 0xff;
                bitmap[2] = 0xff;
            }

            bitmap += 3;
        }
    }
}
#endif

// Resamples and scales an image using a nearest neighbor algorithm.
static void resize_bitmap_point_sampled_unchecked(const Color_rgb* unscaled_pixels, unsigned int unscaled_width, unsigned int unscaled_height,
                                                  Color_rgb* scaled_pixels, unsigned int scaled_width, unsigned int scaled_height) noexcept
{
    for(unsigned int scaled_y = 0; scaled_y < scaled_height; ++scaled_y)
    {
        for(unsigned int scaled_x = 0; scaled_x < scaled_width; ++scaled_x)
        {
            unsigned int unscaled_x = unscaled_width * scaled_x / scaled_width;
            unsigned int unscaled_y = unscaled_height * scaled_y / scaled_height;

            assert(unscaled_x < unscaled_width);
            assert(unscaled_y < unscaled_height);

            const Color_rgb color = unscaled_pixels[unscaled_y * unscaled_width + unscaled_x];
            scaled_pixels[scaled_y * scaled_width + scaled_x] = color;
        }
    }
}

Bitmap resize_bitmap_point_sampled(const Bitmap& unscaled_bitmap, unsigned int scaled_width, unsigned int scaled_height)
{
    Bitmap scaled_bitmap{std::vector<uint8_t>(scaled_width * scaled_height * sizeof(Color_rgb)), scaled_width, scaled_height, true};

    auto unscaled_pixels = reinterpret_cast<const Color_rgb*>(&unscaled_bitmap.bitmap[0]);
    auto scaled_pixels = reinterpret_cast<Color_rgb*>(&scaled_bitmap.bitmap[0]);

    resize_bitmap_point_sampled_unchecked(unscaled_pixels, unscaled_bitmap.width, unscaled_bitmap.height, scaled_pixels, scaled_width, scaled_height);

    return scaled_bitmap;
}

}


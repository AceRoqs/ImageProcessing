#include "PreCompile.h"
#include "Bitmap.h"         // Pick up forward declarations to ensure correctness.
#include "pcx.h"
#include "targa.h"

namespace ImageProcessing
{

// This code is fine, but it is currently unused.
#if 0
static void generate_grid_texture_rgb(
    _Out_writes_(xsize * ysize) uint8_t* bitmap,
    unsigned int xsize,
    unsigned int ysize) noexcept
{
    for(unsigned int iy = 0; iy < ysize; ++iy)
    {
        for(unsigned int ix = 0; ix < xsize; ++ix)
        {
            if((ix < (xsize / 2)) ^ (iy >= (ysize / 2)))
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

#if 0
Bitmap resize_bitmap(const Bitmap& unscaled_bitmap, unsigned int xsize, unsigned int ysize)
{
    Bitmap scaled_bitmap{std::vector<uint8_t>(xsize * ysize * sizeof(Color_rgb)), xsize, ysize, true};

    for(unsigned int y_index = 0; y_index < ysize; ++y_index)
    {
        for(unsigned int x_index = 0; x_index < xsize; )
        {
            Color_rgb color;

            // TODO: 2016: Point sample the unscaled_bitmap.
            (void)unscaled_bitmap;
            color.red = 255;
            color.green = 255;
            color.blue = 0;

            scaled_bitmap.bitmap[y_index * ysize + x_index + 0] = color.red;
            scaled_bitmap.bitmap[y_index * ysize + x_index + 1] = color.green;
            scaled_bitmap.bitmap[y_index * ysize + x_index + 2] = color.blue;
            x_index += sizeof(Color_rgb);
        }
    }

    return scaled_bitmap;
}
#endif

}


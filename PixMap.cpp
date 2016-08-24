#include "PreCompile.h"
//#include "PixMap.h"
#include "Bitmap.h"

// Portable PixMap specs:
// https://en.wikipedia.org/wiki/Netpbm_format
namespace ImageProcessing
{
static const uint8_t* skip_whitespace(const uint8_t* scan_location, size_t size)
{
    (void)size;

    return scan_location;
}


Bitmap decode_bitmap_from_pixmap_memory(_In_count_(size) const uint8_t* pixmap_memory, size_t size)
{
    (void)pixmap_memory;
    (void)size;

    const uint8_t* scan_location = pixmap_memory;
    scan_location = skip_whitespace(scan_location, size);

    Bitmap bitmap{};
    return bitmap;
}

}


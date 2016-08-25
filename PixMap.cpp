#include "PreCompile.h"
//#include "PixMap.h"
#include "Bitmap.h"

// Portable PixMap specs:
// https://en.wikipedia.org/wiki/Netpbm_format
namespace ImageProcessing
{
static bool is_ascii_whitespace_character(char ch) noexcept
{
    const char whitespace[] = { u8'\t', u8'\n', u8'\v', u8'\f', u8'\r', u8' '};
    for(auto ix = 0u; ix < sizeof(whitespace); ++ix)
    {
        if(whitespace[ix] == ch)
        {
            return true;
        }
    }

    return false;
}

static const uint8_t* skip_whitespace(_In_reads_(size) const uint8_t* scan_location, size_t size)
{
    for(size_t ix = 0; ix < size; ++ix)
    {
        if(is_ascii_whitespace_character(*scan_location))
        {
            ++scan_location;
        }
        else
        {
            break;
        }
    }

    return scan_location;
}


Bitmap decode_bitmap_from_pixmap_memory(_In_reads_(size) const uint8_t* pixmap_memory, size_t size)
{
    (void)pixmap_memory;
    (void)size;

    const uint8_t* scan_location = pixmap_memory;
    scan_location = skip_whitespace(scan_location, size);

    Bitmap bitmap{};
    return bitmap;
}

}


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

static size_t count_whitespace(_In_reads_(size) const uint8_t* scan_location, size_t size) noexcept
{
    size_t count = 0;

    for(size_t ix = 0; ix < size; ++ix)
    {
        if(is_ascii_whitespace_character(*scan_location))
        {
            ++count;
            ++scan_location;
        }
        else
        {
            break;
        }
    }

    return count;
}

// TODO: 2016: May be better to have a tokenizer class that holds the memory state?
static std::string read_token(_In_reads_(size) const uint8_t* scan_location, size_t size)
{
    (void)scan_location;
    (void)size;
    std::string token;
    return token;
}

Bitmap decode_bitmap_from_pixmap_memory(_In_reads_(size) const uint8_t* pixmap_memory, size_t size)
{
    (void)pixmap_memory;
    (void)size;

    const uint8_t* scan_location = pixmap_memory;
    scan_location += count_whitespace(scan_location, size);
    size -= (scan_location - pixmap_memory);
    read_token(scan_location, size);
    //CHECK_EXCEPTION(size > 

    //CHECK_EXCEPTION(size >= sizeof(TGA_header), u8"Image data is invalid.");

    Bitmap bitmap{};
    return bitmap;
}

}


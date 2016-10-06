#include "PreCompile.h"
#include "pcx.h"                // Pick up forward declarations to ensure correctness.
#include "Bitmap.h"
#include <PortableRuntime/CheckException.h>

// PCX spec:
// http://www.fileformat.info/format/pcx/egff.htm
namespace ImageProcessing
{

enum class PCX_manufacturer : uint8_t { PCX_magic = 10 };
enum class PCX_version : uint8_t
{
    PC_Paintbrush_2_5 = 0,          // PC Paintbrush 2.5.
    PC_Paintbrush_2_8_palette = 2,  // PC Paintbrush 2.8 with palette info.
    PC_Paintbrush_2_8 = 3,          // PC Paintbrush 2.8 without palette info.
    PC_Paintbrush_Windows = 4,      // PC Paintbrush for Windows.
    PC_Paintbrush_3 = 5,            // PC Paintbrush 3.0+.
};
enum class PCX_encoding : uint8_t { RLE_encoding = 1 };

#pragma pack(push)
#pragma pack(1)
struct PCX_header
{
    PCX_manufacturer manufacturer;      // PCX_manufacturer.
    PCX_version version;                // PCX_version.
    PCX_encoding encoding;              // PCX_encoding.
    uint8_t  bits_per_pixel;            // Bits per pixel per plane - 1, 2, 4, or 8.
    uint16_t min_x;                     // Minimum X value - usually zero.
    uint16_t min_y;                     // Minimum Y value - usually zero.
    uint16_t max_x;                     // Maximum X value.
    uint16_t max_y;                     // Maximum Y value.
    uint16_t horizontal_dpi;            // Horizontal resolution in dots per inch.
    uint16_t vertical_dpi;              // Vertical resolution in dots per inch.
    uint8_t  color_map[48];             // EGA color palette.
    uint8_t  reserved;                  // Should be zero.
    uint8_t  color_plane_count;         // Number of color planes.
    uint16_t bytes_per_line;            // bytes per scan line plane, must be even.
    uint16_t palette_info;              // 1 = Color/BW, 2 = Grayscale.
    uint16_t horizontal_screen_size;    // Horizontal screen size in pixels.
    uint16_t vertical_screen_size;      // Vertical screen size in pixels.
    uint8_t  filler[54];                // Padding to 128 bytes.
};
#pragma pack(pop)

static void validate_pcx_header(_In_ const PCX_header* header)
{
    CHECK_EXCEPTION(header->manufacturer == PCX_manufacturer::PCX_magic, u8"Image data is invalid.");
    CHECK_EXCEPTION(header->encoding == PCX_encoding::RLE_encoding, u8"Image data is invalid.");
    CHECK_EXCEPTION(header->min_x < header->max_x, u8"Image data is invalid.");
    CHECK_EXCEPTION(header->min_y < header->max_y, u8"Image data is invalid.");
    CHECK_EXCEPTION((header->color_plane_count == 1) || (header->color_plane_count == 3), u8"Image data is invalid.");
    CHECK_EXCEPTION(header->bits_per_pixel == 8, u8"Image data is invalid.");
}

static const uint8_t* rle_decode(
    _In_ const uint8_t* start_iterator,
    _In_ const uint8_t* end_iterator,
    _Out_ uint8_t* value,
    _Out_ uint8_t* run_count)
{
    CHECK_EXCEPTION(start_iterator < end_iterator, u8"Image data is invalid.");

    *run_count = 1;
    if(*start_iterator >= 192)
    {
        *run_count = *start_iterator - 192;
        ++start_iterator;

        CHECK_EXCEPTION(start_iterator < end_iterator, u8"Image data is invalid.");
    }

    *value = *start_iterator;
    ++start_iterator;
    return start_iterator;
}

static void rle_decode_fill(
    _In_reads_to_ptr_(end_iterator) const uint8_t* start_iterator,
    const uint8_t* end_iterator,
    _Out_writes_to_ptr_(output_end_iterator) uint8_t* output_start_iterator,
    uint8_t* output_end_iterator,
    const std::function<uint8_t* (uint8_t*, uint8_t*, uint8_t, uint8_t)>& fill_buffer)
{
    do
    {
        uint8_t value;
        uint8_t run_count;
        start_iterator = rle_decode(start_iterator, end_iterator, &value, &run_count);

        output_start_iterator = fill_buffer(output_start_iterator, output_end_iterator, value, run_count);
    } while(output_start_iterator < output_end_iterator);

    assert(output_start_iterator == output_end_iterator);
}

static void pcx_decode(
    _In_reads_to_ptr_(end_iterator) const uint8_t* start_iterator,
    const uint8_t* end_iterator,
    _Out_writes_to_ptr_(bitmap_end_iterator) uint8_t* bitmap_start_iterator,
    uint8_t* bitmap_end_iterator,
    _In_reads_opt_(256) const Color_rgb* palette)
{
    // MSVC complains that fill_n is insecure.
    // _SCL_SECURE_NO_WARNINGS or checked iterator required.
    // MSVC 10 also does not implement fill_n's C++11 return value.
    // TODO: 2014: Revisit this in a future compiler.
    const std::function<uint8_t* (uint8_t*, uint8_t*, uint8_t, uint8_t)> fill_palette =
        [palette](uint8_t* output_start_iterator, uint8_t* output_end_iterator, uint8_t value, uint8_t run_count) -> uint8_t*
        {
            CHECK_EXCEPTION(output_start_iterator + (run_count * sizeof(Color_rgb)) <= output_end_iterator, u8"Image data is invalid.");
            std::fill_n(reinterpret_cast<Color_rgb*>(output_start_iterator), run_count, palette[value]);
            return output_start_iterator + (run_count * sizeof(palette[0]));
        };

    const std::function<uint8_t* (uint8_t*, uint8_t*, uint8_t, uint8_t)> fill_no_palette =
        [](uint8_t* output_start_iterator, uint8_t* output_end_iterator, uint8_t value, uint8_t run_count) -> uint8_t*
        {
            CHECK_EXCEPTION(output_start_iterator + run_count <= output_end_iterator, u8"Image data is invalid.");
            std::fill_n(output_start_iterator, run_count, value);
            return output_start_iterator + run_count;
        };

    const auto fill_buffer = palette != nullptr ? fill_palette : fill_no_palette;
    rle_decode_fill(start_iterator, end_iterator,
                    bitmap_start_iterator, bitmap_end_iterator,
                    fill_buffer);
}

// This function is case sensitive due to the lack of library support for
// UTF-8 case insensitive matching.
// TODO: Consider case insensitive for ASCII subset of UTF-8.
// TODO: 2016: There are many copies of this function.  Find a place to put file functions.
static bool file_has_extension_case_sensitive(_In_z_ const char* file_name, _In_z_ const char* extension) noexcept
{
    const size_t length_file = strlen(file_name);
    const size_t length_extension = strlen(extension);
    return ((length_file >= length_extension) && (strcmp(file_name + length_file - length_extension, extension) == 0));
}

bool is_pcx_file_name(_In_z_ const char* file_name)
{
    return file_has_extension_case_sensitive(file_name, ".pcx");
}

Bitmap decode_bitmap_from_pcx_memory(_In_reads_(size) const uint8_t* pcx_memory, size_t size)
{
    CHECK_EXCEPTION(size >= sizeof(PCX_header), u8"Image data is invalid.");

    const PCX_header* header = reinterpret_cast<const PCX_header*>(pcx_memory);
    validate_pcx_header(header);

    const Color_rgb* palette = nullptr;
    if(header->version == PCX_version::PC_Paintbrush_3 && header->color_plane_count == 1)
    {
        // Add space for palette + C0 marker byte.
        CHECK_EXCEPTION(size >= sizeof(PCX_header) + sizeof(Color_rgb) * 256 + 1, u8"Image data is invalid.");

        palette = reinterpret_cast<const Color_rgb*>(pcx_memory + size - sizeof(Color_rgb) * 256);

        // Validate 0C byte.  Some documentation incorrectly says this byte is C0 instead of 0C.
        CHECK_EXCEPTION(reinterpret_cast<const uint8_t*>(palette)[-1] == 0x0c, u8"Image data is invalid.");
    }

    const auto image_width = static_cast<unsigned int>(header->max_x) - header->min_x + 1;
    const auto image_height = static_cast<unsigned int>(header->max_y) - header->min_y + 1;
    Bitmap bitmap{std::vector<uint8_t>(image_width * image_height * sizeof(Color_rgb)), image_width, image_height, true};

    const uint8_t* start_iterator = pcx_memory + sizeof(PCX_header);
    const uint8_t* end_iterator = palette != nullptr ? reinterpret_cast<const uint8_t*>(palette) - 1 : start_iterator + size;

    pcx_decode(start_iterator, end_iterator, bitmap.bitmap.data(), bitmap.bitmap.data() + bitmap.bitmap.size(), palette);

    // Return value optimization expected.
    return bitmap;
}

}


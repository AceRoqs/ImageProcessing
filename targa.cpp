#include "PreCompile.h"
#include "targa.h"              // Pick up forward declarations to ensure correctness.
#include "Bitmap.h"
#include "FileExtensionTest.h"
#include <PortableRuntime/CheckException.h>
#include <PortableRuntime/Tracing.h>

// Targa spec:
// http://www.dca.fee.unicamp.br/~martino/disciplinas/ea978/tgaffs.pdf
// http://www.fileformat.info/format/tga/egff.htm
namespace ImageProcessing
{

const unsigned int max_dimension = 16384;

enum class TGA_color_map : uint8_t
{
    Has_no_color_map = 0,
    Has_color_map = 1,
};

enum class TGA_image_type : uint8_t
{
    No_image_data = 0,
    Color_mapped = 1,
    True_color = 2,
    Black_and_white = 3,
    RLE_color_mapped = 9,
    RLE_true_color = 10,
    RLE_black_and_white = 11,
};

enum class TGA_alpha_type : uint8_t
{
    No_alpha = 0,                       // No alpha data exists.
    Ignorable_alpha = 1,                // Alpha data can be ignored.
    Retained_alpha = 2,                 // Alpha data is undefined, but should be retained.
    Alpha_exists = 3,                   // Alpha data exists.
    Premultiplied_alpha = 4,            // Alpha is pre-multiplied.
};

#pragma pack(push)
#pragma pack(1)

struct TGA_header
{
    uint8_t        id_length;                   // Length of the image ID field.
    TGA_color_map  color_map_type;              // TGA_color_map.
    TGA_image_type image_type;                  // TGA_image_type.
    uint16_t       color_map_first_index;       // Offset into color map table.
    uint16_t       color_map_length;            // Number of entries.
    uint8_t        color_map_bits_per_pixel;    // Number of bits per pixel.
    uint16_t       x_origin;                    // Lower-left corner.
    uint16_t       y_origin;                    // Lower-left corner.
    uint16_t       image_width;                 // Width in pixels.
    uint16_t       image_height;                // Height in pixels.
    uint8_t        bits_per_pixel;              // Bits per pixel.
    uint8_t        image_descriptor;            // Bits 0-3 are the alpha channel depth; bits 4-5 are the direction.
};

struct TGA_footer
{
    uint32_t extension_area_offset;         // Offset in bytes from the beginning of the file.
    uint32_t developer_directory_offset;    // Offset in bytes from the beginning of the file.
    char     signature[18];                 // "TRUEVISION-XFILE.".
};

struct TGA_developer_directory_entry
{
    uint16_t tag;                       // Application specific tag ID.
    uint32_t developer_field_offset;    // Offset in bytes from the beginning of the file.
    uint32_t developer_field_size;      // Size in bytes.
};

struct TGA_developer_directory
{
    uint16_t entry_count;
    TGA_developer_directory_entry entries[1];   // entry_count number of entries.
};

struct TGA_extension_area
{
    uint16_t extension_size;            // Size in bytes of the extension area (always 495).
    char     author_name[41];           // Null terminated, space/null padded author name.
    char     author_comment1[81];       // Null terminated, space/null padded comment line 1.
    char     author_comment2[81];       // Null terminated, space/null padded comment line 2.
    char     author_comment3[81];       // Null terminated, space/null padded comment line 3.
    char     author_comment4[81];       // Null terminated, space/null padded comment line 4.
    uint16_t creation_month;
    uint16_t creation_day;
    uint16_t creation_year;
    uint16_t creation_hour;
    uint16_t creation_minute;
    uint16_t creation_second;
    char     job_name[41];              // Null terminated, space/null padded production job.
    uint16_t job_hours;                 // Time spent on the job (for billing).
    uint16_t job_minutes;
    uint16_t job_seconds;
    char     software_id[41];           // Null terminated, space/null padded creation application.
    uint16_t version;                   // Version number times 100.
    char     version_letter;
    uint8_t  blue;                      // Background color.  TODO: 2014: Should this be a Color_rgba?
    uint8_t  green;
    uint8_t  red;
    uint8_t  alpha;
    uint16_t pixel_aspect_numerator;    // Pixel aspect ratio.
    uint16_t pixel_aspect_denominator;
    uint16_t gamma_value_numerator;     // Gamma, in the range of 0.0-10.0.
    uint16_t gamma_value_denominator;   // Denominator of zero indicates field is unused.
    uint32_t color_correction_offset;   // Offset in bytes from the beginning of the file.
    uint32_t thumbnail_image_offset;    // Offset in bytes from the beginning of the file.
    uint32_t scanline_table_offset;     // Offset in bytes from the beginning of the file
    TGA_alpha_type attributes_type;     // TGA_alpha_type.
} ;

#pragma pack(pop)

static bool is_left_to_right(uint8_t image_descriptor)
{
    return (image_descriptor & 0x10) != 0x10;
}

static bool is_top_to_bottom(uint8_t image_descriptor)
{
    return (image_descriptor & 0x20) == 0x20;
}

static unsigned int top_to_bottom_bit()
{
    return 0x20;
}

static void validate_tga_header(_In_ const TGA_header* header)
{
    bool succeeded = true;

    succeeded &= (header->image_type == TGA_image_type::True_color);
    succeeded &= (header->bits_per_pixel == 24);
    succeeded &= (header->color_map_length == 0);
    succeeded &= (header->color_map_bits_per_pixel == 0);
    succeeded &= (is_left_to_right(header->image_descriptor));

    // Bound the size as this is used in buffer size calculations.
    succeeded &= (header->image_width <= max_dimension);
    succeeded &= (header->image_height <= max_dimension);

    // id_length (uint8_t) is unbounded.

    CHECK_EXCEPTION(succeeded, u8"Image data is invalid.");
}

static size_t get_pixel_data_offset(_In_ const TGA_header* header)
{
    return sizeof(TGA_header) +
           header->id_length +
           static_cast<size_t>(header->color_map_length) * (header->color_map_bits_per_pixel / 8);
}

bool is_tga_file_name(_In_z_ const char* file_name)
{
    return file_has_extension_case_sensitive(file_name, ".tga");
}

Bitmap decode_bitmap_from_tga_memory(_In_reads_(size) const uint8_t* tga_memory, size_t size)
{
    CHECK_EXCEPTION(size >= sizeof(TGA_header), u8"Image data is invalid.");

    const TGA_header* header = reinterpret_cast<const TGA_header*>(tga_memory);
    validate_tga_header(header);

    // TODO: 2016: Validate no integer overflows from untrusted data.
    const size_t pixel_data_offset = get_pixel_data_offset(header);
    const auto pixel_size = sizeof(Color_rgb);
    const auto pixel_start = reinterpret_cast<const uint8_t*>(tga_memory + pixel_data_offset);
    const size_t pixel_count = static_cast<size_t>(header->image_width) * header->image_height;

    CHECK_EXCEPTION((pixel_start + (pixel_count * pixel_size) >= pixel_start) && (tga_memory + size >= tga_memory), u8"Image data is invalid.");
    CHECK_EXCEPTION(reinterpret_cast<const uint8_t*>(pixel_start + (pixel_count * pixel_size)) <= (tga_memory + size), u8"Image data is invalid.");

    // MSVC complains that std::copy is insecure.
    // _SCL_SECURE_NO_WARNINGS or checked iterator required. TODO: 2016: Consider using _SCL_SECURE_NO_WARNINGS only in release.
    Bitmap bitmap{std::vector<uint8_t>(pixel_count * pixel_size), header->image_width, header->image_height, true};
    if(is_top_to_bottom(header->image_descriptor))
    {
        std::copy(pixel_start, pixel_start + (pixel_count * pixel_size), bitmap.bitmap.data());
    }
    else
    {
        // If this code path is hit, it means that the image should be exported from the content creation tool
        // from top to bottom.
        // TODO: 2016: To encourage this, make such a tool available from the ImageProcessing library.
        PortableRuntime::dprintf("^Copying Targa image bottom to top. Use content that is encoded from top to bottom for best performance.");

        const auto row_length = pixel_count / header->image_height;
        for(decltype(header->image_height) iy = 0; iy < header->image_height; ++iy)
        {
            const auto row_offset = row_length * iy;
            std::copy(pixel_start + (row_offset * pixel_size),
                      pixel_start + ((row_offset + row_length) * pixel_size),
                      &bitmap.bitmap[(pixel_count - row_offset - row_length) * pixel_size]);
        }
    }

    // Return value optimization expected.
    return bitmap;
}

std::vector<uint8_t> encode_tga_from_bitmap(const Bitmap& bitmap)
{
    CHECK_EXCEPTION(bitmap.width <= max_dimension, u8"Image data is invalid.");
    CHECK_EXCEPTION(bitmap.height <= max_dimension, u8"Image data is invalid.");

    std::vector<uint8_t> tga(sizeof(TGA_header) + bitmap.bitmap.size() * sizeof(Color_rgb));

    TGA_header* header = reinterpret_cast<TGA_header*>(tga.data());
    header->color_map_type = TGA_color_map::Has_no_color_map;
    header->image_type = TGA_image_type::True_color;
    header->image_width = static_cast<decltype(header->image_width)>(bitmap.width);
    header->image_height = static_cast<decltype(header->image_height)>(bitmap.height);
    header->bits_per_pixel = sizeof(Color_rgb) * 8;
    header->image_descriptor |= top_to_bottom_bit();

    const auto pixel_start = reinterpret_cast<const uint8_t*>(bitmap.bitmap.data());
    std::copy(pixel_start, pixel_start + bitmap.bitmap.size() * sizeof(Color_rgb), &tga[sizeof(TGA_header)]);

    // Return value optimization expected.
    return tga;
}

}


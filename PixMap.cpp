#include "PreCompile.h"
#include "PixMap.h"             // Pick up forward declarations to ensure correctness.
#include "Bitmap.h"
#include <PortableRuntime/CheckException.h>

// Portable PixMap specs:
// https://en.wikipedia.org/wiki/Netpbm_format
namespace ImageProcessing
{

enum class PixMap_format {P1, P2, P3, P4, P5, P6};

constexpr char whitespace[] = {u8'\t', u8'\n', u8'\v', u8'\f', u8'\r', u8' '};

static bool is_ascii_whitespace_character(char ch) noexcept
{
    constexpr char const* whitespace_end = whitespace + sizeof(whitespace);
    return std::find(whitespace, whitespace_end, ch) != whitespace_end;
}

static bool is_valid_token_character(char ch) noexcept
{
    return ((ch >= u8'0' && ch <= u8'9') ||
            (ch >= u8'A' && ch <= u8'Z') ||
            (ch >= u8'a' && ch <= u8'z') ||
            (ch == u8'#'));
}

static bool is_valid_integer_character(char ch) noexcept
{
    return (ch >= u8'0') && (ch <= u8'9');
}

static int parse_int32(_In_reads_to_ptr_(buffer_end) const char* buffer_start, const char* buffer_end, _Out_ const char** next_start, _Out_ bool* success) noexcept
{
    *success = false;

    // Buffer begins with no whitespace.
    const char* token_begin = buffer_start;
    const char* token_end = buffer_end;

    // Nothing to parse if the buffer is empty.
    int result = 0;
    if(token_begin != token_end)
    {
        // The end of the token is the first whitespace character.
        token_end = std::find_first_of(token_begin, token_end, whitespace, whitespace + sizeof(whitespace));

        // Check for negative sign.
        bool negate = false;
        if(*token_begin == u8'-')
        {
            negate = true;
            ++token_begin;
        }

        // Add the characters into a running sum, if they are digits.
        while((token_begin != token_end) && is_valid_integer_character(*token_begin))
        {
            // TODO: 2016: Currently, parse_int32 does not support accepting INT_MIN (-2147483648) as an input.
            if(result > (INT_MAX - (*token_begin - u8'0')) / 10)
            {
                // Overflow.
                break;
            }

            result *= 10;
            result += *token_begin - u8'0';
            ++token_begin;
        }

        // If the token was fully consumed, then the token was successfully parsed.
        if(token_begin == token_end)
        {
            if(negate)
            {
                result = -result;
            }

            *success = true;
        }
    }

    // If the parse was not successful, do not consume the buffer.
    *next_start = *success ? token_end : buffer_start;

    return result;
}

static std::string parse_string(_In_reads_to_ptr_(buffer_end) const char* buffer_start, const char* buffer_end, _Out_ const char** next_start, _Out_ bool* success)
{
    *success = false;

    // Buffer begins with no whitespace.
    const char* token_begin = buffer_start;
    const char* token_end = buffer_end;

    // Nothing to parse if the buffer is empty.
    std::string result;
    if(token_begin != token_end)
    {
        // The end of the token is the first whitespace character.
        token_end = std::find_first_of(token_begin, token_end, whitespace, whitespace + sizeof(whitespace));

        // Add the characters into the result buffer.
        while((token_begin != token_end) && is_valid_token_character(*token_begin))
        {
            result.append(token_begin, 1);
            ++token_begin;
        }

        // If the token was fully consumed, then the token was successfully parsed.
        if(token_begin == token_end)
        {
            *success = true;
        }
    }

    // If the parse was not successful, do not consume the buffer.
    *next_start = *success ? token_end : buffer_start;

    return result;
}

static void find_first_line_end(_In_reads_to_ptr_(buffer_end) const char* buffer_start, const char* buffer_end, _Out_ const char** line_end) noexcept
{
    constexpr char newlines[] = {u8'\r', u8'\n'};
    *line_end = std::find_first_of(buffer_start, buffer_end, newlines, newlines + sizeof(newlines));
    if(*line_end != buffer_end)
    {
        ++(*line_end);

        // Eat the \n if the file has \r\n as the delimiter as on Windows.
        // NOTE: This will also eat \n\n, which is an optimization in scenarios of concern.
        if(**line_end == u8'\n')
        {
            ++(*line_end);
        }
    }
}

static void find_first_token_begin(_In_reads_to_ptr_(buffer_end) const char* buffer_start, const char* buffer_end, _Out_ const char** token_begin) noexcept
{
     *token_begin = std::find_if_not(buffer_start, buffer_end, is_ascii_whitespace_character);
 }

static PixMap_format pixmap_format_from_string(const std::string& value)
{
    CHECK_EXCEPTION((value == u8"P1") ||
                    (value == u8"P2") ||
                    (value == u8"P3") ||
                    (value == u8"P4") ||
                    (value == u8"P5") ||
                    (value == u8"P6"), u8"Image data is invalid.");

    PixMap_format format;
    if(value == u8"P1")
    {
        format = PixMap_format::P1;
    }
    else if(value == u8"P2")
    {
        format = PixMap_format::P2;
    }
    else if(value == u8"P3")
    {
        format = PixMap_format::P3;
    }
    else if(value == u8"P4")
    {
        format = PixMap_format::P4;
    }
    else if(value == u8"P5")
    {
        format = PixMap_format::P5;
    }
    else
    {
        assert(value == u8"P6");
        format = PixMap_format::P6;
    }

    return format;
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

bool is_pixmap_file_name(_In_z_ const char* file_name)
{
    return file_has_extension_case_sensitive(file_name, ".pbm") ||
           file_has_extension_case_sensitive(file_name, ".pgm") ||
           file_has_extension_case_sensitive(file_name, ".ppm");
}

Bitmap decode_bitmap_from_pixmap_memory(_In_reads_(size) const uint8_t* pixmap_memory, size_t size)
{
    const char* line_begin = reinterpret_cast<const char*>(pixmap_memory);
    const char* line_end = line_begin;
    const char* buffer_end = line_begin + size;

    int image_width = 0, image_height = 0;
    uint8_t image_max_value = 1;

    enum class Parse_mode {magic, width, height, max_value, data};
    Parse_mode mode = Parse_mode::magic;

    PixMap_format format = PixMap_format::P1;

    std::vector<uint8_t> data;

    while(line_begin != buffer_end)
    {
        if(line_begin == line_end)
        {
            find_first_line_end(line_begin, buffer_end, &line_end);
        }

        if((mode != Parse_mode::data) || (format == PixMap_format::P1) || (format == PixMap_format::P2) || (format == PixMap_format::P3))
        {
            find_first_token_begin(line_begin, line_end, &line_begin);
        }
        else
        {
            // P4/P5/P6 data.
            find_first_line_end(line_begin, buffer_end, &line_begin);
            line_end = buffer_end;
        }

        if(line_begin != line_end)
        {
            bool success;
            if(line_begin[0] == u8'#')
            {
                line_begin = line_end;
            }
            else if(mode == Parse_mode::magic)
            {
                const auto token = parse_string(line_begin, line_end, &line_begin, &success);
                CHECK_EXCEPTION(success, u8"Image data is invalid.");

                format = pixmap_format_from_string(token);
                mode = Parse_mode::width;
            }
            else if((mode == Parse_mode::width) ||
                    (mode == Parse_mode::height) ||
                    (mode == Parse_mode::max_value))
            {
                const int token = parse_int32(line_begin, line_end, &line_begin, &success);
                CHECK_EXCEPTION(success, u8"Image data is invalid.");

                if(mode == Parse_mode::width)
                {
                    image_width = token;
                    mode = Parse_mode::height;
                }
                else if(mode == Parse_mode::height)
                {
                    image_height = token;
                    if((format == PixMap_format::P1) || (format == PixMap_format::P4))
                    {
                        mode = Parse_mode::data;
                    }
                    else
                    {
                        mode = Parse_mode::max_value;
                    }

                    data.reserve(image_width * image_height * sizeof(Color_rgb));
                }
                else
                {
                    CHECK_EXCEPTION((token > 0) && (token <= 255), u8"Image data is invalid.");
                    image_max_value = static_cast<uint8_t>(token);
                    mode = Parse_mode::data;
                }
            }
            else if(mode == Parse_mode::data)
            {
                if((format == PixMap_format::P1) ||
                   (format == PixMap_format::P2) ||
                   (format == PixMap_format::P3))
                {
                    int token = parse_int32(line_begin, line_end, &line_begin, &success);
                    CHECK_EXCEPTION(success, u8"Image data is invalid.");
                    CHECK_EXCEPTION((token >= 0) && (token <= image_max_value), u8"Image data is invalid.");

                    if((format == PixMap_format::P1) ||
                       (format == PixMap_format::P2))
                    {
                        // Black and white / Grayscale.
                        CHECK_EXCEPTION((data.size() <= image_width * image_height * sizeof(Color_rgb) - 3), u8"Image data is invalid.");

                        const uint8_t scale = 255 / image_max_value;

                        // P1/P2 only specify a single channel.  Expand to three channels here (R/G/B).
                        data.push_back(static_cast<uint8_t>(token) * scale);
                        data.push_back(static_cast<uint8_t>(token) * scale);
                        data.push_back(static_cast<uint8_t>(token) * scale);
                    }
                    else
                    {
                        // RGB.
                        assert(format == PixMap_format::P3);
                        CHECK_EXCEPTION((data.size() < image_width * image_height * sizeof(Color_rgb)), u8"Image data is invalid.");
                        data.push_back(static_cast<uint8_t>(token));
                    }
                }
                else
                {
                    // Black and white.
                    if(format == PixMap_format::P4)
                    {
                        CHECK_EXCEPTION(line_end == (line_begin + (image_width * image_height) / 8), u8"Image data is invalid.");

                        std::for_each(line_begin, line_end, [image_max_value, &data](uint8_t value)
                        {
                            for(int i = 0; i < 8; ++i)
                            {
                                uint8_t color = 255 - (((value & 0x80) >> 7) * 255);

                                // P4 only specifies a single channel.  Expand to three channels here (R/G/B).
                                data.push_back(color);
                                data.push_back(color);
                                data.push_back(color);

                                value <<= 1;
                            }
                        });
                    }
                    // Grayscale.
                    else if(format == PixMap_format::P5)
                    {
                        CHECK_EXCEPTION(line_end == (line_begin + (image_width * image_height)), u8"Image data is invalid.");

                        const uint8_t scale = 255 / image_max_value;

                        std::for_each(line_begin, line_end, [image_max_value, scale, &data](const uint8_t& value)
                        {
                            CHECK_EXCEPTION((value >= 0) && (value <= image_max_value), u8"Image data is invalid.");

                            // P5 only specifies a single channel.  Expand to three channels here (R/G/B).
                            data.push_back(value * scale);
                            data.push_back(value * scale);
                            data.push_back(value * scale);
                        });
                    }
                    // RGB.
                    else
                    {
                        assert(format == PixMap_format::P6);
                        CHECK_EXCEPTION(line_end == (line_begin + (image_width * image_height * sizeof(Color_rgb))), u8"Image data is invalid.");

                        // RGB.
                        std::copy_if(line_begin, line_end, std::back_inserter(data), [image_max_value](const uint8_t& value) -> bool
                        {
                            CHECK_EXCEPTION((value >= 0) && (value <= image_max_value), u8"Image data is invalid.");
                            return true;
                        });
                    }

                    line_begin = line_end;
                }
            }
        }
    }

    // Ensure that the bitmap data has been fully populated.
    CHECK_EXCEPTION((data.size() == image_width * image_height * sizeof(Color_rgb)), u8"Image data is invalid.");

    Bitmap bitmap{std::move(data), static_cast<unsigned int>(image_width), static_cast<unsigned int>(image_height), true};
    return bitmap;
}

}


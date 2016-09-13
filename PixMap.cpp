#include "PreCompile.h"
//#include "PixMap.h"
#include "Bitmap.h"
#include <PortableRuntime/CheckException.h>

// Portable PixMap specs:
// https://en.wikipedia.org/wiki/Netpbm_format
namespace ImageProcessing
{

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

// TODO: 2016: When parsing, don't update token_end unless successful.

static int parse_int32(_In_reads_to_ptr_(buffer_end) const char* buffer_start, const char* buffer_end, _Out_ const char** token_end, _Out_ bool* success) noexcept
{
    *success = false;

    // Buffer begins with no whitespace.
    const char* token_begin = buffer_start;
    *token_end = buffer_end;

    // Nothing to parse if the buffer is empty.
    int result = 0;
    if(token_begin != *token_end)
    {
        // The end of the token is the first whitespace character.
        *token_end = std::find_first_of(token_begin, *token_end, whitespace, whitespace + sizeof(whitespace));

        // Check for negative sign.
        bool negate = false;
        if(*token_begin == u8'-')
        {
            negate = true;
            ++token_begin;
        }

        // Add the characters into a running sum, if they are digits.
        while((token_begin != *token_end) && is_valid_integer_character(*token_begin))
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
        if(token_begin == *token_end)
        {
            if(negate)
            {
                result = -result;
            }

            *success = true;
        }
    }

    // If the parse was not successful, do not consume the buffer.
    if(!*success)
    {
        *token_end = buffer_start;
    }

    return result;
}

static std::string parse_string(_In_reads_to_ptr_(buffer_end) const char* buffer_start, const char* buffer_end, _Out_ const char** token_end, _Out_ bool* success)
{
    *success = false;

    // Buffer begins with no whitespace.
    const char* token_begin = buffer_start;
    *token_end = buffer_end;

    // Nothing to parse if the buffer is empty.
    std::string result;
    if(token_begin != *token_end)
    {
        // The end of the token is the first whitespace character.
        *token_end = std::find_first_of(token_begin, *token_end, whitespace, whitespace + sizeof(whitespace));

        // Add the characters into the result buffer.
        while((token_begin != *token_end) && is_valid_token_character(*token_begin))
        {
            result.append(token_begin, 1);
            ++token_begin;
        }

        // If the token was fully consumed, then the token was successfully parsed.
        if(token_begin == *token_end)
        {
            *success = true;
        }
    }

    // If the parse was not successful, do not consume the buffer.
    if(!*success)
    {
        *token_end = buffer_start;
    }

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

Bitmap decode_bitmap_from_pixmap_memory(_In_reads_(size) const uint8_t* pixmap_memory, size_t size)
{
    const char* line_begin = reinterpret_cast<const char*>(pixmap_memory);
    const char* line_end = line_begin;
    const char* buffer_end = line_begin + size;

    int image_width = 0, image_height = 0, image_max_value;

    enum class Parse_mode {magic, width, height, max_value, data};
    Parse_mode mode = Parse_mode::magic;

    while(line_begin != buffer_end)
    {
        if(line_begin == line_end)
        {
            find_first_line_end(line_begin, buffer_end, &line_end);
        }

        find_first_token_begin(line_begin, line_end, &line_begin);
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
                CHECK_EXCEPTION(token == u8"P3", u8"Image data is invalid.");
                // TODO: Support P1-P6.  Skip P7.
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
                    mode = Parse_mode::max_value;
                }
                else
                {
                    image_max_value = token;
                    mode = Parse_mode::data;
                }
            }
            else if(mode == Parse_mode::data)
            {
                // TODO:
            }
        }
    }

    Bitmap bitmap{};
    bitmap.width = image_width;
    bitmap.height = image_height;
    return bitmap;
}

}


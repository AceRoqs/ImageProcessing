#include "PreCompile.h"
//#include "PixMap.h"
#include "Bitmap.h"
#include <PortableRuntime/CheckException.h>

// Portable PixMap specs:
// https://en.wikipedia.org/wiki/Netpbm_format
namespace ImageProcessing
{

constexpr char whitespace[] = {u8'\t', u8'\n', u8'\v', u8'\f', u8'\r', u8' '};
constexpr char whitespace_and_null[] = {u8'\t', u8'\n', u8'\v', u8'\f', u8'\r', u8' ', u8'\0'};
constexpr char integers[] = {u8'0', u8'1', u8'2', u8'3', u8'4', u8'5', u8'6', u8'7', u8'8', u8'9'};

static bool is_ascii_whitespace_character(char ch) noexcept
{
    for(auto ix = 0u; ix < sizeof(whitespace); ++ix)
    {
        if(whitespace[ix] == ch)
        {
            return true;
        }
    }

    return false;
}

/*static*/ int parse_int(_In_reads_(size) const char* buffer, size_t size, _Out_ const char** token_end, _Out_ bool* success) noexcept
{
    *success = false;
    *token_end = buffer + size;

    // Beginning of token is the first non-whitespace character.
    const char* token_begin = std::find_first_of(buffer, *token_end, whitespace_and_null, whitespace_and_null + sizeof(whitespace_and_null),
        [](const char ch1, const char ch2)
        {
            return ch1 != ch2;
        });

    // Nothing to parse if the buffer is only whitespace.
    int result = 0;
    if(token_begin != *token_end)
    {
        // The end of the token is the first whitespace character.
        *token_end = std::find_first_of(token_begin, *token_end, whitespace_and_null, whitespace_and_null + sizeof(whitespace_and_null));

        // Check for negative sign.
        bool negate = false;
        if(*token_begin == u8'-')
        {
            negate = true;
            ++token_begin;
        }

        // Add the characters into a running sum, if they are digits.
        while(token_begin != *token_end)
        {
            constexpr char const* integers_end = integers + sizeof(integers);
            if(std::find(integers, integers_end, *token_begin) == integers_end)
            {
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
        *token_end = buffer;
    }

    return result;
}

/*static*/ std::string parse_string(_In_reads_(size) const char* buffer, size_t size, _Out_ const char** token_end, _Out_ bool* success)
{
    *success = false;
    *token_end = buffer + size;

    // Beginning of token is the first non-whitespace character.
    const char* token_begin = std::find_first_of(buffer, *token_end, whitespace_and_null, whitespace_and_null + sizeof(whitespace_and_null),
        [](const char ch1, const char ch2)
        {
            return ch1 != ch2;
        });

    // Nothing to parse if the buffer is only whitespace.
    std::string result;
    if(token_begin != *token_end)
    {
        // The end of the token is the first whitespace character.
        *token_end = std::find_first_of(token_begin, *token_end, whitespace_and_null, whitespace_and_null + sizeof(whitespace_and_null));

        // Add the characters into a running sum, if they are digits.
        while(token_begin != *token_end)
        {
            if(!((*token_begin >= u8'0' && *token_begin <= u8'9') ||
                 (*token_begin >= u8'A' && *token_begin <= u8'Z') ||
                 (*token_begin >= u8'a' && *token_begin <= u8'z') ||
                 (*token_begin == u8'#')))
            {
                break;
            }

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
        *token_end = buffer;
    }

    return result;
}

/*static*/ void find_end_of_line(_In_reads_(size) const char* buffer, size_t size, _Out_ const char** line_end) noexcept
{
    *line_end = buffer + size;
    const char* line_begin = buffer;

    for(size_t ix = 0; ix < size; ++ix)
    {
        if(line_begin[ix] == u8'\n')
        {
            *line_end = line_begin + ix + 1;
            break;
        }
        else if(line_begin[ix] == u8'\r')
        {
            *line_end = line_begin + ix + 1;

            // Eat the \n if the file has \r\n as the deliminator as on Windows.
            ++ix;
            if(ix < size)
            {
                if(line_begin[ix] == u8'\n')
                {
                    *line_end = line_begin + ix + 1;
                }
            }

            break;
        }
    }
}

/*static*/ void advance_to_end_of_line(_In_reads_(size) const char* buffer, size_t size, _Out_ const char** token_end, _Out_ bool* success) noexcept
{
    *token_end = buffer + size;
    *success = false;
    const char* token_begin = buffer;

    for(size_t ix = 0; ix < size; ++ix)
    {
        if(token_begin[ix] == u8'\n')
        {
            *token_end = token_begin + ix + 1;
            *success = true;
            break;
        }
        else if(token_begin[ix] == u8'\r')
        {
            *token_end = token_begin + ix + 1;
            *success = true;

            // Eat the \n if the file has \r\n as the deliminator as on Windows.
            ++ix;
            if(ix < size)
            {
                if(token_begin[ix] == u8'\n')
                {
                    *token_end = token_begin + ix + 1;
                }
            }

            break;
        }
    }
}

void advance_past_whitespace(_In_reads_(size) const char* buffer, size_t size, _Out_ const char** next_token) noexcept
{
     *next_token = std::find_if_not(buffer, buffer + size, is_ascii_whitespace_character);
 }

Bitmap decode_bitmap_from_pixmap_memory(_In_reads_(size) const uint8_t* pixmap_memory, size_t size)
{
    (void)pixmap_memory;
    (void)size;

    // TODO: 2016: Support comment before the magic number
    auto next_token = reinterpret_cast<const char*>(pixmap_memory);
    bool success;

    auto magic_number = parse_string(next_token, size, &next_token, &success);
    CHECK_EXCEPTION(success, u8"Image data is invalid.");
    CHECK_EXCEPTION(magic_number == u8"P3", u8"Image data is invalid.");

    auto width = parse_int(next_token, size, &next_token, &success);
    CHECK_EXCEPTION(success, u8"Image data is invalid.");

    auto height = parse_int(next_token, size, &next_token, &success);
    CHECK_EXCEPTION(success, u8"Image data is invalid.");

    auto max_value = parse_int(next_token, size, &next_token, &success);
    CHECK_EXCEPTION(success, u8"Image data is invalid.");
    (void)max_value;

    Bitmap bitmap{};
    bitmap.width = width;
    bitmap.height = height;
    return bitmap;
}

}


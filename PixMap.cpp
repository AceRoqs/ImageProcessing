#include "PreCompile.h"
//#include "PixMap.h"
#include "Bitmap.h"
#include <PortableRuntime/CheckException.h>

// Portable PixMap specs:
// https://en.wikipedia.org/wiki/Netpbm_format
namespace ImageProcessing
{

constexpr char whitespace[] = { u8'\t', u8'\n', u8'\v', u8'\f', u8'\r', u8' '};
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

class Tokenizer
{
    const char* m_buffer_start;
    const char* m_current_pointer;
    const char* m_buffer_end;

public:
    Tokenizer(_In_reads_(size) const char* buffer, size_t size) noexcept : m_buffer_start(buffer), m_current_pointer(buffer), m_buffer_end(buffer + size)
    {
    }

    void advance_past_whitespace() noexcept
    {
        m_current_pointer = std::find_if_not(m_current_pointer, m_buffer_end, is_ascii_whitespace_character);
    }

    std::string read_token()
    {
        CHECK_EXCEPTION(m_buffer_start != m_buffer_end, u8"Image data is invalid.");
        const auto token_end = std::find_if(m_current_pointer, m_buffer_end, is_ascii_whitespace_character);

        // TODO: 2016: What happens if string range has embedded null?

        std::string token(m_current_pointer, token_end - m_current_pointer);
        m_current_pointer = token_end;

        return token;
    }
};

Bitmap decode_bitmap_from_pixmap_memory(_In_reads_(size) const uint8_t* pixmap_memory, size_t size)
{
    (void)pixmap_memory;
    (void)size;

    Tokenizer tokenizer(reinterpret_cast<const char*>(pixmap_memory), size);
    tokenizer.advance_past_whitespace();
    const auto magic_number = tokenizer.read_token();
    CHECK_EXCEPTION(magic_number == u8"P3" || magic_number == u8"P6", u8"Image data is invalid.");

    Bitmap bitmap{};
    return bitmap;
}

}


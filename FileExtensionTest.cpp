#include "PreCompile.h"
#include "FileExtensionTest.h"

namespace ImageProcessing
{

static char ascii_to_lower(char ch) noexcept
{
    if(ch >= u8'A' && ch <= u8'Z')
    {
        ch = u8'a' + ch - u8'A';
    }

    return ch;
}

static bool are_utf8_strings_identical_ascii_case_insensitive(_In_z_ const char* first_string, _In_z_ const char* second_string) noexcept
{
    bool identical = true;

    while((*first_string != u8'\0') || (*second_string != u8'\0'))
    {
        if(ascii_to_lower(*first_string) != ascii_to_lower(*second_string))
        {
            identical = false;
            break;
        }
        ++first_string;
        ++second_string;
    }

    return identical;
}

// This function does a case sensitive comparison of UTF-8, except that
// ASCII characters are matched in a case insensitive way.  This is intended, as
// extensions for image types are all ASCII, and nearly always, the extension is
// passed as a string literal.  So file names can have UTF-8 characters, but the
// correct handler (pcx, targa, etc.) will always be correctly found.
bool file_has_extension_case_sensitive(_In_z_ const char* file_name, _In_z_ const char* extension) noexcept
{
    const size_t length_file = strlen(file_name);
    const size_t length_extension = strlen(extension);
    return ((length_file >= length_extension) && are_utf8_strings_identical_ascii_case_insensitive(file_name + length_file - length_extension, extension));
}

}


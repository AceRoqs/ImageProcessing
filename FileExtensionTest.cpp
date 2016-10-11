#include "PreCompile.h"
#include "FileExtensionTest.h"

namespace ImageProcessing
{

// This function is case sensitive due to the lack of library support for
// UTF-8 case insensitive matching.
// TODO: Consider case insensitive for ASCII subset of UTF-8.
bool file_has_extension_case_sensitive(_In_z_ const char* file_name, _In_z_ const char* extension) noexcept
{
    const size_t length_file = strlen(file_name);
    const size_t length_extension = strlen(extension);
    return ((length_file >= length_extension) && (strcmp(file_name + length_file - length_extension, extension) == 0));
}

}


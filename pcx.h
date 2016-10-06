#pragma once

namespace ImageProcessing
{

bool is_pcx_file_name(_In_z_ const char* file_name);
struct Bitmap decode_bitmap_from_pcx_memory(_In_reads_(size) const uint8_t* pcx_memory, size_t size);

}


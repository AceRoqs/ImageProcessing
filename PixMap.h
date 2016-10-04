#pragma once

namespace ImageProcessing
{

bool is_pixmap_file_name(_In_z_ const char* file_name);
struct Bitmap decode_bitmap_from_pixmap_memory(_In_reads_(size) const uint8_t* pixmap_memory, size_t size);

}


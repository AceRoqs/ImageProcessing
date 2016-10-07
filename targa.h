#pragma once

namespace ImageProcessing
{

bool is_tga_file_name(_In_z_ const char* file_name);
struct Bitmap decode_bitmap_from_tga_memory(_In_reads_(size) const uint8_t* tga_memory, size_t size);
std::vector<uint8_t> encode_tga_from_bitmap(const struct Bitmap& bitmap);

}


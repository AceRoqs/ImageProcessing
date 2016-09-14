#pragma once

namespace ImageProcessing
{

struct Bitmap decode_bitmap_from_pixmap_memory(_In_reads_(size) const uint8_t* pixmap_memory, size_t size);

}


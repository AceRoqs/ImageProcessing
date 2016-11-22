#include "PreCompile.h"

namespace ImageProcessing
{

std::vector<float> generate_simple_box_filter(unsigned int dimension)
{
    // 65536 * 65536 = UINT_MAX.
    assert(dimension < 65536);

    const size_t element_count = dimension * dimension;

    std::vector<float> box_filter;
    box_filter.reserve(element_count);

    const float value = 1.0f / element_count;
    for(size_t ix = 0; ix < element_count; ++ix)
    {
        box_filter.push_back(value);
    }

    return box_filter;
}

}


#include "PreCompile.h"
#include "Bitmap.h"

namespace ImageProcessing
{

std::vector<float> generate_simple_box_filter(unsigned int dimension)
{
    // 65536 * 65536 = UINT_MAX.  Dimension should be odd so it has a center point.
    assert(dimension < 65536);
    assert(dimension % 2 == 1);

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

Bitmap apply_box_filter(const std::vector<float>& filter, unsigned int dimension, const Bitmap& source)
{
    assert(dimension < 65536);
    assert(dimension % 2 == 1);
    assert(dimension < source.width / 2);
    assert(dimension < source.height / 2);

    // TODO: 2016: Should define a minimum height/width that Bitmaps are required to support.
    // TODO: 2016: It is unnecessary that filters can be so big.
    assert(source.width < 4096);
    assert(source.height < 4096);

    Bitmap target;
    target.height = source.height;
    target.width = source.width;
    target.filtered = source.filtered;
    target.bitmap.resize(source.bitmap.size());

    auto source_rgb = reinterpret_cast<const Color_rgb*>(&source.bitmap[0]);
    auto target_rgb = reinterpret_cast<Color_rgb*>(&target.bitmap[0]);

    const int half_dimension = dimension / 2;
    for(int h_ix = 0; h_ix < static_cast<int>(source.height); ++h_ix)
    {
        for(int w_ix = 0; w_ix < static_cast<int>(source.width); ++w_ix)
        {
            Color_rgb rgb = {0, 0, 0};
            for(int d_h = 0; d_h < static_cast<int>(dimension); ++d_h)
            {
                for(int d_w = 0; d_w < static_cast<int>(dimension); ++d_w)
                {
                    float filter_sample = filter[dimension * d_h + d_w];

                    int sample_w = std::min(std::max(0, w_ix + d_w - half_dimension), static_cast<int>(source.width) - 1);
                    int sample_h = std::min(std::max(0, h_ix + d_h - half_dimension), static_cast<int>(source.height) - 1);
                    const Color_rgb* color_sample = &source_rgb[source.width * sample_h + sample_w];

                    rgb.red += static_cast<unsigned char>(color_sample->red * filter_sample);
                    rgb.green += static_cast<unsigned char>(color_sample->green * filter_sample);
                    rgb.blue += static_cast<unsigned char>(color_sample->blue * filter_sample);
                }
            }

            target_rgb[source.width * h_ix + w_ix] = rgb;
        }
    }

    return target;
}

void generate_topdown_gradient_in_place(Bitmap& target, const Color_rgb& start_color, const Color_rgb& end_color)
{
    auto pixel = reinterpret_cast<Color_rgb*>(&target.bitmap[0]);
    for(unsigned int yy = 0; yy < target.height; ++yy)
    {
        Color_rgb color;
        color.red = start_color.red + static_cast<uint8_t>(yy * ((end_color.red - start_color.red + 1.0f) / target.height));
        color.green = start_color.green + static_cast<uint8_t>(yy * ((end_color.green - start_color.green + 1.0f) / target.height));
        color.blue = start_color.blue + static_cast<uint8_t>(yy * ((end_color.blue - start_color.blue + 1.0f) / target.height));

        for(unsigned int xx = 0; xx < target.width; ++xx)
        {
            *pixel = color;
            ++pixel;
        }
    }
}

}


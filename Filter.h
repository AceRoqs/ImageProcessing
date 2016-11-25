#pragma once

namespace ImageProcessing
{

std::vector<float> generate_simple_box_filter(unsigned int dimension);
Bitmap apply_box_filter(const std::vector<float>& filter, unsigned int dimension, const Bitmap& source);

}


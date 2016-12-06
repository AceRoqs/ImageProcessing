#pragma once

namespace ImageProcessing
{

std::vector<float> generate_simple_box_filter(unsigned int dimension);
Bitmap apply_box_filter(const std::vector<float>& filter, unsigned int dimension, const Bitmap& source);
void generate_topdown_gradient_in_place(Bitmap& target, const Color_rgb& start_color, const Color_rgb& end_color);
void generate_bottomup_gradient_in_place(Bitmap& target, const Color_rgb& start_color, const Color_rgb& end_color);

}


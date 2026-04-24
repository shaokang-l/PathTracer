#pragma once
#include "utils/matrix.hpp"
namespace gl
{
    static gl::vec3 RED = gl::vec3(1.0f, 0.0f, 0.0f);
    static gl::vec3 GREEN = gl::vec3(0.0f, 1.0f, 0.0f);
    static gl::vec3 BLUE = gl::vec3(0.0f, 0.2f, 0.7f);
    static gl::vec3 WHITE = gl::vec3(1.0f, 1.0f, 1.0f);
    static gl::vec3 BLACK = gl::vec3(0.0f, 0.0f, 0.0f);
    static gl::vec3 YELLOW = gl::vec3(1.0f, 1.0f, 0.0f);
    static gl::vec3 CYAN = gl::vec3(0.0f, 1.0f, 1.0f);
    static gl::vec3 MAGENTA = gl::vec3(1.0f, 0.0f, 1.0f);
    static gl::vec3 GRAY = gl::vec3(0.5f, 0.5f, 0.5f);
    static gl::vec3 LIGHT_GRAY = gl::vec3(0.8f, 0.8f, 0.8f);
    static gl::vec3 DARK_GRAY = gl::vec3(0.2f, 0.2f, 0.2f);
    static gl::vec3 SKY_BLUE = gl::vec3(0.529f, 0.808f, 0.922f);
    static gl::vec3 ORANGE = gl::vec3(1.0f, 0.647f, 0.0f);
    static gl::vec3 PURPLE = gl::vec3(0.502f, 0.0f, 0.502f);
    static gl::vec3 BROWN = gl::vec3(0.647f, 0.164f, 0.164f);
    static gl::vec3 PINK = gl::vec3(1.0f, 0.412f, 0.706f);
    static gl::vec3 GOLD = gl::vec3(1.0f, 0.843f, 0.0f);
    static gl::vec3 SILVER = gl::vec3(0.753f, 0.753f, 0.753f);
    static gl::vec3 BRASS = gl::vec3(0.71f, 0.65f, 0.26f);
    static gl::vec3 BRONZE = gl::vec3(0.55f, 0.47f, 0.14f);
    static gl::vec3 CHROME = gl::vec3(0.4f, 0.4f, 0.4f);
    static gl::vec3 COPPER = gl::vec3(0.72f, 0.45f, 0.2f);
    static gl::vec3 GOLD_POLISHED = gl::vec3(1.0f, 0.8f, 0.0f);
    static gl::vec3 SILVER_POLISHED = gl::vec3(0.95f, 0.93f, 0.88f);
    static gl::vec3 EMERALD = gl::vec3(0.07568f, 0.61424f, 0.07568f);
    static gl::vec3 JADE = gl::vec3(0.54f, 0.89f, 0.63f);
    static gl::vec3 OBSIDIAN = gl::vec3(0.18275f, 0.17f, 0.22525f);
    static gl::vec3 PEARL = gl::vec3(1.0f, 0.829f, 0.829f);
    static gl::vec3 RUBY = gl::vec3(0.61424f, 0.04136f, 0.04136f);
    static gl::vec3 TURQUOISE = gl::vec3(0.396f, 0.74151f, 0.69102f);
    static gl::vec3 BLACK_PLASTIC = gl::vec3(0.01f, 0.01f, 0.01f);
    static gl::vec3 CYAN_PLASTIC = gl::vec3(0.0f, 0.50980392f, 0.50980392f);
    static gl::vec3 GREEN_PLASTIC = gl::vec3(0.1f, 0.35f, 0.1f);
    static gl::vec3 RED_PLASTIC = gl::vec3(0.5f, 0.0f, 0.0f);
    static gl::vec3 WHITE_PLASTIC = gl::vec3(0.55f, 0.55f, 0.55f);
    static gl::vec3 YELLOW_PLASTIC = gl::vec3(0.5f, 0.5f, 0.0f);

    static inline float rgbToLuminance(const gl::vec3 &rgb)
    {
        float y = dot(rgb, gl::vec3(0.2126f, 0.7152f, 0.0722f));
        return y;
    }
}; // namespace gl
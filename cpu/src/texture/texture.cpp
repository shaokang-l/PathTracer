#include "texture/texture.hpp"
#include "external/stb_image.h"
ImageTexture::ImageTexture(const std::string &filename, bool flip_y, bool isHDR)
{
  if (isHDR)
  {
    int width, height, channels = 3;
    float *data =
        stbi_loadf(filename.c_str(), &width, &height, &channels, 3);
    if (data == nullptr)
    {
      throw std::runtime_error("Failed to load texture file");
    }

    // set the width and height
    _width = width;
    _height = height;

    // set the texels
    texels.resize(width);
    for (uint i = 0; i < width; i++)
    {
      texels[i].resize(height);
    }

    for (uint i = 0; i < width; i++)
    {
      for (uint j = 0; j < height; j++)
      {
        // flip the y axis
        if (flip_y)
        {
          texels[i][j] = gl::vec3(data[(i + j * width) * channels + 0],
                                  data[(i + j * width) * channels + 1],
                                  data[(i + j * width) * channels + 2]);
        }
        else
        {
          texels[i][j] = gl::vec3(
              data[(i + (height - j - 1) * width) * channels + 0],
              data[(i + (height - j - 1) * width) * channels + 1],
              data[(i + (height - j - 1) * width) * channels + 2]);
        }
      }
    }
    // free the data
    stbi_image_free(data);
  }
  else
  {
    int width, height, channels = 3;
    unsigned char *data =
        stbi_load(filename.c_str(), &width, &height, &channels, 3);
    if (data == nullptr)
    {
      throw std::runtime_error("Failed to load texture file");
    }

    // set the width and height
    _width = width;
    _height = height;

    // set the texels
    texels.resize(width);
    for (uint i = 0; i < width; i++)
    {
      texels[i].resize(height);
    }

    for (uint i = 0; i < width; i++)
    {
      for (uint j = 0; j < height; j++)
      {
        // flip the y axis
        if (flip_y)
        {
          texels[i][j] = gl::vec3(data[(i + j * width) * channels + 0] / 255.0f,
                                  data[(i + j * width) * channels + 1] / 255.0f,
                                  data[(i + j * width) * channels + 2] / 255.0f);
        }
        else
        {
          texels[i][j] = gl::vec3(
              data[(i + (height - j - 1) * width) * channels + 0] / 255.0f,
              data[(i + (height - j - 1) * width) * channels + 1] / 255.0f,
              data[(i + (height - j - 1) * width) * channels + 2] / 255.0f);
        }
      }
    }
    // free the data
    stbi_image_free(data);
  }
}

gl::vec3 ImageTexture::getTexelColor(float u, float v, LERP_MODE mode) const
{
  uint x = 0;
  uint y = 0;

  if (mode == LERP_MODE::NEAREST)
  {
    // get the texel coordinate
    x = (uint)(u * _width);
    y = (uint)(v * _height);
    // clamp the texel coordinate
    x = std::clamp(x, 0u, _width - 1);
    y = std::clamp(y, 0u, _height - 1);

    return texels[x][y];
  }
  else if (mode == LERP_MODE::BILINEAR)
  {
    // get the texel coordinate
    x = (uint)(u * _width);
    y = (uint)(v * _height);

    // get the texel coordinate
    float x1 = (u * _width) - x;
    float y1 = (v * _height) - y;

    x = std::clamp(x, 0u, _width - 2);
    y = std::clamp(y, 0u, _height - 2);

    // get the texel coordinate
    float x2 = 1.0 - x1;
    float y2 = 1.0 - y1;

    // get the texel coordinate
    gl::vec3 c1 = texels[x][y];
    gl::vec3 c2 = texels[x + 1][y];
    gl::vec3 c3 = texels[x][y + 1];
    gl::vec3 c4 = texels[x + 1][y + 1];

    // get the texel coordinate
    gl::vec3 c12 = c1 * x2 + c2 * x1;
    gl::vec3 c34 = c3 * x2 + c4 * x1;

    // get the texel coordinate
    gl::vec3 c1234 = c12 * y2 + c34 * y1;

    return c1234;
  }
  else
  {
    throw std::runtime_error("Unsupported lerp mode");
  }

  return gl::vec3(0.0f);
};

CheckerTexture::CheckerTexture(gl::vec3 color1, gl::vec3 color2, float scale)
{
  _color1 = color1;
  _color2 = color2;
  _scale = scale;
}

CheckerTexture::CheckerTexture(float scale)
{
  _color1 = gl::vec3(0.0f);
  _color2 = gl::vec3(1.0f);
  _scale = scale;
}

gl::vec3 CheckerTexture::getTexelColor(float u, float v, LERP_MODE mode) const
{
  // get the texel coordinate
  float x = u * _scale;
  float y = v * _scale;

  // get the texel coordinate
  float s = sin(x) * sin(y);

  // get the texel coordinate
  if (s < 0.0f)
  {
    return _color1;
  }
  else
  {
    return _color2;
  }
};

gl::vec3 NoiseTexture::getTexelColor(float u, float v, LERP_MODE mode) const
{

  // get the texel coordinate
  float x = u * _scale;
  float y = v * _scale;

  return gl::fractal_perlin_2D(gl::vec2(x, y), _fractal);
};
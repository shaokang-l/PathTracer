#pragma once
#include "objectList.hpp"
#include "utils/matrix.hpp"
#include "utils/utility.hpp"
#include <cstdint>

class FrameBuffer
{
public:
  FrameBuffer(uint width, uint height, uint spp_x = 4, uint spp_y = 4,
              uint channels = 3);

  void setOffsets(uint spp_x = 4, uint spp_y = 4);

  std::vector<gl::vec2> getOffsets() { return this->sample_offset; }

  uint getSampleCount() { return this->sample_offset.size(); }

  void setPixelColor(uint u, uint v, gl::vec3 color);
  // the input should be 2 integers
  gl::vec3 getPixelColor(uint u, uint v) const
  {
    return this->_pixel_color[u][v];
  };

  uint getWidth() const { return this->width; };

  uint getHeight() const { return this->height; };

  void writeToFile(const std::string &file_path, float gamma = 1.0f) const;
  void gaussianBlur(int kernel_size = 3, float sigma = 1.0f);

private:
  uint width;
  uint height;
  uint channels;
  std::vector<gl::vec2> sample_offset;
  std::vector<std::vector<gl::vec3>> _pixel_color;
};

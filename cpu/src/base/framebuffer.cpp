#include "base/framebuffer.hpp"
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "external/stb_image.h"
#include "external/stb_image_write.h"
#include <iostream>
#include <stdexcept>

FrameBuffer::FrameBuffer(uint width, uint height, uint spp_x, uint spp_y,
                         uint channels) {
  // row major
  this->width = width;
  this->height = height;
  this->channels = channels;
  _pixel_color.resize(height);
  for (uint i = 0; i < height; i++) {
    _pixel_color[i].resize(width);
  }

  setOffsets(spp_x, spp_y);
}

void FrameBuffer::setPixelColor(uint u, uint v, gl::vec3 color) {
  this->_pixel_color[u][v] = color;
}

void FrameBuffer::writeToFile(const std::string &file_path, float gamma) const {
  std::vector<uint8_t> data;
  data.reserve(this->width * this->height * this->channels);

  for (uint i = 0; i < this->height; i++) {
    for (uint j = 0; j < this->width; j++) {
      for (uint k = 0; k < this->channels; k++) {
        // nan handling and gamma correction
        auto color = this->_pixel_color[i][j][k];
        if (color != color)
          color = 0.0f;
        else
          color = pow(color, 1.0f / gamma);

        data.push_back(
            static_cast<uint8_t>(std::clamp(color, 0.0f, 1.0f) * 255));
      }
    }
  }

  stbi_flip_vertically_on_write(1);
  auto res =
      stbi_write_png(file_path.c_str(), this->width, this->height,
                     this->channels, data.data(), this->width * this->channels);

  std::cout << "write to file: " << file_path << std::endl;
  if (res == 0) {
    std::cout << "write to file failed" << std::endl;
  } else {
    std::cout << "write to file success" << std::endl;
  }
};

void FrameBuffer::gaussianBlur(int kernel, float sigma) {
  // gaussian blur on the pixel color, using 2 pass
  if (kernel % 2 == 0 || kernel < 3 || sigma < 0.0f) {
    std::cout << "kernel size should be odd, sigma should be positive, kernel "
                 "size should be larger than 1"
              << std::endl;
    return;
  }
  // generate gaussian kernel
  std::vector<std::vector<float>> kernel_2d;
  kernel_2d.resize(kernel);
  for (uint i = 0; i < kernel; i++) {
    kernel_2d[i].resize(kernel);
  }

  float sum = 0.0f;
  for (int x = -kernel / 2; x <= kernel / 2; x++) {
    for (int y = -kernel / 2; y <= kernel / 2; y++) {
      float r = sqrt(x * x + y * y);
      kernel_2d[x + kernel / 2][y + kernel / 2] =
          (exp(-(r * r) / (2 * sigma * sigma)) / (2 * M_PI * sigma * sigma));
      sum += kernel_2d[x + kernel / 2][y + kernel / 2];
    }
  }

  for (uint i = 0; i < kernel; i++) {
    for (uint j = 0; j < kernel; j++) {
      kernel_2d[i][j] /= sum;
    }
  }
  // using the blur kernel in 1 pass
  std::vector<std::vector<gl::vec3>> pixel_color_temp;
  pixel_color_temp.resize(this->height);
  for (uint i = 0; i < this->height; i++) {
    pixel_color_temp[i].resize(this->width);
  }

  for (uint i = 0; i < this->height; i++) {
    for (uint j = 0; j < this->width; j++) {
      gl::vec3 color = {0.0f, 0.0f, 0.0f};
      for (uint s = 0; s < kernel_2d.size(); s++) {
        for (uint t = 0; t < kernel_2d[0].size(); t++) {
          int index_x = i - (int)kernel / 2 + s;
          int index_y = j - (int)kernel / 2 + t;
          if (index_x < 0) {
            index_x = 0;
          }
          if (index_x >= (int)this->height) {
            index_x = this->height - 1;
          }
          if (index_y < 0) {
            index_y = 0;
          }
          if (index_y >= (int)this->width) {
            index_y = this->width - 1;
          }
          color += this->_pixel_color[index_x][index_y] * kernel_2d[s][t];
        }
      }
      pixel_color_temp[i][j] = color;
    }
  }

  // assign value
  for (uint i = 0; i < this->height; i++) {
    for (uint j = 0; j < this->width; j++) {
      this->_pixel_color[i][j] = pixel_color_temp[i][j];
    }
  }
};

void FrameBuffer::setOffsets(uint spp_x, uint spp_y) {
  this->sample_offset.clear();
  // sample_offset offset in the grid
  for (uint i = 0; i < spp_x; i++) {
    for (uint j = 0; j < spp_y; j++) {
      float i_seg = (float)i / (float)spp_x;
      float i_seg_next = (float)(i + 1) / (float)spp_x;
      float j_seg = (float)j / (float)spp_y;
      float j_seg_next = (float)(j + 1) / (float)spp_y;
      gl::vec2 offset = {gl::rand_num(i_seg, i_seg_next),
                         gl::rand_num(j_seg, j_seg_next)};
      this->sample_offset.push_back(offset);
    }
  }
}

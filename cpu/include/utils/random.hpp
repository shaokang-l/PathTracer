#pragma once
#include "utils/matrix.hpp"
#include "utils/utility.hpp"

namespace gl {

inline gl::vec3 uniformSampleSphere(float u1, float u2) {
  // z ∈ [−1,1], φ ∈ [0,2π)
  float z = 1.0f - 2.0f * u1;
  float r = std::sqrt(std::max(0.f, 1.f - z * z));
  float phi = 2.0f * M_PI * u2;
  float x = r * std::cos(phi);
  float y = r * std::sin(phi);
  return gl::vec3(x, y, z);
}

inline vec3 uniformSampleHemiSphere(float r1 = rand_num(),
                                    float r2 = rand_num()) {
  float x = std::cos(2.f * M_PI * r1) * std::sqrt(r2 - r2 * r2);
  float y = std::sin(2.f * M_PI * r1) * std::sqrt(r2 - r2 * r2);
  float z = 1.f - r2;
  return vec3(x, y, z);
};

inline vec3 cosineSampleHemiSphere(float r1 = rand_num(),
                                   float r2 = rand_num()) {
  float x = std::cos(2.f * M_PI * r1) * std::sqrt(r2);
  float y = std::sin(2.f * M_PI * r1) * std::sqrt(r2);
  float z = std::sqrt(1 - r2);
  return vec3(x, y, z);
}

// A vector vec3(x,y,z) in the local frame where the positive z
// z–axis points toward the sphere's center.
// making it perfect for importance‐sampling a spherical light source.
inline vec3 randomToSphere(float radius, float distance_squared,
                           float r1 = rand_num(), float r2 = rand_num()) {
  float z =
      1.f + r2 * (std::sqrt(1.f - radius * radius / distance_squared) - 1.f);
  float phi = 2.f * M_PI * r1;
  float x = std::cos(phi) * std::sqrt(1.f - z * z);
  float y = std::sin(phi) * std::sqrt(1.f - z * z);
  return vec3(x, y, z);
}

// stratified sampling offset
inline std::vector<gl::vec2> getOffsets(int spp_x, int spp_y) {
  // sample_offset offset in the grid
  std::vector<gl::vec2> sample_offset;
  for (int i = 0; i < spp_x; i++) {
    for (int j = 0; j < spp_y; j++) {
      float i_seg = (float)i / (float)spp_x;
      float i_seg_next = (float)(i + 1) / (float)spp_x;
      float j_seg = (float)j / (float)spp_y;
      float j_seg_next = (float)(j + 1) / (float)spp_y;
      gl::vec2 offset = {gl::rand_num(i_seg, i_seg_next),
                         gl::rand_num(j_seg, j_seg_next)};
      sample_offset.push_back(offset);
    }
  }

  return sample_offset;
}

}; // namespace gl
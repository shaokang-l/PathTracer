#pragma once
#include "utils/matrix.hpp"
#include "utils/transformations.hpp"
#include <cmath>

namespace gl {

// Evaluate a cubic Bézier at parameter u ∈ [0,1]:
//
//    C(u) = (1−u)³ P₀ + 3u(1−u)² P₁ + 3u²(1−u) P₂ + u³ P₃
//
inline vec3 evalBezier(const std::array<vec3, 4> &cps, float u) {
  float u1 = 1.0f - u;
  float b0 = u1 * u1 * u1;
  float b1 = 3.0f * u * u1 * u1;
  float b2 = 3.0f * u * u * u1;
  float b3 = u * u * u;
  return cps[0] * b0 + cps[1] * b1 + cps[2] * b2 + cps[3] * b3;
}

// Evaluate the derivative dC/du of that same cubic Bézier:
//
//    C′(u) = 3 [ (1−u)² (P₁−P₀) + 2u(1−u) (P₂−P₁) + u² (P₃−P₂) ]
//
inline vec3 evalBezierDerivative(const std::array<vec3, 4> &cps, float u) {
  // Precompute the 3*(Pi+1 - Pi) terms
  std::array<vec3, 3> dcp = {3.0f * (cps[1] - cps[0]), 3.0f * (cps[2] - cps[1]),
                             3.0f * (cps[3] - cps[2])};
  float u1 = 1.0f - u;
  float b0 = u1 * u1;
  float b1 = 2.0f * u * u1;
  float b2 = u * u;
  return dcp[0] * b0 + dcp[1] * b1 + dcp[2] * b2;
}

}; // namespace gl
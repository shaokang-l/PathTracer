// ======================================================================== //
// ptInterop.h - tiny conversions between OWL vectors and common pt vectors.
//
// Keep these out of OptiX program code so deviceCode.cu can stay focused on
// tracing and shading logic.
// ======================================================================== //

#pragma once

#include "pt/math/vec.h"

#include <owl/common/math/vec.h>

using namespace owl;

namespace mypt {

  __device__ inline pt::Vec2f toPt(vec2f v)
  {
    return pt::Vec2f(v.x, v.y);
  }

  __device__ inline pt::Vec3f toPt(vec3f v)
  {
    return pt::Vec3f(v.x, v.y, v.z);
  }

  __device__ inline vec2f fromPt(pt::Vec2f v)
  {
    return vec2f(v.x, v.y);
  }

  __device__ inline vec3f fromPt(pt::Vec3f v)
  {
    return vec3f(v.x, v.y, v.z);
  }

} // namespace mypt

#pragma once

#include "../common/compiler.h"

#include <math.h>

namespace pt {

  constexpr float Pi = 3.14159265358979323846f;

  template <typename T>
  __both__ inline T square(T x) { return x * x; }

  __both__ inline float clamp(float x, float lo, float hi)
  {
    return fminf(fmaxf(x, lo), hi);
  }

  __both__ inline float safeSqrt(float x)
  {
    return sqrtf(fmaxf(0.f, x));
  }

  __both__ inline bool isInf(float x)
  {
#ifdef __CUDA_ARCH__
    return isinf(x);
#else
    return isinf(x) != 0;
#endif
  }

  __both__ inline float lerp(float a, float b, float t)
  {
    return a * (1.f - t) + b * t;
  }

} // namespace pt


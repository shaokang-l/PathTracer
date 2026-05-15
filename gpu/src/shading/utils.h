#pragma once
#include <owl/common/math/vec.h>
#include <owl/common/math/random.h>
// This file includes some utility math functions (e.g. sampling, PBR helper math)

namespace pt {
    using owl::vec3f;
    using owl::vec2f;
    using owl::normalize;
    using owl::cross;
    using owl::dot;
    using RNG = owl::common::LCG<4>;

  // ---------- helpers --------------------------------------------------

  __both__ inline vec3f sampleCosineHemisphere(const vec2f& u) {
    float r   = sqrtf(u.x);
    float phi = 2.f * float(M_PI) * u.y;
  
    float x = r * cosf(phi);
    float y = r * sinf(phi);
    float z = sqrtf(fmaxf(0.f, 1.f - u.x));
  
    return vec3f(x, y, z);
  }

  __both__ inline float pdfCosineHemisphere(float cosTheta) {
    return cosTheta / float(M_PI);
  }

  __both__ inline vec3f reflect(const vec3f &d, const vec3f &n)
  {
    return d - 2.f * dot(d, n) * n;
  }

  __both__ inline bool refract(const vec3f &d, const vec3f &n,
                                 float eta, vec3f &out)
  {
    const float cosI = -dot(d, n);
    const float k = 1.f - eta * eta * (1.f - cosI * cosI);
    if (k < 0.f) return false;
    out = normalize(eta * d + (eta * cosI - sqrtf(k)) * n);
    return true;
  }

  __both__ inline float fresnelSchlick(float cosTheta, float ior)
  {
    float r0 = (1.f - ior) / (1.f + ior);
    r0 = r0 * r0;
    return r0 + (1.f - r0) * powf(fmaxf(0.f, 1.f - cosTheta), 5.f);
  }

};
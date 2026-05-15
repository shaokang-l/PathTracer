#pragma once

#include <owl/common/math/vec.h>

namespace pt {
  using owl::vec3f;
  using owl::normalize;
  using owl::cross;
  using owl::dot;
  
  struct OrthoBasis {
    vec3f axis[3]; // axis[0] = T, axis[1] = B, axis[2] = N
  
    __both__ inline vec3f toWorld(const vec3f& v) const {
      return v.x * axis[0] + v.y * axis[1] + v.z * axis[2];
    }
  
    __both__ inline vec3f toLocal(const vec3f& v) const {
      return vec3f(dot(v, axis[0]),
                   dot(v, axis[1]),
                   dot(v, axis[2]));
    }
  };
  
  __both__ inline OrthoBasis makeOrthoBasis(const vec3f& n_) {
    OrthoBasis onb;
  
    vec3f n = normalize(n_);
  
    vec3f a = (fabsf(n.x) > 0.9f)
                ? vec3f(0.0f, 1.0f, 0.0f)
                : vec3f(1.0f, 0.0f, 0.0f);
  
    vec3f b = normalize(cross(n, a));
    vec3f t = cross(b, n);
  
    onb.axis[0] = t;
    onb.axis[1] = b;
    onb.axis[2] = n;
  
    return onb;
  }

  namespace local {
    __both__ inline float cosTheta   (vec3f w) { return w.z; }
    __both__ inline float cos2Theta  (vec3f w) { return w.z * w.z; }
    __both__ inline float absCosTheta(vec3f w) { return fabsf(w.z); }
    __both__ inline float sin2Theta  (vec3f w) { return fmaxf(0.f, 1.f - cos2Theta(w)); }
    __both__ inline float sinTheta   (vec3f w) { return sqrtf(sin2Theta(w)); }
    __both__ inline float tanTheta(vec3f w) {
      float c = cosTheta(w);
      return fabsf(c) > 1e-7f ? sinTheta(w) / c : 1e30f;
    }
    
    __both__ inline float tan2Theta(vec3f w) {
      float c2 = cos2Theta(w);
      return c2 > 1e-14f ? sin2Theta(w) / c2 : 1e30f;
    }
    __both__ inline bool  sameHemisphere(vec3f w1, vec3f w2) { return w1.z * w2.z > 0.f; }
  } // namespace local

} // namespace pt
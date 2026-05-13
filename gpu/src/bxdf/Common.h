#pragma once

#include "../../../common/include/pt/common/compiler.h"
#include "../utils.h"

#include "../../../common/include/pt/material/fresnel.h"
#include "../../../common/include/pt/material/trowbridge_reitz.h"

namespace mypt {

  __both__ inline pt::Vec2f toPt(const vec2f &v)
  {
    return pt::Vec2f(v.x, v.y);
  }

  __both__ inline pt::Vec3f toPt(const vec3f &v)
  {
    return pt::Vec3f(v.x, v.y, v.z);
  }

  __both__ inline vec3f fromPt(const pt::Vec3f &v)
  {
    return vec3f(v.x, v.y, v.z);
  }

  __both__ inline float absCosTheta(const vec3f &w)
  {
    return fabsf(w.z);
  }

  __both__ inline bool sameHemisphere(const vec3f &a, const vec3f &b)
  {
    return a.z * b.z > 0.f;
  }

  __both__ inline vec3f reflectLocal(const vec3f &wo, const vec3f &wm)
  {
    return fromPt(pt::local::reflect(toPt(wo), toPt(wm)));
  }

  __both__ inline bool refractLocal(const vec3f &wo,
                                    const vec3f &wm,
                                    float        eta,
                                    float       &etap,
                                    vec3f       &wi)
  {
    pt::Vec3f wt;
    const bool ok = pt::local::refract(toPt(wo), toPt(wm), eta, etap, wt);
    wi = fromPt(wt);
    return ok;
  }

  __both__ inline bool effectivelySmooth(float alphaX, float alphaY)
  {
    return fmaxf(alphaX, alphaY) < 1e-3f;
  }

} // namespace mypt


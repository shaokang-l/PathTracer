// ======================================================================== //
// bxdf/Dielectric.h - smooth glass / fresnel-weighted reflect+refract.
//
// STUB: not implemented yet. Sample_f returns false so the integrator
// terminates the path on a dielectric hit (object renders black).
// Implement Fresnel + total-internal-reflection branch later.
// ======================================================================== //

#pragma once

#include "../bsdfSample.h"
#include "../bxdfFlags.h"
#include "../orthoBasis.h"
#include "../utils.h"

namespace mypt {

  struct DielectricBxDF {
    float ior;

    __both__ inline bool Sample_f(const vec3f &/*wo*/,
                                  float        /*uc*/,
                                  vec2f        /*u*/,
                                  BSDFSample  &/*out*/) const
    {
      // TODO: Fresnel-weighted reflection vs refraction
      return false;
    }

    __both__ inline vec3f f(const vec3f &/*wo*/, const vec3f &/*wi*/) const
    {
      return vec3f(0.f);
    }

    __both__ inline float pdf(const vec3f &/*wo*/, const vec3f &/*wi*/) const
    {
      return 0.f;
    }
  };

} // namespace mypt

// ======================================================================== //
// bxdf/ThinDielectric.h - thin Fresnel sheet without volume tracking.
// ======================================================================== //

#pragma once

#include "Common.h"
#include "../bsdfSample.h"
#include "../bxdfFlags.h"

namespace pt {

  struct ThinDielectricBxDF {
    float ior;

    __both__ inline bool Sample_f(const vec3f &wo,
                                  float        uc,
                                  vec2f        /*u*/,
                                  BSDFSample  &out) const
    {
      if (wo.z == 0.f || ior == 1.f) return false;

      float R = pt::fresnelDielectric(fabsf(wo.z), ior);
      if (R < 1.f) {
        const float T = 1.f - R;
        R += pt::square(T) * R / (1.f - pt::square(R));
      }
      const float T = 1.f - R;

      if (uc < R) {
        out.wi = vec3f(-wo.x, -wo.y, wo.z);
        out.pdf = R;
        out.f = vec3f(R / fmaxf(1e-6f, absCosTheta(out.wi)));
        out.flag = BxDFFlags::SPECULAR_REFLECTION;
        return true;
      }

      out.wi = -wo;
      out.pdf = T;
      out.f = vec3f(T / fmaxf(1e-6f, absCosTheta(out.wi)));
      out.flag = BxDFFlags::SPECULAR_TRANSMISSION;
      return true;
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

} // namespace pt


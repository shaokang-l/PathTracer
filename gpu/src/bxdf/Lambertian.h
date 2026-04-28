// ======================================================================== //
// bxdf/Lambertian.h - perfectly diffuse BxDF.
//
// Local space (n = +z). Operates entirely in the shading frame; the
// world<->local conversion is handled by the BSDF wrapper.
// ======================================================================== //

#pragma once

#include "../bsdfSample.h"
#include "../bxdfFlags.h"
#include "../orthoBasis.h"
#include "../utils.h"

namespace mypt {

  struct LambertianBxDF {
    vec3f albedo;

    __both__ inline bool Sample_f(const vec3f &wo,
                                  float        uc,
                                  vec2f        u,
                                  BSDFSample  &out) const
    {
      out.wi   = sampleCosineHemisphere(u);
      out.f    = albedo / float(M_PI);
      out.pdf  = pdfCosineHemisphere(local::absCosTheta(out.wi));
      out.flag = BxDFFlags::DIFFUSE_REFLECTION;
      return true;
    }

    __both__ inline vec3f f(const vec3f &wo, const vec3f &wi) const
    {
      if (!local::sameHemisphere(wo, wi)) return vec3f(0.f);
      return albedo / float(M_PI);
    }

    __both__ inline float pdf(const vec3f &wo, const vec3f &wi) const
    {
      if (!local::sameHemisphere(wo, wi)) return 0.f;
      return pdfCosineHemisphere(local::absCosTheta(wi));
    }
  };

} // namespace mypt

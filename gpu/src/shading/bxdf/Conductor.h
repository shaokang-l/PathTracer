// ======================================================================== //
// bxdf/Conductor.h - smooth/rough metallic microfacet reflector.
// ======================================================================== //

#pragma once

#include "Common.h"
#include "../bsdfSample.h"
#include "../bxdfFlags.h"

namespace pt {

  struct ConductorBxDF {
    vec3f eta;
    vec3f k;
    vec3f albedo;
    float alphaX;
    float alphaY;

    __both__ inline bool Sample_f(const vec3f &wo,
                                  float        /*uc*/,
                                  vec2f        u,
                                  BSDFSample  &out) const
    {
      if (wo.z == 0.f) return false;

      if (effectivelySmooth(alphaX, alphaY)) {
        out.wi   = vec3f(-wo.x, -wo.y, wo.z);
        out.pdf  = 1.f;
        out.f    = (albedo * fromPt(pt::fresnelComplex(absCosTheta(out.wi),
                                                       toPt(eta),
                                                       toPt(k)))) /
                   absCosTheta(out.wi);
        out.flag = BxDFFlags::SPECULAR_REFLECTION;
        return true;
      }

      const pt::TrowbridgeReitzDistribution distrib(alphaX, alphaY);
      const vec3f wm = fromPt(distrib.sample_wm(toPt(wo), toPt(u)));
      const vec3f wi = reflectLocal(wo, wm);
      if (!sameHemisphere(wo, wi) || wi.z == 0.f) return false;

      out.wi = wi;
      out.pdf = pdf(wo, wi);
      if (out.pdf <= 0.f) return false;
      out.f = f(wo, wi);
      out.flag = BxDFFlags::GLOSSY_REFLECTION;
      return true;
    }

    __both__ inline vec3f f(const vec3f &wo, const vec3f &wi) const
    {
      if (!sameHemisphere(wo, wi) || wo.z == 0.f || wi.z == 0.f)
        return vec3f(0.f);
      if (effectivelySmooth(alphaX, alphaY)) return vec3f(0.f);

      const pt::TrowbridgeReitzDistribution distrib(alphaX, alphaY);
      const vec3f wm = normalize(wo + wi);
      const float cosWoWm = fabsf(dot(wo, wm));
      const float denom = 4.f * absCosTheta(wo) * absCosTheta(wi);
      if (denom <= 0.f) return vec3f(0.f);

      const vec3f F = fromPt(pt::fresnelComplex(cosWoWm, toPt(eta), toPt(k)));
      return albedo * F * (distrib.D(toPt(wm)) *
                           distrib.G(toPt(wo), toPt(wi)) / denom);
    }

    __both__ inline float pdf(const vec3f &wo, const vec3f &wi) const
    {
      if (!sameHemisphere(wo, wi) || effectivelySmooth(alphaX, alphaY))
        return 0.f;

      const pt::TrowbridgeReitzDistribution distrib(alphaX, alphaY);
      const vec3f wm = normalize(wo + wi);
      const float denom = 4.f * fabsf(dot(wo, wm));
      if (denom <= 1e-6f) return 0.f;
      return distrib.PDF(toPt(wo), toPt(wm)) / denom;
    }
  };

} // namespace pt


// ======================================================================== //
// bxdf/Dielectric.h - smooth/rough Fresnel dielectric.
// ======================================================================== //

#pragma once

#include "Common.h"
#include "../bsdfSample.h"
#include "../bxdfFlags.h"

namespace pt {

  struct DielectricBxDF {
    float ior;
    float alphaX;
    float alphaY;

    __both__ inline bool Sample_f(const vec3f &wo,
                                  float        uc,
                                  vec2f        u,
                                  BSDFSample  &out) const
    {
      if (wo.z == 0.f || ior == 1.f) return false;

      if (effectivelySmooth(alphaX, alphaY)) {
        const float R = pt::fresnelDielectric(wo.z, ior);
        const float T = 1.f - R;

        if (uc < R) {
          out.wi = vec3f(-wo.x, -wo.y, wo.z);
          out.pdf = R;
          out.f = vec3f(R / fmaxf(1e-6f, absCosTheta(out.wi)));
          out.flag = BxDFFlags::SPECULAR_REFLECTION;
          return true;
        }

        float etap = 1.f;
        vec3f wi;
        if (!refractLocal(wo, vec3f(0.f, 0.f, 1.f), ior, etap, wi))
          return false;

        out.wi = wi;
        out.pdf = T;
        out.f = vec3f((T / (etap * etap)) /
                      fmaxf(1e-6f, absCosTheta(out.wi)));
        out.flag = BxDFFlags::SPECULAR_TRANSMISSION;
        return true;
      }

      const pt::TrowbridgeReitzDistribution distrib(alphaX, alphaY);
      const vec3f wm = fromPt(distrib.sample_wm(toPt(wo), toPt(u)));
      const float R = pt::fresnelDielectric(dot(wo, wm), ior);
      const float T = 1.f - R;

      if (uc < R) {
        const vec3f wi = reflectLocal(wo, wm);
        if (!sameHemisphere(wo, wi)) return false;

        out.wi = wi;
        out.pdf = pdf(wo, wi);
        if (out.pdf <= 0.f) return false;
        out.f = f(wo, wi);
        out.flag = BxDFFlags::GLOSSY_REFLECTION;
        return true;
      }

      float etap = 1.f;
      vec3f wi;
      if (T <= 0.f || !refractLocal(wo, wm, ior, etap, wi) ||
          sameHemisphere(wo, wi)) {
        return false;
      }

      out.wi = wi;
      out.pdf = pdf(wo, wi);
      if (out.pdf <= 0.f) return false;
      out.f = f(wo, wi);
      out.flag = BxDFFlags::GLOSSY_TRANSMISSION;
      return true;
    }

    __both__ inline vec3f f(const vec3f &wo, const vec3f &wi) const
    {
      if (ior == 1.f || effectivelySmooth(alphaX, alphaY)) return vec3f(0.f);

      const float cosThetaO = wo.z;
      const float cosThetaI = wi.z;
      const bool reflect = cosThetaI * cosThetaO > 0.f;

      float etap = 1.f;
      if (!reflect) etap = cosThetaO > 0.f ? ior : (1.f / ior);

      vec3f wm = reflect ? (wi + wo) : (wi * etap + wo);
      if (cosThetaI == 0.f || cosThetaO == 0.f || dot(wm, wm) == 0.f)
        return vec3f(0.f);

      wm = fromPt(pt::faceForward(pt::normalize(toPt(wm)), pt::Vec3f(0.f, 0.f, 1.f)));
      if (dot(wm, wi) * cosThetaI < 0.f ||
          dot(wm, wo) * cosThetaO < 0.f)
        return vec3f(0.f);

      const pt::TrowbridgeReitzDistribution distrib(alphaX, alphaY);
      const float F = pt::fresnelDielectric(dot(wo, wm), ior);
      const float D = distrib.D(toPt(wm));
      const float G = distrib.G(toPt(wo), toPt(wi));

      if (reflect) {
        return vec3f(D * G * F / fabsf(4.f * cosThetaI * cosThetaO));
      }

      const float denom = pt::square(dot(wi, wm) + dot(wo, wm) / etap) *
                          cosThetaI * cosThetaO;
      if (denom == 0.f) return vec3f(0.f);

      float ft = D * (1.f - F) * G *
                 fabsf(dot(wi, wm) * dot(wo, wm) / denom);
      ft /= pt::square(etap);
      return vec3f(ft);
    }

    __both__ inline float pdf(const vec3f &wo, const vec3f &wi) const
    {
      if (ior == 1.f || effectivelySmooth(alphaX, alphaY) || wi.z == 0.f)
        return 0.f;

      const float cosThetaO = wo.z;
      const float cosThetaI = wi.z;
      const bool reflect = cosThetaI * cosThetaO > 0.f;
      float etap = 1.f;
      if (!reflect) etap = cosThetaO > 0.f ? ior : (1.f / ior);

      vec3f wm = reflect ? (wi + wo) : (wi * etap + wo);
      if (cosThetaI == 0.f || cosThetaO == 0.f || dot(wm, wm) == 0.f)
        return 0.f;

      wm = fromPt(pt::faceForward(pt::normalize(toPt(wm)), pt::Vec3f(0.f, 0.f, 1.f)));
      if (dot(wm, wi) * cosThetaI < 0.f ||
          dot(wm, wo) * cosThetaO < 0.f)
        return 0.f;

      const pt::TrowbridgeReitzDistribution distrib(alphaX, alphaY);
      const float R = pt::fresnelDielectric(dot(wo, wm), ior);
      const float T = 1.f - R;

      if (reflect) {
        const float denom = 4.f * fabsf(dot(wo, wm));
        return denom > 0.f ? distrib.PDF(toPt(wo), toPt(wm)) * R / denom : 0.f;
      }

      const float denom = pt::square(dot(wi, wm) + dot(wo, wm) / etap);
      if (denom <= 0.f) return 0.f;
      const float dwmDwi = fabsf(dot(wi, wm)) / denom;
      return distrib.PDF(toPt(wo), toPt(wm)) * dwmDwi * T;
    }
  };

} // namespace pt

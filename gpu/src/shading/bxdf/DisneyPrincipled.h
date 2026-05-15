// ======================================================================== //
// bxdf/DisneyPrincipled.h - GPU Disney Principled BSDF.
// ======================================================================== //

#pragma once

#include "Common.h"
#include "../bsdfSample.h"
#include "../bxdfFlags.h"

#include "pt/material/disney.h"

namespace pt {

  struct DisneyPrincipledBxDF {
    vec3f baseColor;
    float specularTransmission;
    float metallic;
    float subsurface;
    float specular;
    float roughness;
    float specularTint;
    float anisotropic;
    float sheen;
    float sheenTint;
    float clearcoat;
    float clearcoatGloss;
    float eta;

    __both__ inline float diffuseWeight() const
    {
      return (1.f - metallic) * (1.f - specularTransmission);
    }

    __both__ inline float metalWeight() const
    {
      return 1.f - specularTransmission * (1.f - metallic);
    }

    __both__ inline float glassWeight() const
    {
      return (1.f - metallic) * specularTransmission;
    }

    __both__ inline float clearcoatWeight() const
    {
      return 0.25f * clearcoat;
    }

    __both__ inline float sheenWeight() const
    {
      return (1.f - metallic) * sheen;
    }

    __both__ inline float sampleDiscrete4(float u,
                                          float w0,
                                          float w1,
                                          float w2,
                                          float w3,
                                          int   &idx) const
    {
      const float sum = w0 + w1 + w2 + w3;
      if (sum <= 0.f) {
        idx = -1;
        return 0.f;
      }

      float x = fminf(u * sum, sum * (1.f - 1e-6f));
      if (x < w0) {
        idx = 0;
        return w0 > 0.f ? x / w0 : 0.f;
      }
      x -= w0;
      if (x < w1) {
        idx = 1;
        return w1 > 0.f ? x / w1 : 0.f;
      }
      x -= w1;
      if (x < w2) {
        idx = 2;
        return w2 > 0.f ? x / w2 : 0.f;
      }
      x -= w2;
      idx = 3;
      return w3 > 0.f ? x / w3 : 0.f;
    }

    __both__ inline float cosinePdf(const vec3f &wi) const
    {
      return wi.z > 0.f ? wi.z / pt::Pi : 0.f;
    }

    __both__ inline vec3f sampleCosine(vec2f u) const
    {
      return sampleCosineHemisphere(u);
    }

    __both__ inline pt::TrowbridgeReitzDistribution tr() const
    {
      return pt::disneyTR(roughness, anisotropic);
    }

    __both__ inline vec3f diffuseF(const vec3f &wo, const vec3f &wi) const
    {
      if (wo.z <= 0.f || wi.z <= 0.f) return vec3f(0.f);

      const float absWi = fabsf(wi.z);
      const float absWo = fabsf(wo.z);
      const float fd90 = pt::computeFd90(toPt(wo), toPt(wi), roughness);
      vec3f baseDiff = baseColor * (1.f / pt::Pi);
      baseDiff = baseDiff * (pt::hackedSchlick(absWi, fd90) *
                             pt::hackedSchlick(absWo, fd90));

      const float fss90 = pt::computeFss90(toPt(wo), toPt(wi), roughness);
      vec3f baseSubsurface = 1.25f * baseColor * (1.f / pt::Pi);
      baseSubsurface = baseSubsurface *
        (pt::hackedSchlick(absWi, fss90) *
         pt::hackedSchlick(absWo, fss90) *
         (1.f / fmaxf(1e-6f, absWi + absWo) - 0.5f) + 0.5f);

      return (1.f - subsurface) * baseDiff + subsurface * baseSubsurface;
    }

    __both__ inline vec3f metalF(const vec3f &wo, const vec3f &wi) const
    {
      if (wo.z <= 0.f || wi.z <= 0.f) return vec3f(0.f);

      const pt::TrowbridgeReitzDistribution d = tr();
      if (d.effectivelySmooth()) return vec3f(0.f);

      const vec3f wh = normalize(wo + wi);
      const float dotWhWi = fabsf(dot(wh, wi));
      const pt::Vec3f tint = pt::tintColor(toPt(baseColor));
      const pt::Vec3f ks = pt::Vec3f(1.f - specularTint) + specularTint * tint;
      const pt::Vec3f r0 = pt::Vec3f(pt::etaToR0(eta));
      const pt::Vec3f c0 =
        specular * r0 * (1.f - metallic) * ks + metallic * toPt(baseColor);
      const vec3f F = fromPt(pt::fresnelSchlick(dotWhWi, c0));

      const float denom = fabsf(4.f * wo.z * wi.z);
      if (denom <= 0.f) return vec3f(0.f);

      return F * (d.D(toPt(wh)) * d.G(toPt(wo), toPt(wi)) / denom);
    }

    __both__ inline float metalPdf(const vec3f &wo, const vec3f &wi) const
    {
      if (wo.z <= 0.f || wi.z <= 0.f) return 0.f;
      const pt::TrowbridgeReitzDistribution d = tr();
      if (d.effectivelySmooth()) return 0.f;
      const vec3f wh = normalize(wo + wi);
      const float denom = 4.f * fabsf(dot(wo, wh));
      return denom > 1e-6f ? d.PDF(toPt(wo), toPt(wh)) / denom : 0.f;
    }

    __both__ inline vec3f glassF(const vec3f &wo, const vec3f &wi) const
    {
      if (eta == 1.f) return vec3f(0.f);
      const pt::TrowbridgeReitzDistribution d = tr();
      if (d.effectivelySmooth()) return vec3f(0.f);

      const float cosO = wo.z;
      const float cosI = wi.z;
      const bool reflect = cosI * cosO > 0.f;
      float etap = 1.f;
      if (!reflect) etap = cosO > 0.f ? eta : (1.f / eta);

      vec3f wm = reflect ? (wi + wo) : (wi * etap + wo);
      if (cosI == 0.f || cosO == 0.f || dot(wm, wm) == 0.f) return vec3f(0.f);
      wm = fromPt(pt::faceForward(pt::normalize(toPt(wm)), pt::Vec3f(0.f, 0.f, 1.f)));
      if (dot(wm, wi) * cosI < 0.f || dot(wm, wo) * cosO < 0.f) return vec3f(0.f);

      const float F = pt::fresnelDielectric(dot(wo, wm), eta);
      if (reflect) {
        const float denom = fabsf(4.f * cosI * cosO);
        return denom > 0.f
          ? baseColor * (d.D(toPt(wm)) * d.G(toPt(wo), toPt(wi)) * F / denom)
          : vec3f(0.f);
      }

      const float denom = pt::square(dot(wi, wm) + dot(wo, wm) / etap) *
                          cosI * cosO;
      if (denom == 0.f) return vec3f(0.f);
      const vec3f sqrtColor(sqrtf(baseColor.x),
                            sqrtf(baseColor.y),
                            sqrtf(baseColor.z));
      vec3f ft = sqrtColor * d.D(toPt(wm)) * (1.f - F) *
                 d.G(toPt(wo), toPt(wi)) *
                 fabsf(dot(wi, wm) * dot(wo, wm) / denom);
      ft = ft / pt::square(etap);
      return ft;
    }

    __both__ inline float glassPdf(const vec3f &wo, const vec3f &wi) const
    {
      if (eta == 1.f || wi.z == 0.f) return 0.f;
      const pt::TrowbridgeReitzDistribution d = tr();
      if (d.effectivelySmooth()) return 0.f;

      const float cosO = wo.z;
      const float cosI = wi.z;
      const bool reflect = cosI * cosO > 0.f;
      float etap = 1.f;
      if (!reflect) etap = cosO > 0.f ? eta : (1.f / eta);

      vec3f wm = reflect ? (wi + wo) : (wi * etap + wo);
      if (cosI == 0.f || cosO == 0.f || dot(wm, wm) == 0.f) return 0.f;
      wm = fromPt(pt::faceForward(pt::normalize(toPt(wm)), pt::Vec3f(0.f, 0.f, 1.f)));
      if (dot(wm, wi) * cosI < 0.f || dot(wm, wo) * cosO < 0.f) return 0.f;

      const float R = pt::fresnelDielectric(dot(wo, wm), eta);
      const float T = 1.f - R;
      if (reflect) {
        const float denom = 4.f * fabsf(dot(wo, wm));
        return denom > 0.f ? d.PDF(toPt(wo), toPt(wm)) * R / denom : 0.f;
      }

      const float denom = pt::square(dot(wi, wm) + dot(wo, wm) / etap);
      const float dwmDwi = denom > 0.f ? fabsf(dot(wi, wm)) / denom : 0.f;
      return d.PDF(toPt(wo), toPt(wm)) * dwmDwi * T;
    }

    __both__ inline vec3f clearcoatF(const vec3f &wo, const vec3f &wi) const
    {
      if (wo.z <= 0.f || wi.z <= 0.f) return vec3f(0.f);
      const vec3f wh = normalize(wo + wi);
      const pt::ClearcoatDistribution d(clearcoatGloss);
      const float denom = fabsf(4.f * wo.z * wi.z);
      if (denom <= 0.f) return vec3f(0.f);
      return vec3f(d.F(fabsf(dot(wh, wi))) *
                   d.D(toPt(wh)) *
                   d.G(toPt(wo), toPt(wi)) / denom);
    }

    __both__ inline float clearcoatPdf(const vec3f &wo, const vec3f &wi) const
    {
      if (wo.z <= 0.f || wi.z <= 0.f) return 0.f;
      const vec3f wh = normalize(wo + wi);
      const float denom = 4.f * fabsf(dot(wo, wh));
      if (denom <= 1e-6f) return 0.f;
      return pt::ClearcoatDistribution(clearcoatGloss).PDF(toPt(wh)) / denom;
    }

    __both__ inline vec3f sheenF(const vec3f &wo, const vec3f &wi) const
    {
      if (wo.z <= 0.f || wi.z <= 0.f) return vec3f(0.f);
      const vec3f wh = normalize(wo + wi);
      const pt::Vec3f tint = pt::tintColor(toPt(baseColor));
      const vec3f cSheen = fromPt(pt::Vec3f(1.f - sheenTint) + sheenTint * tint);
      return cSheen * powf(fmaxf(0.f, 1.f - fabsf(dot(wh, wi))), 5.f);
    }

    __both__ inline vec3f f(const vec3f &wo, const vec3f &wi) const
    {
      vec3f result(0.f);
      result += diffuseWeight() * diffuseF(wo, wi);
      result += metalWeight() * metalF(wo, wi);
      result += glassWeight() * glassF(wo, wi);
      result += clearcoatWeight() * clearcoatF(wo, wi);
      result += sheenWeight() * sheenF(wo, wi);
      return result;
    }

    __both__ inline float pdf(const vec3f &wo, const vec3f &wi) const
    {
      const float wd = diffuseWeight();
      const float wm = metalWeight();
      const float wg = glassWeight();
      const float wc = clearcoatWeight();
      const float sum = wd + wm + wg + wc;
      if (sum <= 0.f) return 0.f;

      return (wd * cosinePdf(wi) +
              wm * metalPdf(wo, wi) +
              wg * glassPdf(wo, wi) +
              wc * clearcoatPdf(wo, wi)) / sum;
    }

    __both__ inline bool Sample_f(const vec3f &wo,
                                  float        uc,
                                  vec2f        u,
                                  BSDFSample  &out) const
    {
      const float wd = diffuseWeight();
      const float wm = metalWeight();
      const float wg = glassWeight();
      const float wc = clearcoatWeight();
      int lobe = -1;
      const float lobeUc = sampleDiscrete4(uc, wd, wm, wg, wc, lobe);
      if (lobe < 0) return false;

      const float sum = wd + wm + wg + wc;
      const float pmf =
        (lobe == 0 ? wd : (lobe == 1 ? wm : (lobe == 2 ? wg : wc))) / sum;
      const vec2f remappedU(lobeUc, u.y);

      if (lobe == 0) {
        out.wi = sampleCosine(remappedU);
        out.flag = BxDFFlags::DIFFUSE_REFLECTION;
      } else if (lobe == 1) {
        const pt::TrowbridgeReitzDistribution d = tr();
        if (d.effectivelySmooth()) {
          out.wi = vec3f(-wo.x, -wo.y, wo.z);
          out.flag = BxDFFlags::SPECULAR_REFLECTION;
          if (out.wi.z <= 0.f) return false;
          const pt::Vec3f tint = pt::tintColor(toPt(baseColor));
          const pt::Vec3f ks = pt::Vec3f(1.f - specularTint) + specularTint * tint;
          const pt::Vec3f r0 = pt::Vec3f(pt::etaToR0(eta));
          const pt::Vec3f c0 =
            specular * r0 * (1.f - metallic) * ks + metallic * toPt(baseColor);
          out.pdf = fmaxf(1e-6f, pmf);
          out.f = (metalWeight() * fromPt(pt::fresnelSchlick(absCosTheta(out.wi), c0))) /
                  fmaxf(1e-6f, absCosTheta(out.wi));
          return true;
        } else {
          const vec3f wh = fromPt(d.sample_wm(toPt(wo), toPt(remappedU)));
          out.wi = reflectLocal(wo, wh);
          out.flag = BxDFFlags::GLOSSY_REFLECTION;
        }
      } else if (lobe == 2) {
        const pt::TrowbridgeReitzDistribution d = tr();
        if (d.effectivelySmooth()) {
          const float R = pt::fresnelDielectric(wo.z, eta);
          if (lobeUc < R) {
            out.wi = vec3f(-wo.x, -wo.y, wo.z);
            out.flag = BxDFFlags::SPECULAR_REFLECTION;
            out.pdf = fmaxf(1e-6f, pmf * R);
            out.f = (glassWeight() * R * baseColor) /
                    fmaxf(1e-6f, absCosTheta(out.wi));
            return true;
          } else {
            float etap = 1.f;
            if (!refractLocal(wo, vec3f(0.f, 0.f, 1.f), eta, etap, out.wi))
              return false;
            out.flag = BxDFFlags::SPECULAR_TRANSMISSION;
            const float T = 1.f - R;
            const vec3f sqrtColor(sqrtf(baseColor.x),
                                  sqrtf(baseColor.y),
                                  sqrtf(baseColor.z));
            out.pdf = fmaxf(1e-6f, pmf * T);
            out.f = (glassWeight() * T * sqrtColor / pt::square(etap)) /
                    fmaxf(1e-6f, absCosTheta(out.wi));
            return true;
          }
        } else {
          const vec3f wh = fromPt(d.sample_wm(toPt(wo), toPt(remappedU)));
          const float R = pt::fresnelDielectric(dot(wo, wh), eta);
          if (lobeUc < R) {
            out.wi = reflectLocal(wo, wh);
            out.flag = BxDFFlags::GLOSSY_REFLECTION;
          } else {
            float etap = 1.f;
            if (!refractLocal(wo, wh, eta, etap, out.wi)) return false;
            out.flag = BxDFFlags::GLOSSY_TRANSMISSION;
          }
        }
      } else {
        const pt::ClearcoatDistribution d(clearcoatGloss);
        const vec3f wh = fromPt(d.sample_h(toPt(remappedU)));
        out.wi = reflectLocal(wo, wh);
        out.flag = BxDFFlags::GLOSSY_REFLECTION;
      }

      out.pdf = pdf(wo, out.wi);
      if (out.pdf <= 0.f) {
        if (isSpecular(out.flag)) {
          out.pdf = fmaxf(1e-6f, pmf);
        } else {
          return false;
        }
      }

      out.f = f(wo, out.wi);
      if (isSpecular(out.flag)) {
        out.f = out.f / fmaxf(1e-6f, absCosTheta(out.wi));
      }
      return true;
    }
  };

} // namespace pt


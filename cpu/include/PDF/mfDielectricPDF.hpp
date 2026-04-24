#pragma once
#include "material/material.hpp"
#include "PDF/pdf.hpp"
#include "utils/orthoBasis.hpp"

extern int rejects;
class MFDielectricPDF : public PDF
{
private:
  const TrowbridgeReitzDistribution &distrib;
  const OrthoBasis onb;
  const gl::vec3 wo;
  const float eta;
  const BxDFReflTransFlags flags;
  const TransportMode mode;

public:
  MFDielectricPDF(const TrowbridgeReitzDistribution &d, const OrthoBasis &basis,
                  const gl::vec3 &wo_world, float eta,
                  BxDFReflTransFlags sampleFlags, TransportMode mode)
      : distrib(d), onb(basis), wo(onb.toLocal(wo_world.normalize())), eta(eta),
        flags(sampleFlags), mode(mode) {}

  // Sample a direction (wi) given randoms uc (for R/T) and u (for microfacet m)
  gl::vec3 get(float uc = gl::rand_num(),
               gl::vec2 u = gl::vec2(gl::rand_num(),
                                     gl::rand_num())) const override
  {
    // 1) sample half‐vector in local space
    gl::vec3 m = distrib.sample_wm(wo, u);
    // 2) Fresnel
    float R = gl::fresnelDielectric(dot(wo, m), eta), T = 1 - R;
    // respect flags
    float pr = !!(flags & BxDFReflTransFlags::Reflection) ? R : 0.f,
          pt = !!(flags & BxDFReflTransFlags::Transmission) ? T : 0.f;
    if (pr + pt == 0)
      return {};

    // 3) pick R vs T
    bool doRefl = uc < pr / (pr + pt);
    if (doRefl)
    {
      // reflect
      gl::vec3 wi_local = gl::pbrt::reflect(wo, m);
      // if below hemisphere, discard

      if (!gl::pbrt::sameHemisphere(wo, wi_local))
#ifdef BIASED_SAMPLING
        wi_local.z() = -wi_local.z();
#elif defined DISCARD_SAMPLING
        return {};
#else
        do
        {
          rejects++;
          using namespace gl;
          vec2 u2 = vec2(rand_num(), rand_num());
          vec3 m = distrib.sample_wm(wo, u2);
          wi_local = gl::pbrt::reflect(wo, m);
        } while (wi_local.z() <= 0);
#endif
      return onb.toWorld(wi_local);
    }
    else
    {
      // refract
      gl::vec3 wi_local;
      float etaScale;
      bool valid = gl::pbrt::refract(wo, m, eta, etaScale, wi_local);
      if (!valid || gl::pbrt::sameHemisphere(wo, wi_local))
#ifdef BIASED_SAMPLING
        wi_local = gl::pbrt::reflect(wo, m);
#elif defined DISCARD_SAMPLING
        return {};
#else
        do
        {
          rejects++;
          using namespace gl;
          // 1) draw m
          vec2 u2 = vec2(rand_num(), rand_num());
          vec3 m = distrib.sample_wm(wo, u2);

          // 2) refract
          float etaScale;
          bool valid = pbrt::refract(wo, m, eta, etaScale, wi_local);
          // 3) break if it's valid
          // 3) break if it’s valid
          if (valid && !pbrt::sameHemisphere(wo, wi_local))
          {
            break;
          }
          // otherwise loop (waste one draw, but keep your SPP budget)
        } while (true);
#endif
      return onb.toWorld(wi_local);
    }
  }

  // Evaluate the PDF for a given direction wi_world
  float at(const gl::vec3 &wi_world) const override
  {

    using namespace gl;
    // delta or same ior
    if (eta == 1 || distrib.effectivelySmooth())
      return 0.f;

    // 1) transform to local space
    vec3 wi = onb.toLocal(wi_world.normalize());
    // must be on one of the two hemispheres
    if (wi.z() == 0)
      return 0;

    float cosTheta_o = pbrt::cosTheta(wo), cosTheta_i = pbrt::cosTheta(wi);
    bool reflect = cosTheta_i * cosTheta_o > 0;
    float etap = 1;
    if (!reflect)
      etap = cosTheta_o > 0 ? eta : (1 / eta);
    vec3 wm = wi * etap + wo;
    if (cosTheta_i == 0 || cosTheta_o == 0 || wm.length() == 0)
      return {};
    wm = pbrt::faceForward(normalize(wm), vec3(0, 0, 1));

    // 3) discard back‐facing microfacets
    if (dot(wm, wi) * cosTheta_i < 0 || dot(wm, wo) * wo.z() < 0)
      return 0.f;

    // 4) Fresnel + flags
    float R = gl::fresnelDielectric(dot(wo, wm), eta), T = 1 - R;
    float pr = !!(flags & BxDFReflTransFlags::Reflection) ? R : 0.f,
          pt = !!(flags & BxDFReflTransFlags::Transmission) ? T : 0.f;
    if (pr + pt == 0)
      return 0.f;

    float pdf = 0.f;
    if (reflect)
      pdf = distrib.PDF(wo, wm) / (4.f * std::fabs(dot(wo, wm))) * R / (R + T);
    else
    {
      float denom = square(dot(wi, wm) + dot(wo, wm) / etap);
      float dwm_dwi = std::fabs(dot(wi, wm)) / denom;
      pdf = distrib.PDF(wo, wm) * dwm_dwi * T / (R + T);
    }
    return pdf;
  }
};

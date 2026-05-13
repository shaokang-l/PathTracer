#pragma once

#include "fresnel.h"
#include "trowbridge_reitz.h"

namespace pt {

  __both__ inline float luminance(Vec3f c)
  {
    return 0.2126f * c.x + 0.7152f * c.y + 0.0722f * c.z;
  }

  __both__ inline float hackedSchlick(float absCosTheta, float f90)
  {
    return 1.f + (f90 - 1.f) * powf(fmaxf(0.f, 1.f - absCosTheta), 5.f);
  }

  __both__ inline float computeFd90(Vec3f wo, Vec3f wi, float roughness)
  {
    const Vec3f wh = normalize(wo + wi);
    const float whWi = dot(wh, wi);
    return 0.5f + 2.f * roughness * square(fabsf(whWi));
  }

  __both__ inline float computeFss90(Vec3f wo, Vec3f wi, float roughness)
  {
    const Vec3f wh = normalize(wo + wi);
    const float whWi = dot(wh, wi);
    return roughness * square(fabsf(whWi));
  }

  __both__ inline Vec3f tintColor(Vec3f baseColor)
  {
    const float lum = luminance(baseColor);
    return lum > 0.f ? baseColor / lum : Vec3f(1.f);
  }

  __both__ inline TrowbridgeReitzDistribution disneyTR(float roughness,
                                                       float anisotropic)
  {
    anisotropic = clamp(anisotropic, 0.f, 1.f / 0.9f - 0.00001f);
    float aspect = safeSqrt(1.f - 0.9f * anisotropic);
    if (aspect == 0.f) aspect = 0.0001f;

    const float alphaX = fmaxf(0.0001f, square(roughness) / aspect);
    const float alphaY = fmaxf(0.0001f, square(roughness) * aspect);
    return TrowbridgeReitzDistribution(alphaX, alphaY);
  }

  struct ClearcoatDistribution {
    float alpha;

    __both__ ClearcoatDistribution() : alpha(0.001f) {}
    __both__ explicit ClearcoatDistribution(float clearcoatGloss)
      : alpha((1.f - clearcoatGloss) * 0.1f + clearcoatGloss * 0.001f) {}

    __both__ inline float effectivelySmooth() const { return alpha < 1e-3f; }

    __both__ inline float D(Vec3f h) const
    {
      if (alpha <= 0.f) return 0.f;
      const float a2 = square(alpha);
      const float lower = Pi * logf(a2);
      const float term = 1.f + (a2 - 1.f) * square(h.z);
      return (a2 - 1.f) / (lower * term);
    }

    __both__ inline float lambda(Vec3f w) const
    {
      const float tan2Theta = local::tan2Theta(w);
      if (isInf(tan2Theta)) return 0.f;
      const float alpha2 =
        square(local::cosPhi(w) * 0.25f) +
        square(local::sinPhi(w) * 0.25f);
      return (sqrtf(1.f + alpha2 * tan2Theta) - 1.f) * 0.5f;
    }

    __both__ inline float G(Vec3f wo, Vec3f wi) const
    {
      return 1.f / (1.f + lambda(wo) + lambda(wi));
    }

    __both__ inline float F(float absHDotWi) const
    {
      return etaToR0(1.5f) +
             (1.f - etaToR0(1.5f)) * powf(fmaxf(0.f, 1.f - absHDotWi), 5.f);
    }

    __both__ inline Vec3f sample_h(Vec2f u) const
    {
      const float a2 = square(alpha);
      const float z = sqrtf(fmaxf(0.f, (1.f - powf(a2, 1.f - u.x)) / (1.f - a2)));
      const float r = sqrtf(fmaxf(0.f, 1.f - z * z));
      const float phi = 2.f * Pi * u.y;
      return Vec3f(r * cosf(phi), r * sinf(phi), z);
    }

    __both__ inline float PDF(Vec3f h) const
    {
      return D(h) * local::absCosTheta(h);
    }
  };

} // namespace pt


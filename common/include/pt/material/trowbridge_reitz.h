#pragma once

#include "../math/vec.h"

namespace pt {

  namespace local {

    __both__ inline float cosTheta(Vec3f w) { return w.z; }
    __both__ inline float cos2Theta(Vec3f w) { return square(w.z); }
    __both__ inline float absCosTheta(Vec3f w) { return fabsf(w.z); }

    __both__ inline float sin2Theta(Vec3f w)
    {
      return fmaxf(0.f, 1.f - cos2Theta(w));
    }

    __both__ inline float tan2Theta(Vec3f w)
    {
      const float c2 = cos2Theta(w);
      if (c2 <= 0.f) return INFINITY;
      return sin2Theta(w) / c2;
    }

    __both__ inline float cosPhi(Vec3f w)
    {
      const float s = sqrtf(sin2Theta(w));
      return s == 0.f ? 1.f : clamp(w.x / s, -1.f, 1.f);
    }

    __both__ inline float sinPhi(Vec3f w)
    {
      const float s = sqrtf(sin2Theta(w));
      return s == 0.f ? 0.f : clamp(w.y / s, -1.f, 1.f);
    }

    __both__ inline bool sameHemisphere(Vec3f a, Vec3f b)
    {
      return a.z * b.z > 0.f;
    }

    __both__ inline Vec3f reflect(Vec3f wo, Vec3f n)
    {
      return -wo + 2.f * dot(wo, n) * n;
    }

    __both__ inline bool refract(Vec3f wi,
                                                Vec3f n,
                                                float eta,
                                                float &etap,
                                                Vec3f &wt)
    {
      float cosThetaI = dot(n, wi);
      if (cosThetaI < 0.f) {
        eta = 1.f / eta;
        cosThetaI = -cosThetaI;
        n = -n;
      }

      const float sin2ThetaI = fmaxf(0.f, 1.f - square(cosThetaI));
      const float sin2ThetaT = sin2ThetaI / square(eta);
      if (sin2ThetaT >= 1.f) return false;

      const float cosThetaT = safeSqrt(1.f - sin2ThetaT);
      wt = -wi / eta + (cosThetaI / eta - cosThetaT) * n;
      etap = eta;
      return true;
    }

  } // namespace local

  struct TrowbridgeReitzDistribution {
    float alphaX;
    float alphaY;

    __both__ TrowbridgeReitzDistribution()
      : alphaX(1.f), alphaY(1.f) {}

    __both__ TrowbridgeReitzDistribution(float ax, float ay)
      : alphaX(ax), alphaY(ay) {}

    __both__ inline bool effectivelySmooth() const
    {
      return fmaxf(alphaX, alphaY) < 1e-3f;
    }

    __both__ inline float D(Vec3f wm) const
    {
      const float tan2Theta = local::tan2Theta(wm);
      if (isInf(tan2Theta)) return 0.f;

      const float cos4Theta = square(local::cos2Theta(wm));
      const float e = tan2Theta *
        (square(local::cosPhi(wm) / alphaX) +
         square(local::sinPhi(wm) / alphaY));

      return 1.f / (Pi * alphaX * alphaY * cos4Theta * square(1.f + e));
    }

    __both__ inline float lambda(Vec3f w) const
    {
      const float tan2Theta = local::tan2Theta(w);
      if (isInf(tan2Theta)) return 0.f;

      const float alpha2 =
        square(local::cosPhi(w) * alphaX) +
        square(local::sinPhi(w) * alphaY);

      return (sqrtf(1.f + alpha2 * tan2Theta) - 1.f) * 0.5f;
    }

    __both__ inline float G1(Vec3f w) const
    {
      return 1.f / (1.f + lambda(w));
    }

    __both__ inline float G(Vec3f wo, Vec3f wi) const
    {
      return 1.f / (1.f + lambda(wo) + lambda(wi));
    }

    __both__ inline float D(Vec3f w, Vec3f wm) const
    {
      return G1(w) / local::absCosTheta(w) * D(wm) * fabsf(dot(w, wm));
    }

    __both__ inline float PDF(Vec3f w, Vec3f wm) const
    {
      return D(w, wm);
    }

    __both__ inline Vec3f sample_wm(Vec3f w, Vec2f u) const
    {
      Vec3f wh = normalize(Vec3f(alphaX * w.x, alphaY * w.y, w.z));
      if (wh.z < 0.f) wh = -wh;

      const Vec3f t1 = (wh.z < 0.99999f)
        ? normalize(cross(Vec3f(0.f, 0.f, 1.f), wh))
        : Vec3f(1.f, 0.f, 0.f);
      const Vec3f t2 = cross(wh, t1);

      Vec2f p = sampleUniformDiskPolar(u);
      const float h = sqrtf(1.f - square(p.x));
      p.y = lerp(h, p.y, (1.f + wh.z) * 0.5f);

      const float pz = safeSqrt(1.f - square(length(p)));
      const Vec3f nh = p.x * t1 + p.y * t2 + pz * wh;
      return normalize(Vec3f(alphaX * nh.x,
                             alphaY * nh.y,
                             fmaxf(1e-6f, nh.z)));
    }
  };

} // namespace pt


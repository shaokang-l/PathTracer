#pragma once

#include "../math/vec.h"

namespace pt {

  __both__ inline float fresnelDielectric(float cosThetaI,
                                                         float eta)
  {
    cosThetaI = clamp(cosThetaI, -1.f, 1.f);

    if (cosThetaI < 0.f) {
      eta = 1.f / eta;
      cosThetaI = -cosThetaI;
    }

    const float sin2ThetaI = fmaxf(0.f, 1.f - square(cosThetaI));
    const float sin2ThetaT = sin2ThetaI / square(eta);
    if (sin2ThetaT >= 1.f) return 1.f;

    const float cosThetaT = safeSqrt(1.f - sin2ThetaT);
    const float rParallel =
      (eta * cosThetaI - cosThetaT) / (eta * cosThetaI + cosThetaT);
    const float rPerp =
      (cosThetaI - eta * cosThetaT) / (cosThetaI + eta * cosThetaT);

    return 0.5f * (square(rParallel) + square(rPerp));
  }

  __both__ inline float fresnelComplex(float cosThetaI,
                                                      float eta,
                                                      float k)
  {
    cosThetaI = clamp(cosThetaI, 0.f, 1.f);
    const float cos2ThetaI = square(cosThetaI);
    const float sin2ThetaI = 1.f - cos2ThetaI;
    const float eta2 = square(eta);
    const float k2 = square(k);

    const float t0 = eta2 - k2 - sin2ThetaI;
    const float a2PlusB2 = safeSqrt(square(t0) + 4.f * eta2 * k2);
    const float t1 = a2PlusB2 + cos2ThetaI;
    const float a = safeSqrt(0.5f * (a2PlusB2 + t0));
    const float t2 = 2.f * cosThetaI * a;
    const float rs = (t1 - t2) / (t1 + t2);

    const float t3 = cos2ThetaI * a2PlusB2 + square(sin2ThetaI);
    const float t4 = t2 * sin2ThetaI;
    const float rp = rs * (t3 - t4) / (t3 + t4);

    return 0.5f * (rp + rs);
  }

  __both__ inline Vec3f fresnelComplex(float cosThetaI,
                                                      Vec3f eta,
                                                      Vec3f k)
  {
    return Vec3f(fresnelComplex(cosThetaI, eta.x, k.x),
                 fresnelComplex(cosThetaI, eta.y, k.y),
                 fresnelComplex(cosThetaI, eta.z, k.z));
  }

  __both__ inline float etaToR0(float eta)
  {
    return square((eta - 1.f) / (eta + 1.f));
  }

  __both__ inline Vec3f fresnelSchlick(float cosTheta,
                                                      Vec3f f0)
  {
    const float w = powf(fmaxf(0.f, 1.f - cosTheta), 5.f);
    return f0 + (Vec3f(1.f) - f0) * w;
  }

} // namespace pt


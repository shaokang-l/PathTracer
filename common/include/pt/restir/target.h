#pragma once

#include "../math/vec.h"

namespace pt {

  struct DirectLightGeometry {
    Vec3f wi = Vec3f(0.f); // direction from shading point to light
    float distance = 0.f;
    float distanceSquared = 0.f;
    float NoI = 0.f; // cosine at the shading point
    float NoL = 0.f; // cosine at the light point
    float G = 0.f;   // area-measure geometry term: NoI * NoL / distance^2
  };

  __both__ inline float luminance(Vec3f c)
  {
    return 0.2126f * c.x + 0.7152f * c.y + 0.0722f * c.z;
  }

  __both__ inline float restirTargetFromRgb(Vec3f value)
  {
    return fmaxf(0.f, luminance(value));
  }

  __both__ inline DirectLightGeometry directLightGeometry(Vec3f shadingP,
                                                          Vec3f shadingN,
                                                          Vec3f lightP,
                                                          Vec3f lightN)
  {
    DirectLightGeometry result;
    const Vec3f toLight = lightP - shadingP;
    result.distanceSquared = dot(toLight, toLight);
    if (result.distanceSquared <= 1e-12f) return result;

    result.distance = sqrtf(result.distanceSquared);
    result.wi = toLight / result.distance;
    result.NoI = fmaxf(0.f, dot(shadingN, result.wi));
    result.NoL = fmaxf(0.f, dot(lightN, -result.wi));
    result.G = result.NoI * result.NoL / result.distanceSquared;
    return result;
  }

  __both__ inline float geometryTerm(Vec3f shadingP,
                                     Vec3f shadingN,
                                     Vec3f lightP,
                                     Vec3f lightN)
  {
    return directLightGeometry(shadingP, shadingN, lightP, lightN).G;
  }

  // Area-domain, unshadowed direct-light contribution: Le * f(wo, wi) * G.
  // `bsdfValue` must be the pure BSDF value f(wo, wi), without a cosine term;
  // `geometry` already includes the shading cosine NoI.
  __both__ inline Vec3f directLightingAreaIntegrand(Vec3f emittedRadiance,
                                                    Vec3f bsdfValue,
                                                    float geometry)
  {
    return emittedRadiance * bsdfValue * geometry;
  }

  __both__ inline Vec3f directLightingAreaIntegrand(Vec3f shadingP,
                                                    Vec3f shadingN,
                                                    Vec3f lightP,
                                                    Vec3f lightN,
                                                    Vec3f emittedRadiance,
                                                    Vec3f bsdfValue)
  {
    return directLightingAreaIntegrand(
      emittedRadiance,
      bsdfValue,
      geometryTerm(shadingP, shadingN, lightP, lightN));
  }

  // ReSTIR's scalar target is based on the unshadowed area-domain contribution.
  // Visibility and source-PDF division are applied by the estimator, not here.
  __both__ inline float directLightingTarget(Vec3f emittedRadiance,
                                             Vec3f bsdfValue,
                                             float geometry)
  {
    return restirTargetFromRgb(
      directLightingAreaIntegrand(emittedRadiance, bsdfValue, geometry));
  }

  __both__ inline float directLightingTarget(Vec3f shadingP,
                                             Vec3f shadingN,
                                             Vec3f lightP,
                                             Vec3f lightN,
                                             Vec3f emittedRadiance,
                                             Vec3f bsdfValue)
  {
    return restirTargetFromRgb(
      directLightingAreaIntegrand(shadingP,
                                  shadingN,
                                  lightP,
                                  lightN,
                                  emittedRadiance,
                                  bsdfValue));
  }

} // namespace pt


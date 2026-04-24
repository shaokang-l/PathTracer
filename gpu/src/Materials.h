// ======================================================================== //
// Materials.h - device-only BSDF sampling helpers.
//
// This is the file you'll grow most when porting your CPU BSDFs.
// Right now it supports three toy materials to prove the pipeline end-
// to-end; structure it however you like, but keep the key constraint:
// everything here must be callable from a closest-hit program, with no
// virtual dispatch.
//
// One good porting strategy:
//   1. Flatten your BSDF hierarchy into one tagged union (MaterialGPU),
//      add switch-dispatched sample/eval/pdf helpers.
//   2. Later, when it becomes unwieldy, split into one OWLGeomType per
//      material family (Lambert, Metal, Glass, Disney, ...) with its
//      own closest-hit program - like samples/cmdline/s05-rtow.
// ======================================================================== //

#pragma once

#include "deviceCode.h"
#include <owl/common/math/random.h>
#include <owl/common/math/vec.h>

using RNG = owl::common::LCG<4>;

namespace mypt {

  // ---------- helpers --------------------------------------------------

  __device__ inline vec3f sampleCosineHemisphere(RNG &rng, const vec3f &N)
  {
    const float r1 = rng();
    const float r2 = rng();
    const float phi = 2.f * float(M_PI) * r1;
    const float r = sqrtf(r2);
    const float x = r * cosf(phi);
    const float y = r * sinf(phi);
    const float z = sqrtf(fmaxf(0.f, 1.f - r2));

    vec3f T, B;
    if (fabsf(N.x) > fabsf(N.y))
      T = normalize(vec3f(-N.z, 0.f, N.x));
    else
      T = normalize(vec3f(0.f, N.z, -N.y));
    B = cross(N, T);
    return normalize(x * T + y * B + z * N);
  }

  __device__ inline vec3f reflect(const vec3f &d, const vec3f &n)
  {
    return d - 2.f * dot(d, n) * n;
  }

  __device__ inline bool refract(const vec3f &d, const vec3f &n,
                                 float eta, vec3f &out)
  {
    const float cosI = -dot(d, n);
    const float k = 1.f - eta * eta * (1.f - cosI * cosI);
    if (k < 0.f) return false;
    out = normalize(eta * d + (eta * cosI - sqrtf(k)) * n);
    return true;
  }

  __device__ inline float fresnelSchlick(float cosTheta, float ior)
  {
    float r0 = (1.f - ior) / (1.f + ior);
    r0 = r0 * r0;
    return r0 + (1.f - r0) * powf(fmaxf(0.f, 1.f - cosTheta), 5.f);
  }

  // ---------- sampleBSDF: one-sample MIS-free interface ----------------
  //
  // Returns throughput weight (f * cosTheta / pdf).
  // 'scatteredDir' is written unless the ray is absorbed (returns false).
  //
  struct BSDFSample {
    vec3f throughput;   // f * cos / pdf
    vec3f direction;    // sampled world-space direction
    bool  isSpecular;
  };

  __device__ inline bool sampleBSDF(const MaterialGPU &mat,
                                    const vec3f       &wo,   // view dir, pointing AWAY from surface
                                    const vec3f       &N,    // shading normal
                                    RNG               &rng,
                                    BSDFSample        &out)
  {
    switch (mat.kind) {

    case MATERIAL_LAMBERTIAN: {
      out.direction   = sampleCosineHemisphere(rng, N);
      out.throughput  = mat.albedo;   // for cosine-weighted sampling, f*cos/pdf = albedo
      out.isSpecular  = false;
      return true;
    }

    case MATERIAL_MIRROR: {
      out.direction   = reflect(-wo, N);
      out.throughput  = mat.albedo;
      out.isSpecular  = true;
      return dot(out.direction, N) > 0.f;
    }

    case MATERIAL_DIELECTRIC: {
      const bool entering = dot(wo, N) > 0.f;
      const vec3f n = entering ? N : -N;
      const float eta = entering ? (1.f / mat.ior) : mat.ior;
      const float cosI = fabsf(dot(wo, N));
      const float Fr = fresnelSchlick(cosI, mat.ior);
      vec3f refracted;
      if (rng() < Fr || !refract(-wo, n, eta, refracted)) {
        out.direction = reflect(-wo, n);
      } else {
        out.direction = refracted;
      }
      out.throughput  = vec3f(1.f);
      out.isSpecular  = true;
      return true;
    }

    case MATERIAL_EMISSIVE:
    default:
      return false;
    }
  }

} // namespace mypt

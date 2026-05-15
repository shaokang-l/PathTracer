// ======================================================================== //
// visibility.h - device-side shadow visibility helpers.
// ======================================================================== //

#pragma once

#include "device/prd.h"
#include "pod/rayTypes.h"

#include <optix_device.h>
#include <owl/owl_device.h>

using namespace owl;
using namespace mypt;

__device__ inline vec3f traceVisibility(OptixTraversableHandle world,
                                        const vec3f &p,
                                        const vec3f &target)
{
  const vec3f toTarget = target - p;
  const float dist2    = dot(toTarget, toTarget);
  if (dist2 <= 1e-7f) return vec3f(0.f);

  const float dist = sqrtf(dist2);
  const vec3f dir  = toTarget * (1.f / dist);

  ShadowPRD prd;
  prd.transmittance = vec3f(1.f);

  ShadowRay ray(p, dir, 1e-3f, dist - 1e-3f);
  owl::traceRay(world,
                ray,
                prd,
                OPTIX_RAY_FLAG_DISABLE_ANYHIT
              | OPTIX_RAY_FLAG_TERMINATE_ON_FIRST_HIT);

  return prd.transmittance;
}

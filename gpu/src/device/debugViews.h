// ======================================================================== //
// debugViews.h - device-side debug output helpers.
// ======================================================================== //

#pragma once

#include "device/prd.h"
#include "device/visibility.h"
#include "pod/launchParams.h"
#include "pod/light.h"
#include "pod/material.h"
#include "pt/render/debug_view_kind.h"
#include "ptInterop.h"

using namespace owl;
using namespace mypt;

__device__ inline vec3f hashColor(int id)
{
  unsigned int x = unsigned(id + 1) * 747796405u + 2891336453u;
  x = ((x >> ((x >> 28u) + 4u)) ^ x) * 277803737u;
  x = (x >> 22u) ^ x;
  const float r = float((x >> 0u) & 255u) / 255.f;
  const float g = float((x >> 8u) & 255u) / 255.f;
  const float b = float((x >> 16u) & 255u) / 255.f;
  return vec3f(r, g, b);
}

__device__ inline vec3f materialDebugAlbedo(const MaterialGPU &material)
{
  if (material.kind == MATERIAL_EMISSIVE) return material.emission;
  if (material.kind == MATERIAL_DISNEY_PRINCIPLED) return material.baseColor;
  return material.albedo;
}

__device__ inline bool isRestirDebugView(pt::DebugViewKind debugView)
{
  return debugView == pt::DebugViewKind::ReservoirWeight ||
         debugView == pt::DebugViewKind::ReservoirM ||
         debugView == pt::DebugViewKind::ReservoirTarget ||
         debugView == pt::DebugViewKind::RestirLightId;
}

__device__ inline float compressDebugScalar(float v)
{
  return v > 0.f ? v / (1.f + v) : 0.f;
}

__device__ inline vec3f shadeRestirDebugView(const LaunchParams &params,
                                             int pxIdx,
                                             pt::DebugViewKind debugView)
{
  if (!params.restirReservoirs) return vec3f(0.f);

  const pt::RestirReservoir reservoir = params.restirReservoirs[pxIdx];
  if (reservoir.M == 0 || reservoir.W <= 0.f) return vec3f(0.f);

  // Stage A note: beauty mode writes this buffer during normal ReSTIR direct
  // lighting, while ReSTIR debug launches generate a no-reuse reservoir before
  // calling this helper so headless --debug-view runs are self-contained.
  if (debugView == pt::DebugViewKind::ReservoirWeight) {
    const float v = compressDebugScalar(reservoir.W);
    return vec3f(v);
  }

  if (debugView == pt::DebugViewKind::ReservoirM) {
    const float v = fminf(float(reservoir.M) / 16.f, 1.f);
    return vec3f(v);
  }

  if (debugView == pt::DebugViewKind::ReservoirTarget) {
    const float v = compressDebugScalar(reservoir.y.target);
    return vec3f(v);
  }

  if (debugView == pt::DebugViewKind::RestirLightId) {
    return reservoir.y.lightId >= 0 ? hashColor(reservoir.y.lightId) : vec3f(0.f);
  }

  return vec3f(0.f);
}

__device__ inline vec3f shadeDebugView(const LaunchParams &params,
                                       const PRD &prd,
                                       int pxIdx)
{
  const pt::DebugViewKind debugView = toDebugViewKind(params.debugView);
  if (isRestirDebugView(debugView)) {
    return shadeRestirDebugView(params, pxIdx, debugView);
  }

  if (!prd.didHit) return prd.emission;

  if (debugView == pt::DebugViewKind::Normal) {
    return 0.5f * (prd.N + vec3f(1.f));
  }

  const MaterialGPU &material = params.materials[prd.materialId];
  if (debugView == pt::DebugViewKind::Albedo) {
    return materialDebugAlbedo(material);
  }

  if (debugView == pt::DebugViewKind::MaterialId) {
    return hashColor(prd.materialId);
  }

  if (debugView == pt::DebugViewKind::Visibility ||
      debugView == pt::DebugViewKind::LightId) {
    LightSample lightSample;
    const int lightID = params.lightCount > 0 ? 0 : -1;
    if (sampleLight(params.lights,
                    params.lightCount,
                    0.f,
                    vec2f(0.5f, 0.5f),
                    lightSample)) {
      const vec3f V = traceVisibility(params.world, prd.hitP, lightSample.p);
      const float visible = fmaxf(V.x, fmaxf(V.y, V.z));
      if (debugView == pt::DebugViewKind::LightId) {
        return visible > 0.f ? hashColor(lightID) : vec3f(0.f);
      }
      return vec3f(visible);
    }
    return vec3f(0.f);
  }

  return vec3f(0.f);
}

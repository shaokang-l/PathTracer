// ======================================================================== //
// deviceCode.cu - the actual OptiX programs (raygen, closest-hit, miss).
//
// This is the ONLY file compiled to PTX and embedded into the binary.
// Keep it as small as possible; put reusable device code in headers
// that this .cu includes.
// ======================================================================== //

#include <optix_device.h>

#include "bsdf.h"
#include "bxdfFlags.h"
#include "geometryData.h"
#include "launchParams.h"
#include "ptInterop.h"
#include "rayTypes.h"
#include "pt/restir/reservoir.h"
#include "pt/restir/target.h"

using namespace owl;
using namespace mypt;

// OWL provides a clean way to publish launch-params to the device:
// declare a __constant__ of your type called 'optixLaunchParams'.
// The 'extern "C"' matches what the OWL samples use so the symbol
// name is stable across device translation units.
extern "C" __constant__ LaunchParams optixLaunchParams;

// ------------------------------------------------------------------
// Per-ray data (PRD) - lives on the device-side "stack" during a
// single traceRay call. We only carry what the closest-hit program
// needs to write back.
// ------------------------------------------------------------------
// PRD lives in local memory and is read/written by CH/miss/raygen on every
// trace. Keep it as small as possible: store a materialId index into the
// global material buffer rather than a copy of the whole MaterialGPU,
// which roughly halves PRD size and the corresponding L1 traffic.
struct PRD {
  vec3f hitP;
  vec3f N;
  int   materialId;
  bool  didHit;
  bool  isEmissive;
  vec3f emission;
};

// ------------------------------------------------------------------
// Shadow PRD
// Carries a vec3f visibility value, which is 0 if the light is not visible, and 1 if it is.
// ------------------------------------------------------------------
struct ShadowPRD {
  vec3f transmittance;
};

enum DebugView : int {
  DEBUG_VIEW_BEAUTY = 0,
  DEBUG_VIEW_NORMAL = 1,
  DEBUG_VIEW_ALBEDO = 2,
  DEBUG_VIEW_VISIBILITY = 3,
  DEBUG_VIEW_MATERIAL_ID = 4,
  DEBUG_VIEW_LIGHT_ID = 5,
};

// ---------------
// ---------------------------------------------------
// Closest-hit: fill the PRD. All shading / next-event logic lives in
// the raygen loop.
// ------------------------------------------------------------------
OPTIX_CLOSEST_HIT_PROGRAM(TriangleMesh)()
{
  PRD &prd = owl::getPRD<PRD>();
  const TriangleMeshSBT &self = owl::getProgramData<TriangleMeshSBT>();
  const MaterialGPU &material = optixLaunchParams.materials[self.materialId];

  const int   primID = optixGetPrimitiveIndex();
  const vec3i idx    = self.index[primID];
  const vec3f A      = self.vertex[idx.x];
  const vec3f B      = self.vertex[idx.y];
  const vec3f C      = self.vertex[idx.z];

  vec3f N = normalize(cross(B - A, C - A));

  const vec3f rayDir = (vec3f)optixGetWorldRayDirection();
  if (dot(N, rayDir) > 0.f) N = -N;

  const float tHit = optixGetRayTmax();
  prd.hitP       = (vec3f)optixGetWorldRayOrigin() + tHit * rayDir;
  prd.N          = N;
  prd.materialId = self.materialId;
  prd.didHit     = true;
  prd.isEmissive = (material.kind == MATERIAL_EMISSIVE);
  prd.emission   = material.emission;
}

// initial version, binary shadow ray visibility check.
OPTIX_CLOSEST_HIT_PROGRAM(TriangleMeshShadow)()
{
  ShadowPRD &prd = owl::getPRD<ShadowPRD>();
  prd.transmittance = vec3f(0.f);
}

// ------------------------------------------------------------------
// Miss: environment light. Simple vertical gradient for now.
// ------------------------------------------------------------------
OPTIX_MISS_PROGRAM(miss)()
{
  PRD &prd = owl::getPRD<PRD>();
  const MissProgData &self = owl::getProgramData<MissProgData>();

  const vec3f rayDir = normalize((vec3f)optixGetWorldRayDirection());
  const float t = 0.5f * (rayDir.y + 1.f);
  const vec3f sky = (1.f - t) * self.skyColorBottom + t * self.skyColorTop;

  prd.didHit     = false;
  prd.isEmissive = true;
  prd.emission   = sky;
}

OPTIX_MISS_PROGRAM(missShadow)()
{
  ShadowPRD &prd = owl::getPRD<ShadowPRD>();
  prd.transmittance = vec3f(1.f);
}

// ------------------------------------------------------------------
// traceVisibility: shoot a ShadowRay from `p` toward `target` and
// return the accumulated transmittance written by the shadow CH/miss
// programs. Returning vec3f (instead of a bool) leaves room for
// colored / partial transmission later.
// ------------------------------------------------------------------
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

__device__ inline vec3f shadeDebugView(const LaunchParams &params,
                                       const PRD &prd)
{
  if (!prd.didHit) return prd.emission;

  if (params.debugView == DEBUG_VIEW_NORMAL) {
    return 0.5f * (prd.N + vec3f(1.f));
  }

  const MaterialGPU &material = params.materials[prd.materialId];
  if (params.debugView == DEBUG_VIEW_ALBEDO) {
    return materialDebugAlbedo(material);
  }

  if (params.debugView == DEBUG_VIEW_MATERIAL_ID) {
    return hashColor(prd.materialId);
  }

  if (params.debugView == DEBUG_VIEW_VISIBILITY ||
      params.debugView == DEBUG_VIEW_LIGHT_ID) {
    LightSample lightSample;
    const int lightID = params.lightCount > 0 ? 0 : -1;
    if (sampleLight(params.lights,
                    params.lightCount,
                    0.f,
                    vec2f(0.5f, 0.5f),
                    lightSample)) {
      const vec3f V = traceVisibility(params.world, prd.hitP, lightSample.p);
      const float visible = fmaxf(V.x, fmaxf(V.y, V.z));
      if (params.debugView == DEBUG_VIEW_LIGHT_ID) {
        return visible > 0.f ? hashColor(lightID) : vec3f(0.f);
      }
      return vec3f(visible);
    }
    return vec3f(0.f);
  }

  return vec3f(0.f);
}

// ------------------------------------------------------------------
// RayGen: path-tracing loop. Iterative (never recursive).
// ------------------------------------------------------------------
OPTIX_RAYGEN_PROGRAM(rayGen)()
{
  const LaunchParams &params = optixLaunchParams;
  const vec2i pixelID = owl::getLaunchIndex();
  const int   pxIdx   = pixelID.x + params.fbSize.x * pixelID.y;

  RNG rng(pxIdx, params.accumID + 1);

  vec3f L = vec3f(0.f);
  const bool debugMode = params.debugView != DEBUG_VIEW_BEAUTY;
  const int spp = debugMode ? 1 : params.samplesPerPixel;

  for (int s = 0; s < spp; ++s) {
    const vec2f jitter = debugMode ? vec2f(0.5f) : vec2f(rng(), rng());
    const vec2f screen = (vec2f(pixelID) + jitter) / vec2f(params.fbSize);

    vec3f rayOrigin = params.camera.pos;
    vec3f rayDir    = normalize(params.camera.dir_00
                                + screen.x * params.camera.dir_du
                                + screen.y * params.camera.dir_dv);

    if (debugMode) {
      PRD prd;
      prd.didHit = false;
      RadianceRay ray(rayOrigin, rayDir, 1e-3f, 1e20f);
      owl::traceRay(params.world, ray, prd);
      L = L + shadeDebugView(params, prd);
      continue;
    }

    vec3f throughput = vec3f(1.f);
    vec3f radiance   = vec3f(0.f);
    bool addEmission = true; // avoid double counting

    for (int depth = 0; depth < params.maxBounces; ++depth) {
      PRD prd;
      prd.didHit = false;

      RadianceRay ray(rayOrigin, rayDir, 1e-3f, 1e20f);
      owl::traceRay(params.world, ray, prd);

      if (prd.isEmissive&&(addEmission || !prd.didHit)) {
        radiance = radiance + throughput * prd.emission;
      }
      if (!prd.didHit) break;

      // 01. BSDF Wrapper and ONB
      const OrthoBasis basis = makeOrthoBasis(prd.N);
      const MaterialGPU &material = params.materials[prd.materialId];
      const BSDF bsdf(basis, &material);
      const vec3f wo = -rayDir;

      if (bsdf.hasNonDelta())
      {
        LightSample lightSample;
        if (sampleLight(params.lights, params.lightCount, rng(), vec2f(rng(), rng()), lightSample))
        {
          vec3f toLight = lightSample.p - prd.hitP;
          float dist2 = dot(toLight, toLight);

          if (dist2 > 1e-7f && lightSample.pdfA > 0.f) {
            float dist = sqrtf(dist2);
            vec3f wi = toLight * (1.f / dist);

            float NoI = fmaxf(dot(prd.N, wi), 0.f);
            float NoL = fmaxf(dot(lightSample.n, -wi), 0.f);

            if (NoI > 0.f && NoL > 0.f) {
              const vec3f V = traceVisibility(params.world, prd.hitP, lightSample.p);
              if (V.x > 0.f || V.y > 0.f || V.z > 0.f) {
                const vec3f f = bsdf.f(wo, wi);
                const float G = NoI * NoL / dist2;
                radiance += throughput * lightSample.Le * f * V * G / lightSample.pdfA;
              }
            }
          }
        }
      }

      // 02. generate uc and u from RNG
      const float uc = rng();
      const vec2f u(rng(), rng());

      // 03. sample BSDF
      BSDFSample sample;
      if (!bsdf.sample_f(wo, uc, u, sample)) break;
      if(sample.pdf <= 0.f) break;

      addEmission = mypt::isSpecular(sample.flag);

      // 04. update throughput, lo = f*cos*Li/pdf
      const float cosTheta = fabsf(dot(sample.wi, prd.N));
      throughput *= sample.f*cosTheta/sample.pdf;

      const float pRR = fminf(0.95f, fmaxf(throughput.x,
                              fmaxf(throughput.y, throughput.z)));
      if (depth >= 3) {
        if (rng() >= pRR) break;
        throughput = throughput * (1.f / pRR);
      }

      rayOrigin = prd.hitP;
      rayDir    = sample.wi;
    }

    L = L + radiance;
  }

  L = L * (1.f / float(spp));

  // Raygen now writes the linear-HDR accumulator only; tone-mapping and
  // fbPtr packing live in postprocess.cu so a denoiser (or any other
  // post-process) can slot in between accumulate and display.
  float4 prev = (params.accumID > 0)
    ? params.accumBuffer[pxIdx]
    : make_float4(0.f, 0.f, 0.f, 0.f);
  const float n = float(params.accumID + 1);
  float4 accum;
  accum.x = (prev.x * float(params.accumID) + L.x) / n;
  accum.y = (prev.y * float(params.accumID) + L.y) / n;
  accum.z = (prev.z * float(params.accumID) + L.z) / n;
  accum.w = 1.f;
  params.accumBuffer[pxIdx] = accum;
}

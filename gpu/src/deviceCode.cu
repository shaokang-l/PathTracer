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
struct PRD {
  vec3f hitP;
  vec3f N;
  MaterialGPU material;
  bool  didHit;
  bool  isEmissive;
  vec3f emission;
};

// ------------------------------------------------------------------
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
  prd.material   = material;
  prd.didHit     = true;
  prd.isEmissive = (material.kind == MATERIAL_EMISSIVE);
  prd.emission   = material.emission;
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
  const int spp = params.samplesPerPixel;

  for (int s = 0; s < spp; ++s) {
    const vec2f jitter(rng(), rng());
    const vec2f screen = (vec2f(pixelID) + jitter) / vec2f(params.fbSize);

    vec3f rayOrigin = params.camera.pos;
    vec3f rayDir    = normalize(params.camera.dir_00
                                + screen.x * params.camera.dir_du
                                + screen.y * params.camera.dir_dv);

    vec3f throughput = vec3f(1.f);
    vec3f radiance   = vec3f(0.f);

    for (int depth = 0; depth < params.maxBounces; ++depth) {
      PRD prd;
      prd.didHit = false;

      owl::Ray ray(rayOrigin, rayDir, 1e-3f, 1e20f);
      owl::traceRay(params.world, ray, prd);

      if (prd.isEmissive) {
        radiance = radiance + throughput * prd.emission;
      }
      if (!prd.didHit) break;

      // 01. BSDF Wrapper and ONB
      const OrthoBasis basis = makeOrthoBasis(prd.N);
      const BSDF bsdf(basis, &prd.material);


      // Light sampling
      if(isSpecular(prd.material.kind))
      {
        
      }

      // 02. generate uc and u from RNG
      const float uc = rng();
      const vec2f u(rng(), rng());

      // 03. sample BSDF
      BSDFSample sample;
      const vec3f wo = -rayDir;
      if (!bsdf.sample_f(wo, uc, u, sample)) break;
      if(sample.pdf <= 0.f) break;

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

  vec3f display;
  display.x = accum.x / (1.f + accum.x);
  display.y = accum.y / (1.f + accum.y);
  display.z = accum.z / (1.f + accum.z);
  display.x = powf(fmaxf(0.f, display.x), 1.f / 2.2f);
  display.y = powf(fmaxf(0.f, display.y), 1.f / 2.2f);
  display.z = powf(fmaxf(0.f, display.z), 1.f / 2.2f);

  params.fbPtr[pxIdx] = owl::make_rgba(display);
}

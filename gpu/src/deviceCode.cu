// ======================================================================== //
// deviceCode.cu - the actual OptiX programs (raygen, closest-hit, miss).
//
// This is the ONLY file compiled to PTX and embedded into the binary.
// Keep it as small as possible; put reusable device code in headers
// that this .cu includes.
// ======================================================================== //

#include <optix_device.h>

#include "shading/bsdf.h"
#include "shading/bxdfFlags.h"
#include "device/debugViews.h"
#include "device/directLight.h"
#include "device/prd.h"
#include "pod/launchParams.h"
#include "pod/geometryData.h"
#include "ptInterop.h"
#include "pod/rayTypes.h"
#include "pt/render/debug_view_kind.h"
#include "pt/render/direct_light_mode.h"

using namespace owl;
using namespace pt;

// OWL provides a clean way to publish launch-params to the device:
// declare a __constant__ of your type called 'optixLaunchParams'.
// The 'extern "C"' matches what the OWL samples use so the symbol
// name is stable across device translation units.
extern "C" __constant__ LaunchParams optixLaunchParams;

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
// RayGen: path-tracing loop. Iterative (never recursive).
// ------------------------------------------------------------------
OPTIX_RAYGEN_PROGRAM(rayGen)()
{
  const LaunchParams &params = optixLaunchParams;
  const vec2i pixelID = owl::getLaunchIndex();
  const int   pxIdx   = pixelID.x + params.fbSize.x * pixelID.y;

  RNG rng(pxIdx, params.accumID + 1);

  vec3f L = vec3f(0.f);
  const pt::DebugViewKind debugView = toDebugViewKind(params.debugView);
  const bool debugMode = debugView != pt::DebugViewKind::Beauty;
  const int spp = debugMode ? 1 : params.samplesPerPixel;

  // initialize the reservoir and surface data
  if (params.restirReservoirs) {
    pt::clearReservoir(params.restirReservoirs[pxIdx]);
  }
  if (params.restirSurfaceData) {
    params.restirSurfaceData[pxIdx] = RestirSurfaceData();
  }
  if (params.restirSelectionSources) {
    params.restirSelectionSources[pxIdx] = 0;
  }

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

      if (isRestirDebugView(debugView) && prd.didHit) {
        storeRestirSurfaceData(params, pxIdx, prd);

        const MaterialGPU &material = params.materials[prd.materialId];
        const OrthoBasis basis = makeOrthoBasis(prd.N);
        const BSDF bsdf(basis, &material);
        const vec3f wo = -rayDir;

        // Stage A debug path:
        // Generate the same no-reuse reservoir that beauty mode would write,
        // so headless --debug-view reservoir-* runs do not depend on a prior
        // beauty launch having populated the reservoir buffer.
        if (bsdf.hasNonDelta()) {
          estimateDirectLightReservoir(params, pxIdx, prd, bsdf, wo, rng);

          if ((debugView == pt::DebugViewKind::TemporalCandidateTarget ||
               debugView == pt::DebugViewKind::TemporalTargetRatio) &&
              params.prevRestirReservoirs) {
            const pt::RestirReservoir prevReservoir = params.prevRestirReservoirs[pxIdx];
            pt::RestirDirectLightCandidate temporalCandidate;
            if (prevReservoir.M > 0 &&
                prevReservoir.W > 0.f &&
                prevReservoir.y.target > 0.f &&
                evaluateReservoirSampleAtCurrentHit(prd,
                                                    bsdf,
                                                    wo,
                                                    prevReservoir.y,
                                                    temporalCandidate)) {
              float v = compressDebugScalar(temporalCandidate.sample.target);
              if (debugView == pt::DebugViewKind::TemporalTargetRatio) {
                const float ratio = temporalCandidate.sample.target / prevReservoir.y.target;
                v = compressDebugScalar(ratio);
              }
              L = L + vec3f(v);
              continue;
            }
          }

          if (debugView == pt::DebugViewKind::TemporalAccepted) {
            pt::RestirDirectLightCandidate temporalCandidate;
            if (acceptTemporalReservoirCandidate(params,
                                                 pxIdx,
                                                 prd,
                                                 bsdf,
                                                 wo,
                                                 temporalCandidate)) {
              L = L + vec3f(1.f);
              continue;
            }
          }
        }
      }

      L = L + shadeDebugView(params, prd, pxIdx);
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

      if (depth == 0) {
        storeRestirSurfaceData(params, pxIdx, prd);
      }

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
        const pt::DirectLightMode directLightMode =
          toDirectLightMode(params.directLightMode);
        const vec3f direct_term =
          directLightMode == pt::DirectLightMode::Restir
            ? estimateDirectLightReservoir(params, pxIdx, prd, bsdf, wo, rng)
            : estimateDirectLightNee(params,
                                     prd,
                                     bsdf,
                                     wo,
                                     rng(),
                                     vec2f(rng(), rng()));
        radiance += throughput * direct_term;
      }

      // 02. generate uc and u from RNG
      const float uc = rng();
      const vec2f u(rng(), rng());

      // 03. sample BSDF
      BSDFSample sample;
      if (!bsdf.sample_f(wo, uc, u, sample)) break;
      if(sample.pdf <= 0.f) break;

      addEmission = pt::isSpecular(sample.flag);

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
  if (debugMode) {
    params.accumBuffer[pxIdx] = make_float4(L.x, L.y, L.z, 1.f);
    return;
  }

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

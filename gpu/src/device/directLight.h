// ======================================================================== //
// directLight.h - device-side NEE and no-reuse ReSTIR DI helpers.
// ======================================================================== //

#pragma once

#include "pt/math/vec.h"
#include "shading/bsdf.h"
#include "device/prd.h"
#include "device/visibility.h"
#include "pod/launchParams.h"
#include "pod/light.h"
#include "ptInterop.h"
#include "pt/restir/reservoir.h"
#include "pt/restir/target.h"
#include "pod/restirState.h"
#include "shading/utils.h"

using namespace owl;
using namespace pt;

__device__ inline bool generateDirectLightCandidate(
  const LaunchParams &params,
  const PRD &prd,
  const BSDF &bsdf,
  const vec3f &wo,
  float uLight,
  vec2f uSurface,
  pt::RestirDirectLightCandidate &out);

__device__ inline bool evaluateReservoirSampleAtCurrentHit(
  const PRD &prd,
  const BSDF &bsdf,
  const vec3f &wo,
  const pt::RestirLightSample &sample,
  pt::RestirDirectLightCandidate &out);

__device__ inline bool acceptTemporalReservoirCandidate(
  const LaunchParams &params,
  int pxIdx,
  const PRD &prd,
  const BSDF &bsdf,
  const vec3f &wo,
  pt::RestirDirectLightCandidate &outTemporalCandidate);

// Re-evaluate a previous-frame reservoir sample at the current hit point.
__device__ inline bool evaluateReservoirSampleAtCurrentHit(
  const PRD &prd,
  const BSDF &bsdf,
  const vec3f &wo,
  const pt::RestirLightSample &sample,
  pt::RestirDirectLightCandidate &out)
{
  out = pt::RestirDirectLightCandidate();

  const vec3f lightP = fromPtVec(sample.position);
  const vec3f lightN = fromPtVec(sample.normal);
  const vec3f lightLe = fromPtVec(sample.emission);

  const vec3f toLight = lightP - prd.hitP;
  const float dist2 = dot(toLight, toLight);
  if (dist2 <= 1e-7f || sample.sourcePdf <= 0.f)
    return false;

  const vec3f wi = normalize(toLight);

  const float NoI = fmaxf(dot(prd.N, wi), 0.f);
  const float NoL = fmaxf(dot(lightN, -wi), 0.f);
  if (NoI <= 0.f || NoL <= 0.f)
    return false;

  const vec3f f = bsdf.f(wo, wi);
  const float G = NoI * NoL / dist2;
  const vec3f unshadowedContribution = lightLe * f * G;
  const float target = pt::restirTargetFromRgb(toPtVec(unshadowedContribution));
  if (target <= 0.f)
    return false;

  out.sample = sample;
  out.sample.target = target;
  out.wi = toPtVec(wi);
  out.unshadowedContribution = toPtVec(unshadowedContribution);
  return true;
}

// Fetch the temporal candidate from the previous frame and check if it is valid.
__device__ inline bool acceptTemporalReservoirCandidate(
  const LaunchParams &params,
  int pxIdx,
  const PRD &prd,
  const BSDF &bsdf,
  const vec3f &wo,
  pt::RestirDirectLightCandidate &outTemporalCandidate)
{
  outTemporalCandidate = pt::RestirDirectLightCandidate();

  // This is same-pixel history validation only. It is useful for static-camera
  // plumbing and debug views, but camera motion still needs real reprojection.
  if (!params.restirTemporal ||
      !params.prevRestirReservoirs ||
      !params.prevRestirSurfaceData) {
    return false;
  }

  const pt::RestirReservoir prevReservoir = params.prevRestirReservoirs[pxIdx];
  if (prevReservoir.M == 0 ||
      prevReservoir.W <= 0.f ||
      prevReservoir.y.target <= 0.f) {
    return false;
  }

  const pt::RestirSurfaceData prevSurface = params.prevRestirSurfaceData[pxIdx];
  if (!prevSurface.valid || !prd.didHit) {
    return false;
  }

  if (prevSurface.materialId != prd.materialId) {
    return false;
  }

  // Same-pixel history should still describe the same local surface.
  const float normalAgreement = dot(prevSurface.normal, prd.N);
  if (normalAgreement < 0.9f) {
    return false;
  }

  const vec3f surfaceDelta = prevSurface.hitP - prd.hitP;
  const float surfaceDist2 = dot(surfaceDelta, surfaceDelta);
  if (surfaceDist2 > 0.01f) {
    return false;
  }

  if (!evaluateReservoirSampleAtCurrentHit(prd,
                                           bsdf,
                                           wo,
                                           prevReservoir.y,
                                           outTemporalCandidate)) {
    return false;
  }

  // Reject samples whose importance changes too drastically at the current hit.
  const float targetRatio =
    outTemporalCandidate.sample.target / prevReservoir.y.target;
  if (targetRatio < 0.1f || targetRatio > 10.f) {
    return false;
  }

  return true;
}

__device__ inline vec3f estimateDirectLightNee(
  const LaunchParams &params,
  const PRD &prd,
  const BSDF &bsdf,
  const vec3f &wo,
  float uLight,
  vec2f uSurface)
{
  LightSample lightSample;
  if (sampleLight(params.lights, params.lightCount, uLight, uSurface, lightSample)) {
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
          return lightSample.Le * f * V * G / lightSample.pdfA;
        }
      }
    }
  }
  return vec3f(0.f);
}

__device__ inline vec3f estimateDirectLightReservoir(
  const LaunchParams &params,
  int pxIdx,
  const PRD &prd,
  const BSDF &bsdf,
  const vec3f &wo,
  RNG &rng)
{
  pt::RestirReservoir reservoir;
  pt::RestirDirectLightCandidate selectedCandidate;
  int selectedSource = 0; // 0: none, 1: current, 2: temporal

  // No-reuse ReSTIR DI, local reservoir only:
  // 1. Generate N initial candidates from the current light sampler.
  // 2. Feed each valid candidate into reservoir update.
  // 3. Keep a copy of the candidate when updateReservoir() replaces y.
  //
  // This is where --restir-initial-candidates is consumed on the device.
  // For each frame, generate N initial candidates and update the local reservoir.
  for (int i = 0; i < params.restirInitialCandidates; ++i) {
    pt::RestirDirectLightCandidate candidate;
    const bool validCandidate =
      generateDirectLightCandidate(params,
                                   prd,
                                   bsdf,
                                   wo,
                                   rng(),
                                   vec2f(rng(), rng()),
                                   candidate);
    if (!validCandidate)
      continue;

    // TODO(ReSTIR): update the local reservoir with this candidate.
    const bool replaced =
      pt::updateReservoir(reservoir, candidate.sample, rng());
    if (replaced) {
      selectedCandidate = candidate;
      selectedSource = 1;
    }
  }

  // temporal merge reservoir
  pt::RestirDirectLightCandidate temporalCandidate;
  if (acceptTemporalReservoirCandidate(params, pxIdx, prd, bsdf, wo, temporalCandidate))
  {
    const pt::RestirReservoir prevReservoir = params.prevRestirReservoirs[pxIdx];
    const uint32_t representedM =
      min(prevReservoir.M, uint32_t(params.restirMaxHistory));
    
      const bool replaced = pt::updateReservoirWithRepresentedCandidate(
        reservoir,
        temporalCandidate.sample,
        temporalCandidate.sample.target,
        prevReservoir.W,
        representedM,
        rng());
      if (replaced) {
        selectedCandidate = temporalCandidate;
        selectedSource = 2;
      }
  }

  pt::finalizeReservoir(reservoir);
  if (params.restirSelectionSources) {
    params.restirSelectionSources[pxIdx] =
      (reservoir.W > 0.f && reservoir.y.target > 0.f) ? selectedSource : 0;
  }

  // Stage A persistent state:
  // Store the no-reuse reservoir so debug views and later temporal reuse can
  // inspect the exact sample selected for this pixel. This is current-frame
  // only; Stage B should replace this with current/previous ping-pong buffers.
  if (params.restirReservoirs) {
    params.restirReservoirs[pxIdx] = reservoir;
  }

  // trace visibility only for the selected reservoir sample.
  if (reservoir.W > 0.f && reservoir.y.target > 0.f) {
    const vec3f lightP = fromPtVec(selectedCandidate.sample.position);
    const vec3f V = traceVisibility(params.world, prd.hitP, lightP);
    if (V.x > 0.f || V.y > 0.f || V.z > 0.f) {
      return fromPtVec(selectedCandidate.unshadowedContribution) * V * reservoir.W;
    }
  }

  return vec3f(0.f);
};

__device__ inline void storeRestirSurfaceData(const LaunchParams &params,
                                             int pxIdx,
                                             const PRD &prd)
{
  if (!params.restirSurfaceData) return;

  RestirSurfaceData surface;
  if (prd.didHit) {
    surface.hitP = prd.hitP;
    surface.normal = prd.N;
    surface.materialId = prd.materialId;
    surface.valid = 1;
  }

  // TODO(ReSTIR temporal): store depth or previous-frame projection data if
  // world-space hitP rejection is not stable enough for camera motion.
  params.restirSurfaceData[pxIdx] = surface;
}

__device__ inline bool generateDirectLightCandidate(const LaunchParams &params,
                                                    const PRD &prd,
                                                    const BSDF &bsdf,
                                                    const vec3f &wo,
                                                    float uLight,
                                                    vec2f uSurface,
                                                    pt::RestirDirectLightCandidate &out)
{
  out = pt::RestirDirectLightCandidate();

  LightSample lightSample;
  if (!sampleLight(params.lights, params.lightCount, uLight, uSurface, lightSample))
    return false;

  const vec3f toLight = lightSample.p - prd.hitP;
  const float dist2 = dot(toLight, toLight);
  if (dist2 <= 1e-7f || lightSample.pdfA <= 0.f)
    return false;

  const float dist = sqrtf(dist2);
  const vec3f wi = toLight * (1.f / dist);

  const float NoI = fmaxf(dot(prd.N, wi), 0.f);
  const float NoL = fmaxf(dot(lightSample.n, -wi), 0.f);
  if (NoI <= 0.f || NoL <= 0.f)
    return false;

  const vec3f f = bsdf.f(wo, wi);
  const float G = NoI * NoL / dist2;
  const vec3f unshadowedContribution = lightSample.Le * f * G;
  const float target = pt::restirTargetFromRgb(toPtVec(unshadowedContribution));
  if (target <= 0.f)
    return false;

  out.sample.lightId = lightSample.lightId;
  out.sample.uv = toPtVec(uSurface);
  out.sample.sourcePdf = lightSample.pdfA;
  out.sample.target = target;
  out.sample.position = toPtVec(lightSample.p);
  out.sample.normal = toPtVec(lightSample.n);
  out.sample.emission = toPtVec(lightSample.Le);
  out.wi = toPtVec(wi);
  out.unshadowedContribution = toPtVec(unshadowedContribution);
  return true;
}

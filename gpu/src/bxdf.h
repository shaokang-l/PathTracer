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
#include "bxdfFlags.h"
#include "orthoBasis.h"
#include "utils.h"

namespace mypt {
  // // ---------- : one-sample MIS-free interface ----------------
  // //
  //     // BSDF sample definition
  struct BSDFSample {
      vec3f f; // bsdf value
      vec3f wi; // world space direction
      float pdf; // 
      BxDFFlags flag;
  };

  // This is the BxDF layer sampling, it accepts wo_local, and returns wi_local,f, pdf, and flag.
  __device__ inline bool sampleBxDF(const MaterialGPU &mat,
                                    const vec3f       &wo,   // view dir, pointing AWAY from surface
                                    const float uc, // 1D, lobe branch picking
                                    const vec2f u, // 2D, lobe sampling
                                    BSDFSample        &out)
  {
    switch (mat.kind) {

    case MATERIAL_LAMBERTIAN: {
      out.wi = sampleCosineHemisphere(u);
      out.f = mat.albedo / float(M_PI);
      out.pdf = pdfCosineHemisphere(local::absCosTheta(out.wi));
      out.flag = BxDFFlags::DIFFUSE_REFLECTION;
      return true;
    }

    // PBRT style, no need to check delta in the integrator layer
    case MATERIAL_MIRROR: {
      out.wi = vec3f(-wo.x, -wo.y, wo.z);
      out.f = mat.albedo/local::absCosTheta(out.wi);
      out.pdf = 1.f;
      out.flag = BxDFFlags::SPECULAR_REFLECTION;
      return true;
    }

    case MATERIAL_DIELECTRIC: {
      // TODO
      return false;
    }

    case MATERIAL_EMISSIVE:
    default:
      return false;
    }
  }


__device__ inline vec3f evalBxDF(const MaterialGPU &mat, vec3f wo, vec3f wi)
{
    switch (mat.kind) {
      case MATERIAL_LAMBERTIAN: {
        if (!local::sameHemisphere(wo, wi)) return vec3f(0.f);
        return mat.albedo / float(M_PI);
      }
      case MATERIAL_MIRROR: {
        return vec3f(0.f);
      }
      case MATERIAL_DIELECTRIC: {
        return vec3f(0.f);
    };
  }
  return vec3f(0.f);
};



__device__ inline float pdfBxDF(const MaterialGPU &mat, vec3f wo, vec3f wi){
  switch (mat.kind) {
    case MATERIAL_LAMBERTIAN: {
      if (!local::sameHemisphere(wo, wi)) return 0.f;
      return pdfCosineHemisphere(local::absCosTheta(wi));
    }
    case MATERIAL_MIRROR: {
      return 0.f;
    }
    case MATERIAL_DIELECTRIC: {
      return 0.f;
    }
  }
  return 0.f;
};



}// namespace mypt
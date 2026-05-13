// ======================================================================== //
// bxdfDispatch.h - tagged-union dispatch over MaterialGPU.kind.
//
// This file is the "router": each switch case constructs the relevant
// BxDF struct from the flat MaterialGPU params and forwards to its
// Sample_f / f / pdf method. No BSDF logic lives here; that's all in
// bxdf/<Material>.h.
//
// Adding a new BxDF:
//   1. Drop a new file under bxdf/<NewBxDF>.h with the same three methods.
//   2. Add an enum value to MaterialKind in material.h.
//   3. Add the relevant params to MaterialGPU (or split it later).
//   4. Add one case to each of the three switches below.
// ======================================================================== //

#pragma once

#include "material.h"
#include "bsdfSample.h"
#include "bxdf/Lambertian.h"
#include "bxdf/Mirror.h"
#include "bxdf/Dielectric.h"
#include "bxdf/Conductor.h"
#include "bxdf/ThinDielectric.h"

namespace mypt {

  __device__ inline bool sampleBxDF(const MaterialGPU &mat,
                                    const vec3f       &wo,   // local
                                    float              uc,
                                    vec2f              u,
                                    BSDFSample        &out)
  {
    switch (mat.kind) {
    case MATERIAL_LAMBERTIAN: { LambertianBxDF b{mat.albedo}; return b.Sample_f(wo, uc, u, out); }
    case MATERIAL_MIRROR:     { MirrorBxDF     b{mat.albedo}; return b.Sample_f(wo, uc, u, out); }
    case MATERIAL_DIELECTRIC: { DielectricBxDF b{mat.ior, mat.alpha_x, mat.alpha_y}; return b.Sample_f(wo, uc, u, out); }
    case MATERIAL_CONDUCTOR:  { ConductorBxDF b{mat.eta, mat.k, mat.albedo, mat.alpha_x, mat.alpha_y}; return b.Sample_f(wo, uc, u, out); }
    case MATERIAL_THIN_DIELECTRIC: { ThinDielectricBxDF b{mat.ior}; return b.Sample_f(wo, uc, u, out); }
    case MATERIAL_EMISSIVE:
    default: return false;
    }
  }

  __device__ inline vec3f evalBxDF(const MaterialGPU &mat,
                                   const vec3f       &wo,    // local
                                   const vec3f       &wi)    // local
  {
    switch (mat.kind) {
    case MATERIAL_LAMBERTIAN: { LambertianBxDF b{mat.albedo}; return b.f(wo, wi); }
    case MATERIAL_MIRROR:     { MirrorBxDF     b{mat.albedo}; return b.f(wo, wi); }
    case MATERIAL_DIELECTRIC: { DielectricBxDF b{mat.ior, mat.alpha_x, mat.alpha_y}; return b.f(wo, wi); }
    case MATERIAL_CONDUCTOR:  { ConductorBxDF b{mat.eta, mat.k, mat.albedo, mat.alpha_x, mat.alpha_y}; return b.f(wo, wi); }
    case MATERIAL_THIN_DIELECTRIC: { ThinDielectricBxDF b{mat.ior}; return b.f(wo, wi); }
    default: return vec3f(0.f);
    }
  }

  __device__ inline float pdfBxDF(const MaterialGPU &mat,
                                  const vec3f       &wo,     // local
                                  const vec3f       &wi)     // local
  {
    switch (mat.kind) {
    case MATERIAL_LAMBERTIAN: { LambertianBxDF b{mat.albedo}; return b.pdf(wo, wi); }
    case MATERIAL_MIRROR:     { MirrorBxDF     b{mat.albedo}; return b.pdf(wo, wi); }
    case MATERIAL_DIELECTRIC: { DielectricBxDF b{mat.ior, mat.alpha_x, mat.alpha_y}; return b.pdf(wo, wi); }
    case MATERIAL_CONDUCTOR:  { ConductorBxDF b{mat.eta, mat.k, mat.albedo, mat.alpha_x, mat.alpha_y}; return b.pdf(wo, wi); }
    case MATERIAL_THIN_DIELECTRIC: { ThinDielectricBxDF b{mat.ior}; return b.pdf(wo, wi); }
    default: return 0.f;
    }
  }

  __device__ inline BxDFFlags flagsBxDF(const MaterialGPU &mat)
  {
    switch (mat.kind) {
    case MATERIAL_LAMBERTIAN: return BxDFFlags::DIFFUSE_REFLECTION;
    case MATERIAL_MIRROR:     return BxDFFlags::SPECULAR_REFLECTION;
    case MATERIAL_DIELECTRIC:
      return effectivelySmooth(mat.alpha_x, mat.alpha_y)
        ? static_cast<BxDFFlags>(BxDFFlags::SPECULAR_REFLECTION | BxDFFlags::SPECULAR_TRANSMISSION)
        : static_cast<BxDFFlags>(BxDFFlags::GLOSSY_REFLECTION | BxDFFlags::GLOSSY_TRANSMISSION);
    case MATERIAL_CONDUCTOR:
      return effectivelySmooth(mat.alpha_x, mat.alpha_y)
        ? BxDFFlags::SPECULAR_REFLECTION
        : BxDFFlags::GLOSSY_REFLECTION;
    case MATERIAL_THIN_DIELECTRIC:
      return static_cast<BxDFFlags>(BxDFFlags::SPECULAR_REFLECTION | BxDFFlags::SPECULAR_TRANSMISSION);
    case MATERIAL_EMISSIVE:
    default:                  return BxDFFlags::NONE;
    }
  }

} // namespace mypt

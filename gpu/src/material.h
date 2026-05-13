// ======================================================================== //
// material.h - host/device material PODs for the OWL path tracer.
//
// Keep this file GPU-friendly: POD data only, no virtuals, no STL.
// ======================================================================== //

#pragma once

#include <owl/common/math/vec.h>

using namespace owl;

// For a first pass we keep a single tagged POD material struct on the
// device. When porting the CPU OOP material hierarchy, this is the struct to
// grow or eventually replace with per-material SBT records.
enum MaterialKind : int {
  MATERIAL_LAMBERTIAN = 0,
  MATERIAL_MIRROR,
  MATERIAL_DIELECTRIC,
  MATERIAL_CONDUCTOR,
  MATERIAL_THIN_DIELECTRIC,
  MATERIAL_EMISSIVE,
};

struct MaterialGPU {
  int    kind;           // MaterialKind
  vec3f  albedo;         // lambertian / mirror base color
  vec3f  emission;       // emissive radiance
  vec3f  eta;            // conductor RGB eta
  vec3f  k;              // conductor RGB absorption
  float  ior;            // dielectric index of refraction
  float  roughness;      // reserved for future use
  float  alpha_x;        // microfacet roughness in tangent direction
  float  alpha_y;        // microfacet roughness in bitangent direction
  int    thinUseSplitRay; // reserved for future split-ray thin dielectric
};

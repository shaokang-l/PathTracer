// ======================================================================== //
// deviceCode.h - shared host <-> device POD types for the OWL path tracer.
//
// Everything here is compiled on BOTH sides:
//   - included from hostCode/Renderer.cpp (plain C++)
//   - included from deviceCode.cu         (CUDA / OptiX)
// Keep it PODs, no virtuals, no STL.
// ======================================================================== //

#pragma once

#include <owl/owl.h>
#include <owl/common/math/vec.h>

using namespace owl;

// ------------------------------------------------------------------
// Materials
// ------------------------------------------------------------------
//
// For a first pass we keep a single tagged POD material struct on the
// device. When porting your OOP BSDF hierarchy, this is the struct you
// grow / replace first:
//   - either extend the union-of-params pattern below,
//   - or introduce per-material-type OWLGeomTypes with dedicated
//     closest-hit programs (see owl/samples/cmdline/s05-rtow).
//
enum MaterialKind : int {
  MATERIAL_LAMBERTIAN = 0,
  MATERIAL_MIRROR,
  MATERIAL_DIELECTRIC,
  MATERIAL_EMISSIVE,
};

struct MaterialGPU {
  int    kind;           // MaterialKind
  vec3f  albedo;         // lambertian / mirror base color
  vec3f  emission;       // emissive radiance
  float  ior;            // dielectric index of refraction
  float  roughness;      // reserved for future use
};
// ------------------------------------------------------------------
// Per-geom SBT record (one per OWLGeom, filled via owlGeomSet*)
// ------------------------------------------------------------------
struct TriangleMeshSBT {
  vec3f       *vertex;   // device pointer
  vec3i       *index;    // device pointer
  int32_t     materialId;
};

// ------------------------------------------------------------------
// RayGen SBT record
// ------------------------------------------------------------------
//
// Intentionally empty - all per-frame data lives in LaunchParams so
// we can update it every frame without rebuilding the SBT.
//
struct RayGenData {
  int _dummy;
};

// ------------------------------------------------------------------
// Miss SBT record
// ------------------------------------------------------------------
struct MissProgData {
  vec3f skyColorTop;
  vec3f skyColorBottom;
};

// ------------------------------------------------------------------
// LaunchParams - updated every frame, no SBT rebuild needed
// ------------------------------------------------------------------
struct LaunchParams {
  // accumulator: HDR, one float4 per pixel, cleared on reset
  float4              *accumBuffer;
  // final display buffer: 8-bit RGBA, owned by OWLViewer via CUDA/GL interop
  uint32_t            *fbPtr;
  // global material buffer, use materialId to index into this buffer
  vec2i                fbSize;
  MaterialGPU         *materials;
  int                  accumID;       // frames accumulated so far
  int                  samplesPerPixel;
  int                  maxBounces;

  OptixTraversableHandle world;

  struct {
    vec3f pos;
    vec3f dir_00;   // lower-left ray direction
    vec3f dir_du;   // per-pixel x delta
    vec3f dir_dv;   // per-pixel y delta
  } camera;
};

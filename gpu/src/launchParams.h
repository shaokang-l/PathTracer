// ======================================================================== //
// launchParams.h - host/device launch and program data.
//
// Everything here is compiled on both sides. Keep it POD-only so the host
// OWL bindings and device programs agree on layout.
// ======================================================================== //

#pragma once

#include "light.h"
#include "material.h"

#include <cstdint>
#include <owl/owl.h>
#include <owl/common/math/vec.h>

using namespace owl;

// RayGen SBT record.
//
// Intentionally empty: all per-frame data lives in LaunchParams so it can be
// updated every frame without rebuilding the SBT.
struct RayGenData {
  int _dummy;
};

// Miss SBT record.
struct MissProgData {
  vec3f skyColorTop;
  vec3f skyColorBottom;
};

// LaunchParams - updated every frame, no SBT rebuild needed.
struct LaunchParams {
  // Accumulator: HDR, one float4 per pixel, cleared on reset.
  float4              *accumBuffer;

  // Final display buffer: 8-bit RGBA, owned by OWLViewer via CUDA/GL interop.
  uint32_t            *fbPtr;

  vec2i                fbSize;

  // Global material buffer, indexed by TriangleMeshSBT::materialId.
  MaterialGPU         *materials;
  mypt::LightGPU      *lights;
  int                  lightCount;

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

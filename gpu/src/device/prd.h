// ======================================================================== //
// prd.h - device-side per-ray data records.
// ======================================================================== //

#pragma once

#include "pod/material.h"

#include <owl/common/math/vec.h>

using namespace owl;

struct PRD {
  vec3f hitP;
  vec3f N;
  int   materialId;
  bool  didHit;
  bool  isEmissive;
  vec3f emission;
};

struct ShadowPRD {
  vec3f transmittance;
};

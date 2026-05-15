// ======================================================================== //
// bsdfSample.h - one-sample BSDF result, shared by both BxDF (local space)
// and BSDF (world space) layers.
//
// PBRT convention: the same struct is reused; only `wi`'s coordinate space
// differs by layer (local in BxDF, world in BSDF). f / pdf / flag are
// space-invariant and pass through the wrapper unchanged.
// ======================================================================== //

#pragma once

#include <owl/common/math/vec.h>
#include "shading/bxdfFlags.h"

namespace pt {

  struct BSDFSample {
    owl::vec3f f;     // BSDF value (space-invariant)
    owl::vec3f wi;    // sampled direction (BxDF: local; BSDF: world)
    float      pdf;   // probability density
    BxDFFlags  flag;  // lobe flags (specular, diffuse, transmission, ...)
  };

} // namespace pt

// ======================================================================== //
// restirState.h - GPU-side ReSTIR DI persistent state.
//
// Stage A only stores enough per-pixel state to inspect no-reuse RIS output
// and prepare for temporal reuse. Temporal reprojection/merge is intentionally
// not implemented here yet.
// ======================================================================== //

#pragma once

#include "pt/restir/reservoir.h"

#include <owl/common/math/vec.h>

namespace mypt {

  struct RestirSurfaceData {
    owl::vec3f hitP = owl::vec3f(0.f);
    owl::vec3f normal = owl::vec3f(0.f);
    int materialId = -1;
    int valid = 0;
  };

} // namespace mypt

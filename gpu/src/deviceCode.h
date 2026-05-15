// ======================================================================== //
// deviceCode.h - compatibility umbrella for shared GPU POD types.
//
// Prefer including the focused headers directly in new code:
//   - material.h      -> MaterialKind / MaterialGPU
//   - light.h         -> LightKind / LightGPU / LightSample
//   - geometryData.h  -> geometry SBT records
//   - launchParams.h  -> RayGenData / MissProgData / LaunchParams
// ======================================================================== //

#pragma once

#include "pod/geometryData.h"
#include "pod/launchParams.h"
#include "pod/material.h"
#include "pod/rayTypes.h"
#include "pt/restir/reservoir.h"
#include "pt/restir/target.h"
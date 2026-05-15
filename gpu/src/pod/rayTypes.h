#pragma once

enum RayType : int {
    RADIANCE_RAY_TYPE = 0,
    SHADOW_RAY_TYPE   = 1,
    RAY_TYPE_COUNT    = 2,
};

#ifdef __CUDACC__
#include <owl/owl_device.h>

namespace pt {
  using RadianceRay = owl::RayT<RADIANCE_RAY_TYPE, RAY_TYPE_COUNT>;
  using ShadowRay   = owl::RayT<SHADOW_RAY_TYPE,   RAY_TYPE_COUNT>;
}
#endif

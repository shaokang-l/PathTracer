#pragma once
#include <owl/common/math/vec.h>

namespace pt {

    enum BxDFFlags : uint32_t {
      NONE                  = 0,
      REFLECTION            = 1u << 0,
      TRANSMISSION          = 1u << 1,
      DIFFUSE               = 1u << 2,
      GLOSSY                = 1u << 3,
      SPECULAR              = 1u << 4,
      DIFFUSE_REFLECTION    = DIFFUSE  | REFLECTION,
      DIFFUSE_TRANSMISSION  = DIFFUSE  | TRANSMISSION,
      GLOSSY_REFLECTION     = GLOSSY   | REFLECTION,
      GLOSSY_TRANSMISSION   = GLOSSY   | TRANSMISSION,
      SPECULAR_REFLECTION   = SPECULAR | REFLECTION,
      SPECULAR_TRANSMISSION = SPECULAR | TRANSMISSION,
      ALL                   = 0xFFFFFFFFu,
    };
    
    __both__ inline bool isSpecular    (uint32_t f) { return f & SPECULAR; }
    __both__ inline bool isDiffuse     (uint32_t f) { return f & DIFFUSE; }
    __both__ inline bool isGlossy      (uint32_t f) { return f & GLOSSY; }
    __both__ inline bool isReflection(uint32_t f) { return f & REFLECTION; }
    __both__ inline bool isTransmission(uint32_t f) { return f & TRANSMISSION; }
    __both__ inline bool hasNonDelta(uint32_t f) { return isDiffuse(f) || isGlossy(f); }
    
} // namespace pt
// ======================================================================== //
// ptInterop.h - tiny conversions between OWL vectors and common pt vectors.
//
// Keep these out of OptiX program code so deviceCode.cu can stay focused on
// tracing and shading logic.
// ======================================================================== //

#pragma once

#include "pt/math/vec.h"
#include "pt/render/debug_view_kind.h"
#include "pt/render/direct_light_mode.h"

#include <owl/common/math/vec.h>

using namespace owl;

namespace pt {

  __both__ inline pt::Vec2f toPtVec(vec2f v)
  {
    return pt::Vec2f(v.x, v.y);
  }

  __both__ inline pt::Vec3f toPtVec(vec3f v)
  {
    return pt::Vec3f(v.x, v.y, v.z);
  }

  __both__ inline vec2f fromPtVec(pt::Vec2f v)
  {
    return vec2f(v.x, v.y);
  }

  __both__ inline vec3f fromPtVec(pt::Vec3f v)
  {
    return vec3f(v.x, v.y, v.z);
  }

  __both__ inline pt::DebugViewKind toDebugViewKind(int value)
  {
    return static_cast<pt::DebugViewKind>(value);
  }

  __both__ inline pt::DirectLightMode toDirectLightMode(int value)
  {
    return static_cast<pt::DirectLightMode>(value);
  }

} // namespace pt

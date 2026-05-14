// ======================================================================== //
// light.h - host/device light PODs for direct-light sampling.
//
// Keep this GPU-friendly: flat tagged data, no virtuals, no STL.
// ======================================================================== //

#pragma once

#include <owl/common/math/vec.h>

using namespace owl;

namespace mypt {

enum LightKind : int {
  LIGHT_QUAD = 0,
};

struct LightGPU {
  int   kind;       // LightKind
  vec3f emission;   // emitted radiance

  // Quad light represented as p(u,v) = v0 + u * edgeU + v * edgeV.
  vec3f v0;
  vec3f edgeU;
  vec3f edgeV;
  vec3f normal;
  float area;
};

struct LightSample {
  vec3f p;
  vec3f n;
  vec3f Le;
  float pdfA;       // area-measure pdf, including light selection if used
};

// currently a uniformly choose a light from the light list.
__both__ inline bool sampleLight(const LightGPU* lights,
                                    int lightCount,
                                    float uLight,
                                    vec2f uSurface,
                                    LightSample &out)
{
    if (!lights || lightCount <= 0) return false;
    int lightID = int(uLight * lightCount);
    if (lightID >= lightCount) lightID = lightCount - 1;
    const LightGPU &light = lights[lightID];
    switch (light.kind) {
    case LIGHT_QUAD:
      out.p = light.v0 + uSurface.x * light.edgeU + uSurface.y * light.edgeV;
      out.n = light.normal;
      out.Le = light.emission;
      if (light.area <= 0.f) return false;
      out.pdfA = (1.f / float(lightCount)) * (1.f / light.area);
      return true;
    default:
      return false;
    }
}

} // namespace mypt
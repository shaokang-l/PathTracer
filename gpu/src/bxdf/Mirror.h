// ======================================================================== //
// bxdf/Mirror.h - perfectly specular reflector.
//
// Delta-function BSDF: f and pdf are zero except along the mirror direction.
// PBRT convention: Sample_f returns f = albedo / |cos(wi)|, pdf = 1, so the
// integrator's generic update  throughput *= f * |cos| / pdf  collapses to
// throughput *= albedo without a special case.
// ======================================================================== //

#pragma once

#include "../bsdfSample.h"
#include "../bxdfFlags.h"
#include "../orthoBasis.h"
#include "../utils.h"

namespace mypt {

  struct MirrorBxDF {
    vec3f albedo;

    __both__ inline bool Sample_f(const vec3f &wo,
                                  float        /*uc*/,
                                  vec2f        /*u*/,
                                  BSDFSample  &out) const
    {
      out.wi   = vec3f(-wo.x, -wo.y, wo.z);
      out.f    = albedo / local::absCosTheta(out.wi);
      out.pdf  = 1.f;
      out.flag = BxDFFlags::SPECULAR_REFLECTION;
      return true;
    }

    __both__ inline vec3f f(const vec3f &/*wo*/, const vec3f &/*wi*/) const
    {
      return vec3f(0.f);
    }

    __both__ inline float pdf(const vec3f &/*wo*/, const vec3f &/*wi*/) const
    {
      return 0.f;
    }
  };

} // namespace mypt

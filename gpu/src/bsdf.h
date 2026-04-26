#pragma once
#include "bxdf.h"
#include "orthoBasis.h"

// This BSDF is a wrapper for the BxDF class, it processes stuff from world space
// converting to local space and calling the BxDF class.

namespace mypt{

struct BSDF{
    OrthoBasis basis;
    const MaterialGPU *mat;

    __device__ BSDF(OrthoBasis basis, const MaterialGPU *mat) : basis(basis), mat(mat) {}

    __device__ inline bool sample_f(const vec3f &wo, float uc, vec2f u, BSDFSample &out) const {
        vec3f wo_local = basis.toLocal(wo);
        if(!sampleBxDF(*mat, wo_local, uc, u, out)) return false;
        out.wi = basis.toWorld(out.wi);
        return true;
    }

    __device__ inline vec3f f(const vec3f &wo, const vec3f &wi) const {
        vec3f wo_local = basis.toLocal(wo);
        vec3f wi_local = basis.toLocal(wi);
        return evalBxDF(*mat, wo_local, wi_local);
    }

    __device__ inline float pdf(const vec3f &wo, const vec3f &wi) const {
        vec3f wo_local = basis.toLocal(wo);
        vec3f wi_local = basis.toLocal(wi);
        return pdfBxDF(*mat, wo_local, wi_local);
    }
};

} // namespace mypt
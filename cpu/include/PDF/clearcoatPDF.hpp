#pragma once
#include "material/materialMath.hpp"
#include "PDF/pdf.hpp"
#include "utils/orthoBasis.hpp"
extern int rejects;
class ClearcoatPDF : public PDF
{
private:
    const ClearcoatTRD &distrib;
    const OrthoBasis onb;
    const gl::vec3 wo_local;

public:
    ClearcoatPDF(const ClearcoatTRD &distrib, const OrthoBasis &basis,
                 const gl::vec3 &wo_world)
        : distrib(distrib), onb(basis), wo_local(onb.toLocal(wo_world.normalize())) {};

    gl::vec3 get(float uc = gl::rand_num(),
                 gl::vec2 u = gl::vec2(gl::rand_num(),
                                       gl::rand_num())) const override
    {
        // sample half‐vector in local
        gl::vec3 m_local = distrib.sample_h(u);
        gl::vec3 wi_local = gl::pbrt::reflect(wo_local, m_local);
        // if it went below the surface, just flip

        if (!gl::pbrt::sameHemisphere(wo_local, wi_local))
#ifdef BIASED_SAMPLING
            wi_local.z() = -wo_local.z();
#elif defined DISCARD_SAMPLING
            return gl::vec3(0.f);
#else
            do
            {
                rejects++;
                using namespace gl;
                vec2 u2 = vec2(rand_num(), rand_num());
                vec3 m = distrib.sample_h(u2);
                wi_local = gl::pbrt::reflect(wo_local, m);
            } while (wi_local.z() <= 0);
#endif
        // back to world
        return onb.toWorld(wi_local);
    };

    // PDF for a given outgoing direction
    float at(const gl::vec3 &wi_world) const override
    {
        gl::vec3 wi = onb.toLocal(wi_world.normalize());
        if (wi.z() <= 0)
            return 0.f;
        // half‐vector
        gl::vec3 h = (wo_local + wi).normalize();
        auto denominator = 4 * std::abs(dot(wi, h));
        if (denominator < 1e-6)
            return 0.f;
        float abs_n_dot_h = gl::pbrt::absCosTheta(h);

        return distrib.D(h) * abs_n_dot_h / denominator;
    }
};
#pragma once
#include "utils/matrix.hpp"
#include "utils/utility.hpp"
#include "utils/orthoBasis.hpp"
namespace gl
{
    inline float HenyeyGreenstein(float cos_theta, float g)
    {
        float upper = 1 - square(g);
        // this (+) is for PBRT convention (wi,wo);
        float lower = 1 + square(g) + 2 * g * cos_theta;
        lower *= safeSqrt(lower);
        float INV4PI = 1 / (4 * M_PI);
        return upper * INV4PI / lower;
    };

    inline gl::vec3 sampleHenyeyGreenstein(const vec3 &wo_world, float g, const gl::vec2 &u, float *pdf = nullptr)
    {
        vec3 wo_normalized = wo_world.normalize();
        float cos_theta = 0.f;
        if (std::fabs(g) < 1e-3)
        {
            cos_theta = 1 - 2 * u[0];
        }
        else
        {
            cos_theta = -1 / (2 * g) *
                        (1 + square(g) - square((1 - square(g)) / (1 + g - 2 * g * u[0])));
        }

        float sin_theta = safeSqrt(1 - square(cos_theta));
        float phi = 2 * M_PI * u[1];

        OrthoBasis onb(wo_normalized);
        vec3 wi_local = sphericalDirection(sin_theta, cos_theta, phi);
        vec3 wi_world = onb.toWorld(wi_local);
        if (pdf)
        {
            *pdf = HenyeyGreenstein(cos_theta, g);
        }
        return wi_world;
    }
};
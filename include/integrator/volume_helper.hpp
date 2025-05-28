#pragma once
#include "base/lightList.hpp"
#include "base/objectList.hpp"
#include "config.hpp"
#include "primitives/bvh.hpp"
#include "PDF/hittablePDF.hpp"
#include "PDF/mixedPDF.hpp"
#include "sampler/sampler.hpp"
#include <concepts> // For std::invocable and std::same_as

// MediumProperties is the type returned by curr_medium->sample_properties_at(...).
template <typename Callable, typename PosType, typename PropsType, typename SigmaType, typename TransmittanceType>
concept VolumeIntegratorCallable = requires(Callable &&func, PosType &&pos, PropsType &&props, SigmaType &&sigma, TransmittanceType &&transmittance) {
    { std::forward<Callable>(func)(std::forward<PosType>(pos), std::forward<PropsType>(props), std::forward<SigmaType>(sigma), std::forward<TransmittanceType>(transmittance)) } -> std::same_as<bool>;
};

template <typename F>
    requires VolumeIntegratorCallable<F, gl::vec3, MediumProperties, gl::vec3, gl::vec3>
gl::vec3 sampleT_maj(Ray &ray, float t_max, float u, F callback)
{
    using namespace gl;

    t_max *= ray.direction.length(); // handle the case where the ray is not normalized
    ray.direction.normalized();

    auto curr_medium = ray.current_medium;
    auto ray_majorant_iter = curr_medium->get_ray_majorant_iterator(ray, t_max);
    if (!ray_majorant_iter)
        return vec3(0.f);

    vec3 T_maj(1.f);
    bool is_terminated = false;
    while (!is_terminated)
    {
        auto seg = ray_majorant_iter->next();
        if (!seg)
            return T_maj;

        float t_min = seg->t_min;
        float t_max = seg->t_max;

        if (seg->sigma_maj[0] == 0)
        {
            float dt = seg->t_max - seg->t_min;
            if (isInf(dt))
                dt = FLT_MAX;
            T_maj *= exp(-seg->sigma_maj * dt);
            continue;
        }

        float tMin = seg->t_min;
        while (true)
        {
            // generate sample along current segment
            float t = tMin + (-log(1 - u) / seg->sigma_maj[0]);
            u = rand_num();
            if (t < seg->t_max)
            {
                // sample is inside segment
                T_maj *= fastExp(-(t - tMin) * seg->sigma_maj);
                MediumProperties properties = curr_medium->sample_properties_at(ray.at(t));
                if (!callback(ray.at(t), properties, seg->sigma_maj, T_maj))
                {
                    is_terminated = true;
                    break;
                }
                T_maj = gl::vec3(1.f);
                tMin = t;
            }
            else
            {
                // sample is outside segment
                float dt = seg->t_max - t_min;
                T_maj *= fastExp(-dt * seg->sigma_maj);
                break;
            }
        }
    }

    return vec3(1.f);
}

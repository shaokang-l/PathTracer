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

void update_medium_at_interface(Ray &ray, const HitRecord &hit_record)
{
    if (hit_record.medium_interface)
    {
        ray.current_medium = hit_record.is_inside ? hit_record.medium_interface->outside : hit_record.medium_interface->inside;
    }
    else
    {
        ray.current_medium = nullptr;
    }
};

inline gl::vec3 nee_estimate(const gl::vec3 &p_scatter,
                             const gl::vec3 &wo_world, const int max_bounces,
                             const std::shared_ptr<Medium> &init_medium,
                             const Hittable &prims,
                             const LightList &lights,
                             std::shared_ptr<BVHNode> bvh)
{
    using namespace gl;

    gl::vec3 L_nee_accum(0.0f);

    int shadow_bounces = 0;
    float T_light = 1.0f;
    float p_trans_dir = 1.0f; // for MIS
    if (lights.size() == 0 || LIGHT_SAMPLE_NUM == 0)
        return gl::vec3(0.0f);

    auto light = lights.uniform_get(); // Uniformly pick one light from the list
    if (!light)
        return gl::vec3(0.0f);

    auto p_prime = light->get_sample(rand_num(), rand_num());
    auto n_prime = light->get_normal_at(p_prime);
    auto p_prime_pdf = 1.0f / (light->get_area() * lights.size());

    auto p = p_scatter;
    auto shadow_ray = Ray(p_prime, (p_prime - p).normalize(), 1.0f, init_medium);

    while (true)
    {
        shadow_ray.origin = p;
        shadow_ray.direction = (p_prime - p).normalize();
        HitRecord shadow_hit_record;
        bool is_shadow_hit = false;
        is_shadow_hit = bvh ? bvh->intersect(shadow_ray, shadow_hit_record, 0.001f, (p_prime - p).length() - 0.001f)
                            : prims.intersect(shadow_ray, shadow_hit_record, 0.001f, (p_prime - p).length() - 0.001f);

        float next_t = (p_prime - p).length();
        if (is_shadow_hit)
            next_t = shadow_hit_record.t;

        if (shadow_ray.current_medium)
        {
            MediumProperties medium_props = shadow_ray.current_medium->sample_properties_at(shadow_ray.at(next_t));
            float curr_sigma_t = maxComponent(medium_props.sigma_t());
            T_light *= exp(-curr_sigma_t * next_t);
            p_trans_dir *= exp(-curr_sigma_t * next_t);
        }

        // nothing blocking
        if (!is_shadow_hit)
            break;
        else
        {
            // blocked by opaque surface (note that we consider dielectric surface as opaque)
            if (shadow_hit_record.material)
                return vec3(0.0f);

            shadow_bounces++;
            if (shadow_bounces >= max_bounces)
                return vec3(0.0f);

            update_medium_at_interface(shadow_ray, shadow_hit_record);
            p = shadow_ray.at(next_t);
        }
    };

    if (T_light > epsilon)
    {
        HitRecord light_surface_hit;
        light_surface_hit.position = p_prime;
        light_surface_hit.normal = n_prime;
        // CRITICAL ASSUMPTION: uniform light color
        light_surface_hit.texCoords = vec2(0.5f, 0.5f);
        auto Le_prime = light->L_emit(light_surface_hit, (p_scatter - p_prime).normalize());
        auto pdf_light = 1.f / (light->get_area() * lights.size());
        auto cos_theta = absDot(n_prime, (p_scatter - p_prime).normalize());
        auto phase_val = init_medium->sample_properties_at(p_scatter).phase_function->p(wo_world, (p_prime - p_scatter).normalize());
        auto G = cos_theta / square((p_scatter - p_prime).length());
        return (Le_prime * phase_val * T_light) / pdf_light;
    }

    return gl::vec3(0.0f);
}
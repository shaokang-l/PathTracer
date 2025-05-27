#pragma once
#include "base/lightList.hpp"
#include "base/objectList.hpp"
#include "config.hpp"
#include "primitives/bvh.hpp"
#include "PDF/hittablePDF.hpp"
#include "PDF/mixedPDF.hpp"
#include "sampler/sampler.hpp"

inline gl::vec3 getRayColor(Ray &ray, const ObjectList &prims,
                            gl::vec3 bg_color, const LightList &lights,
                            uint max_depth = 40,
                            std::shared_ptr<BVHNode> bvh = nullptr, const std::shared_ptr<Medium> &global_medium = nullptr)
{
    using namespace gl;

    if (!global_medium)
        return vec3(0.f);

    ray.current_medium = global_medium;

    vec3 accum_color(0.f);
    vec3 throughput(1.f);

    for (int bounce = 0; bounce < max_depth; ++bounce)
    {
        HitRecord surface_rec;
        bool intersected_surface = false;
        float t_surface = FLT_MAX;

        // 1. Find distance to nearest surface along the current ray
        if (bvh)
            intersected_surface = bvh->intersect(ray, surface_rec, 0.001f, FLT_MAX);
        else
            intersected_surface = prims.intersect(ray, surface_rec, 0.001f, FLT_MAX);

        if (intersected_surface)
            t_surface = surface_rec.t;

        float t_medium_interaction = FLT_MAX;
        bool sampled_medium_event = false;
        if (ray.current_medium)
        {
            MediumProperties medium_properties = ray.current_medium->sample_properties_at(ray.origin);
            float curr_sigma_t = maxComponent(medium_properties.sigma_t());

            if (curr_sigma_t > epsilon)
            {
                t_medium_interaction = -log(1 - rand_num()) / curr_sigma_t;
            }

            // medium event occurs before surface interaction
            if (t_medium_interaction < t_surface)
            {
                sampled_medium_event = true;
            }
        }

        float t_event = sampled_medium_event ? t_medium_interaction : t_surface;

        if (ray.current_medium && t_event > 0.f && t_event < FLT_MAX)
        {
            MediumProperties medium_properties = ray.current_medium->sample_properties_at(ray.origin);
            float curr_sigma_t = maxComponent(medium_properties.sigma_t());
            throughput *= exp(-curr_sigma_t * t_event);
        }

        if (throughput.length() < epsilon)
            break;

        // process event
        if (sampled_medium_event)
        {
            // medium interaction branch
            ray.origin = ray.at(t_event);
            MediumProperties medium_properties = ray.current_medium->sample_properties_at(ray.origin);
            vec3 sigma_s = medium_properties.sigma_s;
            vec3 sigma_t = medium_properties.sigma_t();
            vec3 sigma_a = medium_properties.sigma_a;
            vec3 albedo = sigma_s / sigma_t;

            float prob_scatter = maxComponent(albedo);

            if (rand_num() < prob_scatter)
            {
                // scatter
                // weight
                throughput *= albedo / prob_scatter;

                const auto &phase_function = medium_properties.phase_function;
                if (!phase_function)
                    throughput = vec3(0.f);

                PhaseRecord phase_rec;
                vec2 u = vec2(rand_num(), rand_num());
                const auto &wo_world = -ray.direction.normalize();
                bool phase_sampled = phase_function->sample_p(wo_world, phase_rec, u);

                if (!phase_sampled)
                    throughput = vec3(0.f);

                throughput *= phase_rec.p / phase_rec.pdf;
                ray.direction = phase_rec.wi.normalize();
            }
            else
            {
                // absorption
                accum_color += throughput * medium_properties.Le;
                throughput = vec3(0.f);
                break;
            }
        }
        else
        {
            // hit surface branch
            // no hit
            if (!intersected_surface)
                accum_color += throughput * bg_color;
            else
            {
                ray.origin = surface_rec.position;

                if (surface_rec.material && surface_rec.material->is_emitter())
                {
                    accum_color += throughput * surface_rec.material->emit(ray, surface_rec);
                }

                const auto &mi = surface_rec.medium_interface;
                // handle medium transition
                if (mi)
                {
                    ray.current_medium = surface_rec.is_inside ? mi->inside : mi->outside;
                    if (surface_rec.material && surface_rec.material->is_emitter())
                        break;
                }
                else
                {
                    ray.current_medium = nullptr;
                }
            }
        }
    }

    return accum_color;
}

// inline gl::vec3 estimate_l_scatter1(const gl::vec3 &p_scatter,
//                                     const gl::vec3 &wo_world,
//                                     const std::shared_ptr<Medium> &medium,
//                                     const Hittable &prims,
//                                     const LightList &lights,
//                                     std::shared_ptr<BVHNode> bvh)
// {
//     using namespace gl;
//     if (!medium)
//         return gl::vec3(0.0f);

//     MediumProperties medium_props = medium->sample_properties_at(p_scatter);
//     if (!medium_props.phase_function)
//         return gl::vec3(0.0f);

//     gl::vec3 L_scatter1_accum(0.0f);
//     if (lights.size() > 0 && LIGHT_SAMPLE_NUM > 0)
//     {
//         for (int i = 0; i < LIGHT_SAMPLE_NUM; i++) // Loop for Number of Direct Light Samples
//         {
//             auto light = lights.uniform_get(); // Uniformly pick one light from the list
//             if (!light)
//                 continue;

//             auto p_prime = light->get_sample(rand_num(), rand_num());
//             auto n_prime = light->get_normal_at(p_prime);
//             auto dir_prime_to_scatter = normalize(p_scatter - p_prime);
//             auto distance_prime_to_scatter = (p_scatter - p_prime).length();

//             Ray shadow_ray(p_prime, dir_prime_to_scatter);
//             HitRecord shadow_hit_record;
//             bool is_shadow_hit = false;
//             is_shadow_hit = bvh ? bvh->intersect(shadow_ray, shadow_hit_record, 0.001f, distance_prime_to_scatter - 0.001f)
//                                 : prims.intersect(shadow_ray, shadow_hit_record, 0.001f, distance_prime_to_scatter - 0.001f);
//             auto V = is_shadow_hit ? 0.0f : 1.0f;

//             if (V > 0.0f)
//             {
//                 HitRecord light_surface_hit; // HitRecord on the light's surface for L_emit
//                 light_surface_hit.position = p_prime;
//                 light_surface_hit.normal = n_prime;
//                 // CRITICAL ASSUMPTION: uniform light color
//                 light_surface_hit.texCoords = vec2(0.5f, 0.5f);
//                 auto Le_prime = light->L_emit(light_surface_hit, dir_prime_to_scatter);
//                 auto pdf_light = 1.f / (light->get_area() * lights.size());
//                 auto cos_theta = absDot(n_prime, dir_prime_to_scatter);
//                 // p is defined as both direction pointing outwards
//                 auto phase_val = medium_props.phase_function->p(wo_world, -dir_prime_to_scatter);
//                 auto transmittance = exp(-medium_props.sigma_t() * distance_prime_to_scatter);

//                 if (distance_prime_to_scatter < gl::epsilon)
//                     continue;
//                 L_scatter1_accum += (Le_prime * cos_theta * phase_val * transmittance) / pdf_light / square(distance_prime_to_scatter);
//             }
//         }

//         L_scatter1_accum /= LIGHT_SAMPLE_NUM;
//     }

//     return L_scatter1_accum;
// }

// inline gl::vec3 getRayColor(const Ray &ray, const ObjectList &prims,
//                             gl::vec3 bg_color, const LightList &lights,
//                             uint max_depth = 40,
//                             std::shared_ptr<BVHNode> bvh = nullptr, const std::shared_ptr<Medium> &global_medium = nullptr)
// {
//     using namespace gl;

//     HitRecord hit_record;
//     bool is_hit = false;

//     if (!global_medium)
//         return vec3(0.f);
//     if (bvh == nullptr)
//         is_hit = prims.intersect(ray, hit_record);
//     else
//         is_hit = bvh->intersect(ray, hit_record);

//     MediumProperties medium_properties = global_medium->sample_properties_at(ray.getOrigin());
//     float u = halton_sampler.get1D();
//     float sigma_t = maxComponent(medium_properties.sigma_t());
//     float t = -log(1 - u) / sigma_t;

//     if (t < hit_record.t)
//     {

//         // float trans_pdf = exp(-sigma_t * t) * sigma_t;
//         // float transmittance = exp(-sigma_t * t);
//         vec3 p = ray.at(t);
//         vec3 L_s1_estimate = estimate_l_scatter1(p, -ray.getDirection().normalize(), global_medium, prims, lights, bvh);
//         vec3 albedo = medium_properties.sigma_s / medium_properties.sigma_t();

//         return albedo * L_s1_estimate / sigma_t;
//     }
//     // Surface interaction occurs first (or at the same distance, or ray escapes to infinity if no hit)
//     else
//     {
//         // float trans_pdf = exp(-sigma_t * hit_record.t);
//         // vec3 transmittance = exp(-sigma_t * hit_record.t);
//         vec3 Le(0.f);

//         if (hit_record.material != nullptr && hit_record.material->is_emitter())
//             Le = hit_record.material->emit(ray, hit_record);

//         return Le;
//     }
// }

// inline gl::vec3 getRayColor(const Ray &ray, const ObjectList &prims,
//                             gl::vec3 bg_color, const LightList &lights,
//                             uint max_depth = 40,
//                             std::shared_ptr<BVHNode> bvh = nullptr, const std::shared_ptr<Medium> &global_medium = nullptr)
// {
//     using namespace gl;

//     HitRecord hit_record;
//     bool is_hit = false;
//     if (bvh == nullptr)
//         is_hit = prims.intersect(ray, hit_record);
//     else
//         is_hit = bvh->intersect(ray, hit_record);

//     if (!is_hit)
//         return bg_color;

//     gl::vec3 transmittance = gl::vec3(1.f);

//     if (global_medium)
//     {
//         // Get medium properties (for homogeneous, point doesn't matter)
//         // Using ray.origin as a dummy point for sample_point
//         MediumProperties medium_properties = global_medium->sample_properties_at(ray.getOrigin());
//         gl::vec3 sigma_a = medium_properties.sigma_a;

//         transmittance = exp(-sigma_a * std::max(0.f, hit_record.t));
//     }

//     gl::vec3 Le(0.f);
//     if (hit_record.material->is_emitter())
//         Le = hit_record.material->emit(ray, hit_record);

//     return transmittance * Le;
// }
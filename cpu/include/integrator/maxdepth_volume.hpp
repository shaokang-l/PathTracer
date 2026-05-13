#pragma once
#include "integrator/volume_helper.hpp"

inline gl::vec3
getRayColor(Ray &ray, const ObjectList &prims, gl::vec3 bg_color,
            const LightList &lights, uint max_depth = 40,
            std::shared_ptr<BVHNode> bvh = nullptr,
            const std::shared_ptr<Medium> &global_medium = nullptr)
{
    using namespace gl;

    ray.current_medium = global_medium;
    vec3 accum_L(0.f);
    vec3 throughput(1.f);
    // MIS weights
    vec3 r_u(1.f);
    vec3 r_l(1.f);
    int bounce = 0;
    bool specular_bounce = false;
    bool any_non_specular = false;
    LightSampleContext prev_context;

    while (true)
    {
        float uc = halton_sampler.get1D();
        vec2 u = halton_sampler.get2D();
        float u_mode = halton_sampler.get1D();
        float u_maj = halton_sampler.get1D();
        vec2 u_phase = halton_sampler.get2D();

        HitRecord hit_record;
        bool is_hit = false;
        if (bvh == nullptr)
            is_hit = prims.intersect(ray, hit_record);
        else
            is_hit = bvh->intersect(ray, hit_record);

        // start medium scattering sampling
        if (ray.current_medium)
        {
            bool scattered = false, terminated = false;
            float t_max = is_hit ? hit_record.t : FLT_MAX;

            vec3 T_maj = sampleT_maj(
                ray, t_max, u_maj,
                [&](vec3 p, MediumProperties mp, vec3 sigma_maj, vec3 T_maj) -> bool
                {
                    if (bounce < max_depth && mp.Le.length() > 0.f)
                    {
                        float pdf = sigma_maj[0] * T_maj[0];
                        vec3 throughput_p = throughput * T_maj / pdf;
                        // rescaled path probability for absorption
                        vec3 r_e = r_u * sigma_maj * T_maj / pdf;

                        if (r_e.length() > 0.f)
                        {
                            accum_L += throughput_p * mp.sigma_a * mp.Le / r_e.average();
                        }
                    };

                    float p_absorb = mp.sigma_a[0] / mp.sigma_t()[0];
                    float p_scatter = mp.sigma_s[0] / mp.sigma_t()[0];
                    float p_null = std::max(0.f, 1.f - p_absorb - p_scatter);

                    int mode = sampleDiscrete({p_absorb, p_scatter, p_null}, u_mode,
                                              nullptr, nullptr);
                    // absorption
                    if (mode == 0)
                    {
                        terminated = true;
                        return false;
                    }
                    else if (mode == 1)
                    {
                        // scattering branch
                        // stop if max depth is reached
                        if (bounce++ >= max_depth)
                        {
                            terminated = true;
                            return false;
                        }

                        // update throughput and r_u
                        float pdf = T_maj[0] * mp.sigma_s[0];
                        throughput *= T_maj * mp.sigma_s / pdf;
                        r_u *= T_maj * mp.sigma_s / pdf;

                        if (throughput.length() > epsilon && r_u.length() > epsilon)
                        {

                            // direct light sampling
                            MediumRecord mrec(p, -ray.direction.normalize(),
                                              ray.current_medium, mp.phase_function);
                            std::shared_ptr<MediumRecord> m_intr = std::make_shared<MediumRecord>(mrec);
                            // accum_L += throughput * nee_estimate(p, -ray.direction.normalize(), max_depth, ray.current_medium, prims, lights, bvh);
                            accum_L += nee_light_sample(-ray.direction.normalize(), m_intr, nullptr, throughput, r_u, prims, lights, bvh);
                            PhaseRecord phase_rec;
                            bool is_sampled = mp.phase_function->sample_p(
                                -ray.direction.normalize(), phase_rec, u_phase);

                            // sample phase function

                            if (!is_sampled || phase_rec.pdf == 0.f)
                            {
                                terminated = true;
                            }
                            else
                            {
                                // update the states
                                throughput *= phase_rec.p / phase_rec.pdf;
                                r_l = r_u / phase_rec.pdf;
                                scattered = true;
                                prev_context.p = p;
                                ray.origin = p;
                                ray.setDirection(phase_rec.wi);
                                specular_bounce = false;
                                any_non_specular = true;
                            }
                        }
                        return false;
                    }
                    else if (mode == 2)
                    {
                        vec3 sigma_n = clamp(sigma_maj - mp.sigma_t(), 0.f, FLT_MAX);
                        float pdf = T_maj[0] * sigma_n[0];
                        if (pdf <= epsilon)
                        {
                            throughput = vec3(0.f);
                            return false;
                        }

                        throughput *= T_maj * sigma_n / pdf;
                        r_u *= T_maj * sigma_n / pdf;
                        r_l *= T_maj * sigma_maj / pdf;
                        return r_u.length() > epsilon;
                    }
                    return false;
                });

            if (terminated || throughput.length() < epsilon || r_u.length() < epsilon)
            {
                return accum_L;
            }

            if (scattered)
                continue;

            throughput *= T_maj / T_maj[0];
            r_u *= T_maj / T_maj[0];
            r_l *= T_maj / T_maj[0];
        }

        if (!is_hit)
        {
            auto env_light = lights.getEnvironmentLight();

            HitRecord env_hit_record;
            env_hit_record.normal = -ray.getDirection().normalize();
            vec3 env_color = bg_color;
            if (env_light)
                env_color =
                    env_light->L_emit(env_hit_record, ray.getDirection().normalize());

            if (bounce == 0 || specular_bounce)
            {
                accum_L += throughput * env_color / r_u.average();
            }
            else
            {
                float lightPDF = 1.f / (4.f * M_PI);
                r_l *= lightPDF;
                accum_L += throughput * env_color / (r_u + r_l).average();
            }

            break;
        }

        auto mat = hit_record.material;
        if (mat && mat->is_emitter())
        {
            if (bounce == 0 || specular_bounce)
            {
                accum_L += throughput * mat->emit(ray, hit_record) / r_u.average();
            }
            else
            {
                float lightPDF = 1.f / (lights.size_excluding_env() * hit_record.surface_area);
                r_l *= lightPDF;
                accum_L += throughput * mat->emit(ray, hit_record) / (r_u + r_l).average();
            }
        }

        if (!mat)
        {
            // hit but no material, medium transition
            const auto &mi = hit_record.medium_interface;
            if (mi && mi->is_transition())
            {
                ray.current_medium = hit_record.is_inside ? mi->outside : mi->inside;
            }
            else
            {
                ray.current_medium = nullptr;
            }

            float sign = hit_record.is_inside ? 1.f : -1.f;
            ray.origin = hit_record.position + hit_record.normal * sign * epsilon;
            continue;
        }

        if (bounce++ >= max_depth)
            break;

        ScatterRecord srec;
        prev_context.p = hit_record.position;
        if (mat->scatter(ray, hit_record, srec, uc, u, MODE))
        {
            std::shared_ptr<HitRecord> hit_record_ptr = std::make_shared<HitRecord>(hit_record);
            if (!srec.is_specular())
                // accum_L+=throughput * nee_estimate(hit_record.position, -ray.direction.normalize(), max_depth, ray.current_medium, prims, lights, bvh);
                accum_L += nee_light_sample(-ray.direction.normalize(), nullptr, hit_record_ptr, throughput, r_u, prims, lights, bvh);

            if (srec.is_specular())
            {
                throughput *= srec.attenuation;
                r_l = r_u;
            }
            else
            {
                // non-delta surfaces
                if (srec.pdf_val > 0.f && srec.attenuation.length() >
                                              epsilon)
                {
                    float abs_cos_theta =
                        absDot(srec.sampled_ray.direction.normalize(),
                               hit_record.normal);
                    throughput *=
                        srec.attenuation * abs_cos_theta / srec.pdf_val;
                    r_l = r_u / srec.pdf_val;
                }
                else
                    break;
            }

            specular_bounce = srec.is_specular();
            any_non_specular |= !srec.is_specular();

            float sign = 1.0f;
            if (srec.is_transmission())
                sign = -1.0f;

            // update ray during surface interaction,avoid self - intersection
            ray.origin = hit_record.position +
                         hit_record.normal * epsilon * sign;
            ray.setDirection(srec.sampled_ray.direction);

            // update medium for transmission
            if (srec.is_transmission())
            {
                const auto &mi = hit_record.medium_interface;
                if (mi && mi->is_transition())
                {
                    ray.current_medium = hit_record.is_inside ? mi->outside : mi->inside;
                }
            }
        }
        else
        {
            break;
        }
    }

    return accum_L;
};

// inline gl::vec3 getRayColor(Ray &ray, const ObjectList &prims,
//                             gl::vec3 bg_color, const LightList &lights,
//                             uint max_depth = 40,
//                             std::shared_ptr<BVHNode> bvh = nullptr, const
//                             std::shared_ptr<Medium> &global_medium = nullptr)
// {
//     using namespace gl;

//     ray.current_medium = global_medium;
//     vec3 accum_L(0.f);
//     vec3 throughput(1.f);
//     // vec3 r_u(1.f);
//     // vec3 r_l(1.f);
//     int bounce = 0;

//     while (true)
//     {
//         float uc = halton_sampler.get1D();
//         vec2 u = halton_sampler.get2D();
//         float u_mode = halton_sampler.get1D();
//         float u_maj = halton_sampler.get1D();
//         vec2 u_phase = halton_sampler.get2D();

//         HitRecord hit_record;
//         bool is_hit = false;
//         if (bvh == nullptr)
//             is_hit = prims.intersect(ray, hit_record);
//         else
//             is_hit = bvh->intersect(ray, hit_record);

//         bool scattered = false, terminated = false;
//         // start medium scattering sampling
//         if (ray.current_medium)
//         {
//             float t_max = is_hit ? hit_record.t : FLT_MAX;

//             sampleT_maj(ray, t_max, u_maj, [&](vec3 p, MediumProperties mp,
//             vec3 sigma_maj, vec3 T_maj) -> bool
//                         {
//                             float p_absorb = mp.sigma_a[0] / mp.sigma_t()[0];
//                             float p_scatter = mp.sigma_s[0] /
//                             mp.sigma_t()[0]; float p_null = std::max(0.f, 1.f
//                             - p_absorb - p_scatter);

//                             int mode = sampleDiscrete({p_absorb, p_scatter,
//                             p_null}, u_mode, nullptr, nullptr); if (mode ==
//                             0)
//                             {
//                                 accum_L += throughput * mp.Le;
//                                 terminated = true;
//                                 return false;
//                             }
//                             else if (mode == 1)
//                             {
//                                 if(bounce++>=max_depth)
//                                 {
//                                     terminated = true;
//                                     return false;
//                                 }
//                                 //sample phase function

//                                 PhaseRecord phase_rec;
//                                 bool is_sampled =
//                                 mp.phase_function->sample_p(-ray.direction.normalize(),
//                                 phase_rec, u_phase); if(!is_sampled)
//                                 {
//                                     terminated = true;
//                                     return false;
//                                 }
//                                 //update the states

//                                 throughput *= phase_rec.p / phase_rec.pdf;
//                                 ray.origin = p;
//                                 ray.direction = phase_rec.wi;
//                                 scattered = true;
//                                 return false;
//                             }
//                             else if (mode == 2)
//                             {
//                                 u_mode = rand_num();
//                                 return true;
//                             }
//                             return false; });
//         }

//         if (terminated)
//             return accum_L;
//         if (scattered)
//             continue;

//         if (is_hit)
//         {
//             if (hit_record.material)
//             {
//                 accum_L += throughput * hit_record.material->emit(ray,
//                 hit_record);
//                 // surface interaction branch

//                 if (bounce++ >= max_depth)
//                     break;

//                 ScatterRecord srec;
//                 auto mat = hit_record.material;
//                 if (mat->scatter(ray, hit_record, srec, uc, u, MODE))
//                 {

//                     // delta handling
//                     if (srec.is_specular())
//                     {
//                         throughput *= srec.attenuation;
//                     }
//                     else
//                     {
//                         // non-delta surfaces
//                         if (srec.pdf_val > 0.f && srec.attenuation.length() >
//                         epsilon)
//                         {
//                             float abs_cos_theta =
//                             absDot(srec.sampled_ray.direction.normalize(),
//                             hit_record.normal); throughput *=
//                             srec.attenuation * abs_cos_theta / srec.pdf_val;
//                         }
//                         else
//                             break;
//                     }

//                     if (throughput.length() < epsilon)
//                         break;

//                     float sign = 1.0f;
//                     if (srec.is_transmission())
//                         sign = -1.0f;

//                     // update ray during surface interaction,avoid
//                     self-intersection ray.origin = hit_record.position +
//                     hit_record.normal * epsilon * sign; ray.direction =
//                     srec.sampled_ray.direction;

//                     // update medium for transmission
//                     if (srec.is_transmission())
//                     {
//                         const auto &mi = hit_record.medium_interface;
//                         if (mi && mi->is_transition())
//                         {
//                             ray.current_medium = hit_record.is_inside ?
//                             mi->outside : mi->inside;
//                         }
//                     }
//                 }
//                 else
//                 {
//                     // material exist but nothing happens with sample_bsdf,
//                     pure absorption break;
//                 }
//             }
//             else
//             {
//                 // hit but no material, medium transition
//                 const auto &mi = hit_record.medium_interface;
//                 if (mi && mi->is_transition())
//                 {
//                     ray.current_medium = hit_record.is_inside ? mi->outside :
//                     mi->inside;
//                 }
//                 else
//                 {
//                     ray.current_medium = nullptr;
//                 }

//                 float sign = hit_record.is_inside ? 1.f : -1.f;
//                 ray.origin = hit_record.position + hit_record.normal * sign *
//                 epsilon;
//             }
//         }
//         else
//         {
//             auto env_light = lights.getEnvironmentLight();
//             if (env_light)
//             {
//                 HitRecord env_hit_record;
//                 env_hit_record.normal = -ray.getDirection().normalize();
//                 vec3 env_color = env_light->L_emit(env_hit_record,
//                 ray.getDirection().normalize()); accum_L += throughput *
//                 env_color;
//             }
//             else
//             {
//                 // if no HDRI, use bg_color
//                 accum_L += throughput * bg_color;
//             }
//             return accum_L;
//         }
//     }

//     return accum_L;
// };

// inline gl::vec3 getRayColor(Ray &ray, const ObjectList &prims,
//                             gl::vec3 bg_color, const LightList &lights,
//                             uint max_depth = 40,
//                             std::shared_ptr<BVHNode> bvh = nullptr, const
//                             std::shared_ptr<Medium> &global_medium = nullptr)
// {
//     using namespace gl;

//     if (!global_medium)
//         return vec3(0.f);

//     ray.current_medium = global_medium;

//     vec3 accum_color(0.f);
//     vec3 throughput(1.f);

//     for (int bounce = 0; bounce < max_depth; ++bounce)
//     {
//         HitRecord surface_rec;
//         bool intersected_surface = false;
//         float t_surface = FLT_MAX;

//         // 1. Find distance to nearest surface along the current ray
//         if (bvh)
//             intersected_surface = bvh->intersect(ray, surface_rec, 0.001f,
//             FLT_MAX);
//         else
//             intersected_surface = prims.intersect(ray, surface_rec, 0.001f,
//             FLT_MAX);

//         if (intersected_surface)
//             t_surface = surface_rec.t;

//         float t_medium_interaction = FLT_MAX;
//         bool sampled_medium_event = false;

//         float trans_pdf = 1.f;
//         vec3 transmittance = 1.f;
//         if (ray.current_medium)
//         {
//             MediumProperties medium_properties =
//             ray.current_medium->sample_properties_at(ray.origin); float
//             curr_sigma_t = maxComponent(medium_properties.sigma_t());

//             t_medium_interaction = -log(1 - rand_num()) / curr_sigma_t;

//             // medium event occurs before surface interaction
//             if (t_medium_interaction < t_surface)
//             {
//                 sampled_medium_event = true;
//                 trans_pdf = exp(-curr_sigma_t * t_medium_interaction) *
//                 curr_sigma_t; transmittance = exp(-curr_sigma_t *
//                 t_medium_interaction);
//             }
//         }

//         float t_event = sampled_medium_event ? t_medium_interaction :
//         t_surface; throughput *= transmittance / trans_pdf; ray.origin =
//         ray.at(t_event);

//         if (throughput.length() < epsilon)
//             break;

//         // process event
//         if (sampled_medium_event)
//         {
//             MediumProperties medium_properties =
//             ray.current_medium->sample_properties_at(ray.origin); vec3
//             sigma_s = medium_properties.sigma_s; vec3 sigma_t =
//             medium_properties.sigma_t(); vec3 sigma_a =
//             medium_properties.sigma_a;

//             const auto &phase_function = medium_properties.phase_function;
//             if (!phase_function)
//                 throughput = vec3(0.f);

//             PhaseRecord phase_rec;
//             vec2 u = vec2(rand_num(), rand_num());
//             const auto &wo_world = -ray.direction.normalize();
//             bool phase_sampled = phase_function->sample_p(wo_world,
//             phase_rec, u);

//             if (!phase_sampled)
//                 throughput = vec3(0.f);

//             throughput *= phase_rec.p / phase_rec.pdf * sigma_s;
//             ray.direction = phase_rec.wi.normalize();
//         }
//         else
//         {
//             // hit surface branch
//             // no hit
//             if (!intersected_surface)
//             {
//                 accum_color += throughput * bg_color;
//                 break;
//             }
//             else
//             {
//                 if (surface_rec.material)
//                 {
//                     accum_color += throughput *
//                     surface_rec.material->emit(ray, surface_rec); break;
//                 }

//                 const auto &mi = surface_rec.medium_interface;
//                 // handle medium transition
//                 if (mi && mi->is_transition())
//                 {
//                     ray.current_medium = surface_rec.is_inside ? mi->outside
//                     : mi->inside; if (surface_rec.material)
//                         break;
//                 }
//                 else
//                 {
//                     ray.current_medium = nullptr;
//                 }
//             }
//         }
//     }

//     return accum_color;
// }

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
//         for (int i = 0; i < LIGHT_SAMPLE_NUM; i++) // Loop for Number of
//         Direct Light Samples
//         {
//             auto light = lights.uniform_get(); // Uniformly pick one light
//             from the list if (!light)
//                 continue;

//             auto p_prime = light->get_sample(rand_num(), rand_num());
//             auto n_prime = light->get_normal_at(p_prime);
//             auto dir_prime_to_scatter = normalize(p_scatter - p_prime);
//             auto distance_prime_to_scatter = (p_scatter - p_prime).length();

//             Ray shadow_ray(p_prime, dir_prime_to_scatter);
//             HitRecord shadow_hit_record;
//             bool is_shadow_hit = false;
//             is_shadow_hit = bvh ? bvh->intersect(shadow_ray,
//             shadow_hit_record, 0.001f, distance_prime_to_scatter - 0.001f)
//                                 : prims.intersect(shadow_ray,
//                                 shadow_hit_record, 0.001f,
//                                 distance_prime_to_scatter - 0.001f);
//             auto V = is_shadow_hit ? 0.0f : 1.0f;

//             if (V > 0.0f)
//             {
//                 HitRecord light_surface_hit; // HitRecord on the light's
//                 surface for L_emit light_surface_hit.position = p_prime;
//                 light_surface_hit.normal = n_prime;
//                 // CRITICAL ASSUMPTION: uniform light color
//                 light_surface_hit.texCoords = vec2(0.5f, 0.5f);
//                 auto Le_prime = light->L_emit(light_surface_hit,
//                 dir_prime_to_scatter); auto pdf_light = 1.f /
//                 (light->get_area() * lights.size()); auto cos_theta =
//                 absDot(n_prime, dir_prime_to_scatter);
//                 // p is defined as both direction pointing outwards
//                 auto phase_val = medium_props.phase_function->p(wo_world,
//                 -dir_prime_to_scatter); auto transmittance =
//                 exp(-medium_props.sigma_t() * distance_prime_to_scatter);

//                 if (distance_prime_to_scatter < gl::epsilon)
//                     continue;
//                 L_scatter1_accum += (Le_prime * cos_theta * phase_val *
//                 transmittance) / pdf_light /
//                 square(distance_prime_to_scatter);
//             }
//         }

//         L_scatter1_accum /= LIGHT_SAMPLE_NUM;
//     }

//     return L_scatter1_accum;
// }

// inline gl::vec3 getRayColor(const Ray &ray, const ObjectList &prims,
//                             gl::vec3 bg_color, const LightList &lights,
//                             uint max_depth = 40,
//                             std::shared_ptr<BVHNode> bvh = nullptr, const
//                             std::shared_ptr<Medium> &global_medium = nullptr)
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

//     MediumProperties medium_properties =
//     global_medium->sample_properties_at(ray.getOrigin()); float u =
//     halton_sampler.get1D(); float sigma_t =
//     maxComponent(medium_properties.sigma_t()); float t = -log(1 - u) /
//     sigma_t;

//     if (t < hit_record.t)
//     {

//         // float trans_pdf = exp(-sigma_t * t) * sigma_t;
//         // float transmittance = exp(-sigma_t * t);
//         vec3 p = ray.at(t);
//         vec3 L_s1_estimate = estimate_l_scatter1(p,
//         -ray.getDirection().normalize(), global_medium, prims, lights, bvh);
//         vec3 albedo = medium_properties.sigma_s /
//         medium_properties.sigma_t();

//         return albedo * L_s1_estimate;
//     }
//     // Surface interaction occurs first (or at the same distance, or ray
//     escapes to infinity if no hit) else
//     {
//         // float trans_pdf = exp(-sigma_t * hit_record.t);
//         // vec3 transmittance = exp(-sigma_t * hit_record.t);
//         vec3 Le(0.f);

//         if (hit_record.material != nullptr &&
//         hit_record.material->is_emitter())
//             Le = hit_record.material->emit(ray, hit_record);

//         return Le;
//     }
// }

// inline gl::vec3 getRayColor(const Ray &ray, const ObjectList &prims,
//                             gl::vec3 bg_color, const LightList &lights,
//                             uint max_depth = 40,
//                             std::shared_ptr<BVHNode> bvh = nullptr, const
//                             std::shared_ptr<Medium> &global_medium = nullptr)
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
//         MediumProperties medium_properties =
//         global_medium->sample_properties_at(ray.getOrigin()); gl::vec3
//         sigma_a = medium_properties.sigma_a;

//         transmittance = exp(-sigma_a * std::max(0.f, hit_record.t));
//     }

//     gl::vec3 Le(0.f);
//     if (hit_record.material->is_emitter())
//         Le = hit_record.material->emit(ray, hit_record);

//     return transmittance * Le;
// }
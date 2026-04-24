#pragma once
#include "base/lightList.hpp"
#include "base/objectList.hpp"
#include "config.hpp"
#include "primitives/bvh.hpp"
#include "PDF/hittablePDF.hpp"
#include "PDF/mixedPDF.hpp"
#include "sampler/sampler.hpp"
inline gl::vec3 getRayColor(const Ray &ray, const ObjectList &prims,
                            gl::vec3 bg_color, const LightList &lights,
                            uint max_depth = 40,
                            std::shared_ptr<BVHNode> bvh = nullptr)
{
  using namespace gl;

  if (max_depth == 0)
    return vec3(0.f);

  HitRecord hit_record;
  bool is_hit = false;
  if (bvh == nullptr)
    is_hit = prims.intersect(ray, hit_record);
  else
    is_hit = bvh->intersect(ray, hit_record);

  if (!is_hit)
  {
    auto env_light = lights.getEnvironmentLight();
    if (env_light)
    {
      HitRecord env_hit_record;
      env_hit_record.normal = -ray.getDirection().normalize();
      return env_light->L_emit(env_hit_record, ray.getDirection().normalize());
    }
    else
      return bg_color;
  }

  ScatterRecord srec;
  auto mat = hit_record.material;
  float uc = halton_sampler.get1D();
  vec2 u = halton_sampler.get2D();

  if (!mat->scatter(ray, hit_record, srec, uc, u, MODE))
    return mat->emit(ray, hit_record);

  // ray splitting, if it has both specular
  bool hasRefl = srec.is_specular_reflection();
  bool hasTran = srec.is_specular_transmission();

  if (hasRefl && hasTran)
  {
    float R = srec.pdf_val;
    float T = 1.0f - R;
    vec3 wo = -ray.getDirection().normalize();
    vec3 n = hit_record.normal;
    vec3 sum(0.0f);

    // branch 0 = reflect, 1 = transmit
    for (int b = 0; b < 2; ++b)
    {
      bool doRefl = (b == 0);
      float w = doRefl ? R : T;
      vec3 wi = doRefl ? pbrt::reflect(wo, n) : -wo;
      Ray next = Ray(hit_record.position, wi);
      sum +=
          w * getRayColor(next, prims, bg_color, lights, max_depth - 1, bvh);
    }

    return mat->emit(ray, hit_record) + sum;
  }

  if (hasRefl || hasTran)
  {
    return mat->emit(ray, hit_record) +
           srec.attenuation * getRayColor(srec.sampled_ray, prims, bg_color,
                                          lights, max_depth - 1, bvh);
  }

  auto pdf_ptr = srec.pdf_ptr
                     ? srec.pdf_ptr
                     : std::make_shared<CosinePDF>(hit_record.normal);

  auto wo_world = -ray.getDirection().normalize();
  auto wi_world = srec.sampled_ray.getDirection().normalize();
  float cos_theta = absDot(hit_record.normal, wi_world);
  auto pdf_val = srec.pdf_val;

  // note that below are sample from light,i.e. DI
  gl::vec3 light_term = gl::vec3(0.f);
  auto offsets = getOffsets(LIGHT_SAMPLE_X, LIGHT_SAMPLE_Y);
  if (lights.size() > 0 && LIGHT_SAMPLE_NUM > 0)
  {
    for (int i = 0; i < LIGHT_SAMPLE_NUM; i++) // Loop for Number of Direct Light Samples
    {
      auto light_sample = lights.uniform_get(); // Uniformly pick one light from the list
      if (!light_sample)
        continue;

      vec3 wi_world_dl;                // Normalized direction from hit_record.position to the light sample
      float V;                         // Visibility term (0.0 or 1.0)
      vec3 L_e_from_light;             // Radiance L_e from the light sample towards hit_record.position
      float pdf_direct_lighting;       // PDF for this direct lighting sample
      vec3 f_brdf_val;                 // BRDF value for the sampled light direction
      float light_contribution_factor; // For area lights: G. For env lights: NoI (cosine at surface).

      if (light_sample->type == LightType::ENVIRONMENT_LIGHT)
      {
        auto env_light = static_cast<const EnvironmentLight *>(light_sample.get());

        // Use offsets[i] (u,v) to sample a direction from the environment light.
        // env_light->get_sample(u,v) returns a point "at infinity" along a sampled direction.
        vec3 light_p_env = env_light->get_sample(offsets[i][0], offsets[i][1]);
        vec3 light_dir_env = light_p_env - hit_record.position; // Vector towards the "point" on env sphere
        wi_world_dl = light_dir_env.normalize();

        Ray shadow_ray_env(hit_record.position, wi_world_dl);
        HitRecord shadow_hit_record_env;
        bool is_shadow_hit_env = bvh ? bvh->intersect(shadow_ray_env, shadow_hit_record_env, 0.001f, FLT_MAX)
                                     : prims.intersect(shadow_ray_env, shadow_hit_record_env, 0.001f, FLT_MAX);
        V = is_shadow_hit_env ? 0.0f : 1.0f;

        if (V > 0.0f)
        {
          HitRecord light_hit_rec_for_emit_env;
          light_hit_rec_for_emit_env.normal = -wi_world_dl; // Conceptual normal on env sphere
          L_e_from_light = env_light->L_emit(light_hit_rec_for_emit_env, wi_world_dl);

          // PDF: P(choose env light) * P(sample wi_world_dl from env light | env light chosen)
          // P(sample wi_world_dl from env light) is a solid angle PDF.
          float pdf_solid_angle_given_env = env_light->pdf_value(hit_record.position, wi_world_dl);
          if (lights.size() == 0)
            pdf_direct_lighting = 0.f; // Should not happen if lights is not empty
          else
            pdf_direct_lighting = (1.0f / lights.size()) * pdf_solid_angle_given_env;

          light_contribution_factor = std::max(0.f, dot(hit_record.normal, wi_world_dl)); // NoI (cosine at surface)
        }
        else
        {
          L_e_from_light = vec3(0.f);
          pdf_direct_lighting = 1.0f; // Avoid div by zero; V is 0.
          light_contribution_factor = 0.f;
        }
      }

      else // Area Lights (Quad, Sphere, etc.)
      {
        vec3 light_p = light_sample->get_sample(offsets[i][0], offsets[i][1]);
        vec3 light_dir = light_p - hit_record.position; // Non-normalized vector to light point
        float light_dir_len = light_dir.length();
        light_dir.normalized(); // light_dir is now normalized (wi_world_dl)
        // Shadow Ray
        Ray shadow_ray(hit_record.position, light_dir);
        wi_world_dl = light_dir; // Store the normalized direction

        HitRecord shadow_hit_record;
        bool is_shadow_hit = false;
        if (light_dir_len < 0.001f)
        { // Shading point is on or behind the light sample point
          V = 0.0f;
        }
        else
        {
          is_shadow_hit = bvh ? bvh->intersect(shadow_ray, shadow_hit_record, 0.001f, light_dir_len - 0.001f)
                              : prims.intersect(shadow_ray, shadow_hit_record, 0.001f, light_dir_len - 0.001f);
          V = is_shadow_hit ? 0.0f : 1.0f;
        }

        if (V > 0.0f)
        {
          HitRecord light_surface_hit; // HitRecord on the light's surface for L_emit
          light_surface_hit.position = light_p;
          light_surface_hit.normal = light_sample->get_normal_at(light_p);
          // CRITICAL ASSUMPTION: offsets[i] (u,v) are the texCoords for the sampled point light_p
          light_surface_hit.texCoords = vec2(offsets[i][0], offsets[i][1]);

          // L_emit expects direction *from* the light surface, which is -wi_world_dl
          L_e_from_light = light_sample->L_emit(light_surface_hit, -wi_world_dl);

          // PDF for area lights: P(choose light k) * P(sample point x_k on light k by area)
          if (light_sample->get_area() <= 1e-7f || lights.size() == 0)
          {
            pdf_direct_lighting = 0.f; // Avoid division by zero / invalid state
          }
          else
          {
            pdf_direct_lighting = 1.f / (light_sample->get_area() * lights.size());
          }

          // Geometric term G = (NoL * NoI) / dist_sq
          float NoI = std::max(0.f, dot(hit_record.normal, wi_world_dl));         // Cosine at shading surface
          float NoL = std::max(0.f, dot(light_surface_hit.normal, -wi_world_dl)); // Cosine at light surface
          float dist_sq = light_dir_len * light_dir_len;
          if (dist_sq < 1e-7f)
          { // Avoid division by zero if light is extremely close
            light_contribution_factor = 0.0f;
          }
          else
          {
            light_contribution_factor = (NoL * NoI) / dist_sq;
          }
        }
        else
        {
          L_e_from_light = vec3(0.f);
          pdf_direct_lighting = 1.0f; // Avoid div by zero; V is 0.
          light_contribution_factor = 0.f;
        }
      }

      // Common calculation for adding contribution
      if (V > 0.0f && pdf_direct_lighting > 1e-7f)
      {
        f_brdf_val = mat->f(wo_world, wi_world_dl, hit_record, MODE);

        if (light_sample->type == LightType::ENVIRONMENT_LIGHT)
        {
          // Term: L_e * f_r * NoI / PDF_combined_solid_angle
          // light_contribution_factor is NoI for env light
          light_term += (f_brdf_val * L_e_from_light * light_contribution_factor) / pdf_direct_lighting;
        }
        else
        { // Area Lights
          // Term: L_e * f_r * G / PDF_combined_area
          // light_contribution_factor is G for area lights
          light_term += (f_brdf_val * L_e_from_light * light_contribution_factor) / pdf_direct_lighting;
        }
      }
    }
    light_term /= static_cast<float>(LIGHT_SAMPLE_NUM);
  }

  HitRecord next_hit_record;
  ScatterRecord next_scatter_record;
  bool is_next_hit = false;

  if (bvh == nullptr)
    is_next_hit =
        prims.intersect(Ray(hit_record.position, wi_world), next_hit_record);
  else
    is_next_hit =
        bvh->intersect(Ray(hit_record.position, wi_world), next_hit_record);

  // next-event estimation, avoid double counting
  if (is_next_hit && next_hit_record.material->is_emitter())
    return mat->emit(ray, hit_record) + light_term;

  auto f = srec.attenuation;
  // direct light sampling + indirect light
  return mat->emit(ray, hit_record) + light_term +
         f *
             getRayColor(Ray(hit_record.position, wi_world), prims, bg_color,
                         lights, max_depth - 1, bvh) *
             cos_theta / pdf_val;
};
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
    return bg_color;

  HitRecord hit_record;
  bool is_hit = false;
  if (bvh == nullptr)
    is_hit = prims.intersect(ray, hit_record);
  else
    is_hit = bvh->intersect(ray, hit_record);

  if (!is_hit)
    return bg_color;

  ScatterRecord srec;
  auto mat = hit_record.material;
  float uc = halton_sampler.get1D();
  vec2 u = halton_sampler.get2D();
  if (mat->scatter(ray, hit_record, srec, uc, u, MODE))
  {

    bool hasRefl = srec.is_specular_reflection();
    bool hasTran = srec.is_specular_transmission();

    if (hasRefl && hasTran)
      throw std::runtime_error("Not supported yet");

    auto pdf_ptr = srec.pdf_ptr
                       ? srec.pdf_ptr
                       : std::make_shared<CosinePDF>(hit_record.normal);

    auto wo_world = -ray.getDirection().normalize();
    auto wi_world = srec.sampled_ray.getDirection().normalize();

    auto out_ray = Ray(hit_record.position, pdf_ptr->get(uc, u).normalize());
    float cos_theta = absDot(hit_record.normal, out_ray.getDirection());
    auto pdf_val = pdf_ptr->at(out_ray.getDirection().normalize());

    // note that below are sample from light,i.e. DI
    gl::vec3 reservoir_sample = gl::vec3(0.f);
    float W = 0.0f;
    auto offsets = getOffsets(LIGHT_SAMPLE_X, LIGHT_SAMPLE_Y);
    if (lights.size() > 0)
      for (int i = 0; i < LIGHT_SAMPLE_NUM; i++)
      {
        auto light_sample = lights.uniform_get();
        auto light_p = light_sample->get_sample(offsets[i][0], offsets[i][1]);
        auto light_dir = light_p - hit_record.position;
        auto light_normal = light_sample->get_normal_at(light_p);

        Ray shadow_ray(hit_record.position, light_dir.normalize());
        HitRecord shadow_hit_record;

        bool is_shadow_hit = false;
        // note that the tmin and tmax are set to exclude the light
        if (bvh == nullptr)
          is_shadow_hit = prims.intersect(shadow_ray, shadow_hit_record, 0.001f,
                                          light_dir.length() - 0.001f);
        else
          is_shadow_hit = bvh->intersect(shadow_ray, shadow_hit_record, 0.001f,
                                         light_dir.length() - 0.001f);

        // cos wi
        auto NoI =
            std::max(dot(hit_record.normal, shadow_ray.getDirection()), 0.f);
        // cos wl
        auto NoL = std::max(dot(light_normal, -shadow_ray.getDirection()), 0.f);

        auto G = NoL * NoI / (dot(light_dir, light_dir));
        auto hit_light = false;
        auto V = is_shadow_hit ? 0.0f : 1.0f;
        auto wi_world = shadow_ray.getDirection().normalize();
        auto f = mat->f(wo_world, wi_world, hit_record, MODE);

        gl::vec3 candidate_contrib = f * light_sample->intensity *
                                     light_sample->texture->getTexelColor(offsets[i]) *
                                     light_sample->get_area() * G * V;

        float w = candidate_contrib.length();
        W += w;

        if (rand_num() < w / W)
        {
          reservoir_sample = candidate_contrib;
        }
      }

    gl::vec3 di_term = reservoir_sample * (W / LIGHT_SAMPLE_NUM);

    HitRecord next_hit_record;
    ScatterRecord next_scatter_record;
    bool is_next_hit = false;

    Ray temp_ray = out_ray;
    float temp_pdf = 0.f;
    gl::vec3 temp_albedo;

    if (bvh == nullptr)
      is_next_hit = prims.intersect(out_ray, next_hit_record);
    else
      is_next_hit = bvh->intersect(out_ray, next_hit_record);

    // next-event estimation,avoid double counting
    if (is_next_hit && next_hit_record.material->is_emitter())
      return 0.f;

    auto f = srec.attenuation;
    // direct light sampling + indirect light
    return mat->emit(ray, hit_record) + di_term +
           f *
               getRayColor(out_ray, prims, bg_color, lights, max_depth - 1,
                           bvh) *
               cos_theta / pdf_val;
  }

  return mat->emit(ray, hit_record);
};
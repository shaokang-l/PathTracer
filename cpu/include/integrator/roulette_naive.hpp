#pragma once
#include "base/lightList.hpp"
#include "base/objectList.hpp"
#include "config.hpp"
#include "primitives/bvh.hpp"
#include "PDF/hittablePDF.hpp"
#include "PDF/mixedPDF.hpp"
#include "sampler/sampler.hpp"
inline gl::vec3 getRayColor(const Ray &ray, const ObjectList &prims,
                            const ObjectList &light_objects, gl::vec3 bg_color,
                            int max_depth = 40,
                            std::shared_ptr<BVHNode> bvh = nullptr)
{
  using namespace gl;

  if (max_depth == 0)
    return 0.f;

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
    float q = clamp(maxComponent(srec.attenuation), 0.5f, 1.0f);
    if (rand_num() > q)
      return 0.f;
    srec.attenuation /= q;

    bool hasRefl = srec.is_specular_reflection();
    bool hasTran = srec.is_specular_transmission();

    if (hasRefl && hasTran)
      throw std::runtime_error("Not supported yet");

    // if it's specular, just reflect
    if (srec.is_specular())
      return srec.attenuation * getRayColor(srec.sampled_ray, prims,
                                            light_objects, bg_color,
                                            max_depth - 1, bvh);

    auto pdfs = std::vector<std::shared_ptr<PDF>>();
    for (int i = 0; i < LIGHT_SAMPLE_NUM; i++)
    {
      auto light = light_objects.uniform_get();
      auto light_pdf =
          std::make_shared<HittablePDF>(hit_record.position, light);
      pdfs.push_back(light_pdf);
    }

    if (srec.pdf_ptr != nullptr)
      pdfs.push_back(srec.pdf_ptr);
    auto mix_pdf = MixedPDF(pdfs);
    auto wi = mix_pdf.get(uc, u).normalize();
    auto out_ray = Ray(hit_record.position, wi);
    auto f = mat->f(-ray.getDirection().normalize(), wi, hit_record) / q;
    auto pdf_val = mix_pdf.at(wi);
    float cos_theta = absDot(hit_record.normal, wi);

    return mat->emit(ray, hit_record) +
           (f *
            getRayColor(out_ray, prims, light_objects, bg_color, max_depth - 1,
                        bvh) *
            cos_theta / pdf_val);
  }

  return mat->emit(ray, hit_record);
};

#pragma once
#include "material/material.hpp"
class ThinDielectric : public Material
{
private:
  float eta;
  bool use_split_ray;

public:
  ThinDielectric(float eta, bool use_split_ray = false)
      : eta(eta), use_split_ray(use_split_ray) {};

  bool
  scatter(const Ray &ray_in, HitRecord &rec, ScatterRecord &srec,
          float uc = gl::rand_num(), // coin-flip sample
          const gl::vec2 &u = {gl::rand_num(),
                               gl::rand_num()}, // 2D microfacet sample
          TransportMode mode = TransportMode::Radiance,
          BxDFReflTransFlags flags = BxDFReflTransFlags::All) const override
  {
    using namespace gl;

    vec3 wo_world = -ray_in.getDirection().normalize();
    float absCosThetaO = fabs(dot(rec.normal, wo_world));

    float R = fresnelDielectric(absCosThetaO, eta), T = 1 - R;

    if (R < 1)
    {
      R += square(T) * R / (1 - square(R));
      T = 1 - R;
    }

    float pr = R, pt = T;

    if (!(flags & BxDFReflTransFlags::Reflection))
      return pr = 0;
    if (!(flags & BxDFReflTransFlags::Transmission))
      return pt = 0;
    if (pr == 0 && pt == 0)
      return false;

    if (use_split_ray)
    {
      srec.sampled_type =
          BxDFFlags::SpecularReflection | BxDFFlags::SpecularTransmission;
      srec.pdf_val = R;
      return true;
    }

    if (uc < pr / (pr + pt))
    {
      vec3 wi_world = pbrt::reflect(wo_world, rec.normal).normalize();
      srec.sampled_ray = Ray(rec.position, wi_world);
      srec.sampled_type = BxDFFlags::SpecularReflection;
      srec.pdf_ptr = nullptr;
      srec.attenuation = 1.0f;
      srec.pdf_val = R;
    }
    else
    {
      vec3 wi_world = -wo_world.normalize();
      srec.sampled_ray = Ray(rec.position, wi_world);
      srec.sampled_type = BxDFFlags::SpecularTransmission;
      srec.pdf_ptr = nullptr;
      srec.attenuation = 1.0f;
      srec.pdf_val = T;
    }
    return true;
  };
};
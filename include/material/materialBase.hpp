#pragma once
#include "base/ray.hpp"
#include "materialFlags.hpp"
#include "PDF/pdf.hpp"
#include "utils/random.hpp"
#include "medium/mediumBase.hpp"

class Material;
extern int rejects;
// determine whether the ray is specular
struct ScatterRecord
{
  Ray sampled_ray;
  uint32_t sampled_type = BxDFFlags::All;
  float pdf_val = 0.0f;
  gl::vec3 attenuation;
  std::shared_ptr<PDF> pdf_ptr = nullptr;

  bool is_specular() const { return this->sampled_type & BxDFFlags::Specular; }
  bool is_specular_reflection() const
  {
    return (this->sampled_type & BxDFFlags::SpecularReflection) ==
           BxDFFlags::SpecularReflection;
  }
  bool is_specular_transmission() const
  {
    return (this->sampled_type & BxDFFlags::SpecularTransmission) ==
           BxDFFlags::SpecularTransmission;
  }

  bool is_transmission() const
  {
    return (this->sampled_type & BxDFFlags::Transmission) ==
           BxDFFlags::Transmission;
  }

  bool is_reflection() const
  {
    return (this->sampled_type & BxDFFlags::Reflection) ==
           BxDFFlags::Reflection;
  }
};

struct HitRecord
{
public:
  float t = FLT_MAX;
  gl::vec3 normal;
  gl::vec3 position;
  std::shared_ptr<Material> material;
  std::shared_ptr<MediumInterface> medium_interface = nullptr;
  gl::vec2 texCoords = gl::vec2(0.0f);
  gl::vec3 hair_tangent = gl::vec3(0.0f);
  // the tangent of the hair, used for hair shading

  // Ref: rt in one weeknd
  // This is used to determine whether the ray is inside or outside the object
  // As we want have the normal always point against the ray
  bool is_inside;
  void set_normal(const Ray &ray, const gl::vec3 &n)
  {
    this->is_inside = dot(ray.getDirection(), n) < 0;
    this->normal = this->is_inside ? n : -n;
  }
};

#pragma once
#include "base/primitive.hpp"
#include "material/material.hpp"
#include "utils/utility.hpp"
#include "medium/mediumBase.hpp"
class Medium
{
public:
  virtual bool is_emitter() const = 0;
  virtual MediumProperties sample_properties_at(const gl::vec3 &point) const = 0;
  virtual std::shared_ptr<RayMajorantIterator> get_ray_majorant_iterator(const Ray &ray, float tMax) = 0;
};

#pragma once
#include "./pdf.hpp"
#include "base/primitive.hpp"

// PDF towards a hittable object
class HittablePDF : public PDF {
public:
  HittablePDF(const gl::vec3 &origin, std::shared_ptr<Hittable> hittable)
      : origin(origin), hittable(hittable) {}

  virtual float at(const gl::vec3 &direction) const override {
    return hittable->pdf_value(origin, direction);
  }

  virtual gl::vec3 get(float uc = gl::rand_num(),
                       gl::vec2 u = gl::vec2(gl::rand_num(),
                                             gl::rand_num())) const override {
    return hittable->get_sample(origin);
  }

  gl::vec3 origin;
  std::shared_ptr<Hittable> hittable;
};
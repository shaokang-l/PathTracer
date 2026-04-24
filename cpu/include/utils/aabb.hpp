#pragma once
#include "./matrix.hpp"
#include "base/ray.hpp"
// class of axis-aligned bounding box
class AABB {

private:
  gl::vec3 min_xyz, max_xyz;

public:
  gl::vec3 get_min() const { return min_xyz; }
  gl::vec3 get_max() const { return max_xyz; }

  AABB() = default;
  AABB(const gl::vec3 &min_xyz, const gl::vec3 &max_xyz)
      : min_xyz(min_xyz), max_xyz(max_xyz){};
  ~AABB() = default;

  static AABB merge(AABB aabb1, AABB aabb2);

  bool intersect(const Ray &ray, float tmin, float tmax) const {

    // Ref: a faster method by Andrew Kensler at Pixar
    for (int i = 0; i < 3; i++) {
      float inv_dir = 1.0f / ray.getDirection()[i];
      float t0 = (min_xyz[i] - ray.getOrigin()[i]) * inv_dir;
      float t1 = (max_xyz[i] - ray.getOrigin()[i]) * inv_dir;

      // ensure t0 <= t1
      if (inv_dir < 0.0f)
        std::swap(t0, t1);

      // find the intersection of 3 axis
      tmin = t0 > tmin ? t0 : tmin;
      tmax = t1 < tmax ? t1 : tmax;

      if (tmax <= tmin)
        return false;
    }

    return true;
  };
};

inline AABB AABB::merge(AABB aabb1, AABB aabb2) {

  auto min_x = std::min(aabb1.get_min().x(), aabb2.get_min().x());
  auto min_y = std::min(aabb1.get_min().y(), aabb2.get_min().y());
  auto min_z = std::min(aabb1.get_min().z(), aabb2.get_min().z());
  auto max_x = std::max(aabb1.get_max().x(), aabb2.get_max().x());
  auto max_y = std::max(aabb1.get_max().y(), aabb2.get_max().y());
  auto max_z = std::max(aabb1.get_max().z(), aabb2.get_max().z());

  return AABB(gl::vec3(min_x, min_y, min_z), gl::vec3(max_x, max_y, max_z));
}
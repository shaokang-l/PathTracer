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
    float t_enter;
    return intersect(ray, tmin, tmax, t_enter);
  };

  bool intersect(const Ray &ray, float tmin, float tmax, float &t_enter) const {
    const auto &ray_origin = ray.getOrigin();
    const auto &ray_inv_direction = ray.getInvDirection();

    // Ref: a faster method by Andrew Kensler at Pixar, unrolled for this hot path.
    float tx0 = (min_xyz.x() - ray_origin.x()) * ray_inv_direction.x();
    float tx1 = (max_xyz.x() - ray_origin.x()) * ray_inv_direction.x();
    if (ray_inv_direction.x() < 0.0f)
      std::swap(tx0, tx1);
    tmin = tx0 > tmin ? tx0 : tmin;
    tmax = tx1 < tmax ? tx1 : tmax;
    if (tmax <= tmin)
      return false;

    float ty0 = (min_xyz.y() - ray_origin.y()) * ray_inv_direction.y();
    float ty1 = (max_xyz.y() - ray_origin.y()) * ray_inv_direction.y();
    if (ray_inv_direction.y() < 0.0f)
      std::swap(ty0, ty1);
    tmin = ty0 > tmin ? ty0 : tmin;
    tmax = ty1 < tmax ? ty1 : tmax;
    if (tmax <= tmin)
      return false;

    float tz0 = (min_xyz.z() - ray_origin.z()) * ray_inv_direction.z();
    float tz1 = (max_xyz.z() - ray_origin.z()) * ray_inv_direction.z();
    if (ray_inv_direction.z() < 0.0f)
      std::swap(tz0, tz1);
    tmin = tz0 > tmin ? tz0 : tmin;
    tmax = tz1 < tmax ? tz1 : tmax;
    if (tmax <= tmin)
      return false;

    t_enter = tmin;
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
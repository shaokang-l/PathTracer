#include "utils/objectTransform.hpp"

// MAXFLOAT is a POSIX/BSD/Apple-only macro. MSVC doesn't define it, so
// fall back to FLT_MAX which is the standard portable equivalent.
#include <cfloat>
#ifndef MAXFLOAT
#define MAXFLOAT FLT_MAX
#endif

template <> AABB Rotate<Axis::X>::getAABB(float t0, float t1) {
  using namespace gl;
  using namespace std;

  vec3 min(MAXFLOAT);
  vec3 max(-MAXFLOAT);

  auto box = object->getAABB(t0, t1);

  for (int i = 0; i < 2; i++) {
    for (int j = 0; j < 2; j++) {
      for (int k = 0; k < 2; k++) {
        auto x = i * box.get_max().x() + (1 - i) * box.get_min().x();
        auto y = j * box.get_max().y() + (1 - j) * box.get_min().y();
        auto z = k * box.get_max().z() + (1 - k) * box.get_min().z();

        auto new_y = cos_theta * y + sin_theta * z;
        auto new_z = -sin_theta * y + cos_theta * z;
        vec3 temp(x, new_y, new_z);

        for (int c = 0; c < 3; c++) {
          min[c] = std::min(min[c], temp[c]);
          max[c] = std::max(max[c], temp[c]);
        }
      }
    }
  }

  return AABB(min, max);
};

template <> AABB Rotate<Axis::Y>::getAABB(float t0, float t1) {
  using namespace gl;
  using namespace std;

  vec3 min(MAXFLOAT);
  vec3 max(-MAXFLOAT);

  auto box = object->getAABB(t0, t1);

  for (int i = 0; i < 2; i++) {
    for (int j = 0; j < 2; j++) {
      for (int k = 0; k < 2; k++) {
        auto x = i * box.get_max().x() + (1 - i) * box.get_min().x();
        auto y = j * box.get_max().y() + (1 - j) * box.get_min().y();
        auto z = k * box.get_max().z() + (1 - k) * box.get_min().z();

        auto new_x = cos_theta * x + sin_theta * z;
        auto new_z = -sin_theta * x + cos_theta * z;
        vec3 temp(new_x, y, new_z);

        for (int c = 0; c < 3; c++) {
          min[c] = std::min(min[c], temp[c]);
          max[c] = std::max(max[c], temp[c]);
        }
      }
    }
  }

  return AABB(min, max);
};

template <> AABB Rotate<Axis::Z>::getAABB(float t0, float t1) {
  using namespace gl;
  using namespace std;

  vec3 min(MAXFLOAT);
  vec3 max(-MAXFLOAT);

  auto box = object->getAABB(t0, t1);

  for (int i = 0; i < 2; i++) {
    for (int j = 0; j < 2; j++) {
      for (int k = 0; k < 2; k++) {
        auto x = i * box.get_max().x() + (1 - i) * box.get_min().x();
        auto y = j * box.get_max().y() + (1 - j) * box.get_min().y();
        auto z = k * box.get_max().z() + (1 - k) * box.get_min().z();

        auto new_x = cos_theta * x + sin_theta * y;
        auto new_y = -sin_theta * x + cos_theta * y;
        vec3 temp(new_x, new_y, z);

        for (int c = 0; c < 3; c++) {
          min[c] = std::min(min[c], temp[c]);
          max[c] = std::max(max[c], temp[c]);
        }
      }
    }
  }

  return AABB(min, max);
};

template <>
bool Rotate<Axis::X>::intersect(const Ray &ray, HitRecord &hit_record,
                                float tmin, float tmax) const {

  using namespace gl;
  using namespace std;
  auto ray_dir = ray.getDirection().normalize();
  auto ray_origin = ray.getOrigin();

  auto ray_origin_new = ray_origin;
  auto ray_dir_new = ray_dir;

  ray_origin_new[1] = cos_theta * ray_origin[1] - sin_theta * ray_origin[2];
  ray_origin_new[2] = sin_theta * ray_origin[1] + cos_theta * ray_origin[2];

  ray_dir_new[1] = cos_theta * ray_dir[1] - sin_theta * ray_dir[2];
  ray_dir_new[2] = sin_theta * ray_dir[1] + cos_theta * ray_dir[2];

  Ray rotated_ray(ray_origin_new, ray_dir_new.normalize());
  if (!object->intersect(rotated_ray, hit_record, tmin, tmax)) {
    return false;
  }

  auto position = hit_record.position;
  auto normal = hit_record.normal;

  position[1] =
      cos_theta * hit_record.position[1] + sin_theta * hit_record.position[2];
  position[2] =
      -sin_theta * hit_record.position[1] + cos_theta * hit_record.position[2];

  normal[1] =
      cos_theta * hit_record.normal[1] + sin_theta * hit_record.normal[2];
  normal[2] =
      -sin_theta * hit_record.normal[1] + cos_theta * hit_record.normal[2];

  hit_record.position = position;
  hit_record.set_normal(ray, normal);

  return true;
}

template <>
bool Rotate<Axis::Y>::intersect(const Ray &ray, HitRecord &hit_record,
                                float tmin, float tmax) const {

  auto ray_dir = ray.getDirection().normalize();
  auto ray_origin = ray.getOrigin();

  auto ray_origin_new = ray_origin;
  auto ray_dir_new = ray_dir;

  ray_origin_new[0] = cos_theta * ray_origin[0] - sin_theta * ray_origin[2];
  ray_origin_new[2] = sin_theta * ray_origin[0] + cos_theta * ray_origin[2];

  ray_dir_new[0] = cos_theta * ray_dir[0] - sin_theta * ray_dir[2];
  ray_dir_new[2] = sin_theta * ray_dir[0] + cos_theta * ray_dir[2];

  Ray rotated_ray(ray_origin_new, ray_dir_new.normalize());

  if (!object->intersect(rotated_ray, hit_record, tmin, tmax))
    return false;

  auto position = hit_record.position;
  auto normal = hit_record.normal;

  position[0] =
      cos_theta * hit_record.position[0] + sin_theta * hit_record.position[2];
  position[2] =
      -sin_theta * hit_record.position[0] + cos_theta * hit_record.position[2];

  normal[0] =
      cos_theta * hit_record.normal[0] + sin_theta * hit_record.normal[2];
  normal[2] =
      -sin_theta * hit_record.normal[0] + cos_theta * hit_record.normal[2];

  hit_record.position = position;
  hit_record.set_normal(ray, normal);
  return true;
}

template <>
bool Rotate<Axis::Z>::intersect(const Ray &ray, HitRecord &hit_record,
                                float tmin, float tmax) const {

  auto ray_dir = ray.getDirection().normalize();
  auto ray_origin = ray.getOrigin();

  auto ray_origin_new = ray_origin;
  auto ray_dir_new = ray_dir;

  ray_origin_new[0] = cos_theta * ray_origin[0] - sin_theta * ray_origin[1];
  ray_origin_new[1] = sin_theta * ray_origin[0] + cos_theta * ray_origin[1];

  ray_dir_new[0] = cos_theta * ray_dir[0] - sin_theta * ray_dir[1];
  ray_dir_new[1] = sin_theta * ray_dir[0] + cos_theta * ray_dir[1];

  Ray rotated_ray(ray_origin_new, ray_dir_new);
  if (!object->intersect(rotated_ray, hit_record, tmin, tmax)) {
    return false;
  }

  auto position = hit_record.position;
  auto normal = hit_record.normal;

  position[0] =
      cos_theta * hit_record.position[0] + sin_theta * hit_record.position[1];
  position[1] =
      -sin_theta * hit_record.position[0] + cos_theta * hit_record.position[1];

  normal[0] =
      cos_theta * hit_record.normal[0] + sin_theta * hit_record.normal[1];
  normal[1] =
      -sin_theta * hit_record.normal[0] + cos_theta * hit_record.normal[1];

  hit_record.position = position;
  hit_record.set_normal(ray, normal);
  return true;
}

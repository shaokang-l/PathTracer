#include "base/primitive.hpp"
namespace gl
{
  uint64_t hit_count = 0;
}

template <>
AABB AARectangle<Axis::X>::getAABB(float t0, float t1)
{
  AABB aabb(gl::vec3(this->_k - gl::epsilon, this->_d0_min, this->_d1_min),
            gl::vec3(this->_k + gl::epsilon, this->_d0_max, this->_d1_max));
  return aabb;
}

template <>
AABB AARectangle<Axis::Y>::getAABB(float t0, float t1)
{
  AABB aabb(gl::vec3(this->_d0_min, this->_k - gl::epsilon, this->_d1_min),
            gl::vec3(this->_d0_max, this->_k + gl::epsilon, this->_d1_max));
  return aabb;
}

template <>
AABB AARectangle<Axis::Z>::getAABB(float t0, float t1)
{
  AABB aabb(gl::vec3(this->_d0_min, this->_d1_min, this->_k - gl::epsilon),
            gl::vec3(this->_d0_max, this->_d1_max, this->_k + gl::epsilon));
  return aabb;
}

template <>
bool AARectangle<Axis::X>::intersect(const Ray &ray, HitRecord &hit_record,
                                     float tmin, float tmax) const
{
  if (this->intersection_mode != IntersectionMode::DEFAULT)
    throw std::runtime_error("CUSTOM intersection not supported");
  gl::hit_count++;
  auto ray_dir = ray.getDirection().normalize();
  auto ray_origin = ray.getOrigin();

  auto t = (this->_k - ray_origin.x()) / ray_dir.x();
  if (t < tmin || t > tmax)
    return false;

  auto d0 = ray_origin.y() + t * ray_dir.y();
  auto d1 = ray_origin.z() + t * ray_dir.z();
  if (d0 < this->_d0_min || d0 > this->_d0_max || d1 < this->_d1_min ||
      d1 > this->_d1_max)
    return false;

  hit_record.t = t;
  hit_record.position = ray_origin + t * ray_dir;
  hit_record.set_normal(ray, gl::vec3(1, 0, 0));
  hit_record.material = this->material;
  hit_record.texCoords =
      gl::vec2((d0 - this->_d0_min) / (this->_d0_max - this->_d0_min),
               (d1 - this->_d1_min) / (this->_d1_max - this->_d1_min));
  hit_record.medium_interface = this->medium_interface;
  hit_record.surface_area = 2 * (this->_d0_max - this->_d0_min) * (this->_d1_max - this->_d1_min);
  return true;
}

template <>
bool AARectangle<Axis::Y>::intersect(const Ray &ray, HitRecord &hit_record,
                                     float tmin, float tmax) const
{
  if (this->intersection_mode != IntersectionMode::DEFAULT)
    throw std::runtime_error("CUSTOM intersection not supported");
  gl::hit_count++;
  auto ray_dir = ray.getDirection().normalize();
  auto ray_origin = ray.getOrigin();

  auto t = (this->_k - ray_origin.y()) / ray_dir.y();
  if (t < tmin || t > tmax)
    return false;

  auto d0 = ray_origin.x() + t * ray_dir.x();
  auto d1 = ray_origin.z() + t * ray_dir.z();
  if (d0 < this->_d0_min || d0 > this->_d0_max || d1 < this->_d1_min ||
      d1 > this->_d1_max)
    return false;

  hit_record.t = t;
  hit_record.position = ray_origin + t * ray_dir;
  hit_record.set_normal(ray, gl::vec3(0, 1, 0));
  hit_record.material = this->material;
  hit_record.texCoords =
      gl::vec2((d0 - this->_d0_min) / (this->_d0_max - this->_d0_min),
               (d1 - this->_d1_min) / (this->_d1_max - this->_d1_min));
  hit_record.medium_interface = this->medium_interface;
  hit_record.surface_area = 2 * (this->_d0_max - this->_d0_min) * (this->_d1_max - this->_d1_min);
  return true;
}

template <>
bool AARectangle<Axis::Z>::intersect(const Ray &ray, HitRecord &hit_record,
                                     float tmin, float tmax) const
{
  if (this->intersection_mode != IntersectionMode::DEFAULT)
    throw std::runtime_error("CUSTOM intersection not supported");
  gl::hit_count++;
  auto ray_dir = ray.getDirection().normalize();
  auto ray_origin = ray.getOrigin();

  auto t = (this->_k - ray_origin.z()) / ray_dir.z();
  if (t < tmin || t > tmax)
    return false;

  auto d0 = ray_origin.x() + t * ray_dir.x();
  auto d1 = ray_origin.y() + t * ray_dir.y();
  if (d0 < this->_d0_min || d0 > this->_d0_max || d1 < this->_d1_min ||
      d1 > this->_d1_max)
    return false;

  hit_record.t = t;
  hit_record.position = ray_origin + t * ray_dir;
  hit_record.set_normal(ray, gl::vec3(0, 0, 1));
  hit_record.material = this->material;
  hit_record.texCoords =
      gl::vec2((d0 - this->_d0_min) / (this->_d0_max - this->_d0_min),
               (d1 - this->_d1_min) / (this->_d1_max - this->_d1_min));
  hit_record.medium_interface = this->medium_interface;
  hit_record.surface_area = 2 * (this->_d0_max - this->_d0_min) * (this->_d1_max - this->_d1_min);
  return true;
}

template <>
gl::vec3 AARectangle<Axis::X>::get_sample(const gl::vec3 &origin) const
{
  auto point = gl::vec3(this->_k, gl::C_rand(_d0_min, _d0_max),
                        gl::C_rand(_d1_min, _d1_max));
  return point - origin;
}

template <>
gl::vec3 AARectangle<Axis::Y>::get_sample(const gl::vec3 &origin) const
{
  auto point = gl::vec3(gl::C_rand(_d0_min, _d0_max), this->_k,
                        gl::C_rand(_d1_min, _d1_max));
  return point - origin;
}

template <>
gl::vec3 AARectangle<Axis::Z>::get_sample(const gl::vec3 &origin) const
{
  auto point = gl::vec3(gl::C_rand(_d0_min, _d0_max),
                        gl::C_rand(_d1_min, _d1_max), this->_k);
  return point - origin;
}

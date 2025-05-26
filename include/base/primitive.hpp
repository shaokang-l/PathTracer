#pragma once
#include "external/scene_io.hpp"
#include "material/commonMaterials.hpp"
#include "material/material.hpp"
#include "object3D.hpp"
#include "ray.hpp"
#include "utils/aabb.hpp"
#include "utils/utility.hpp"
#include "medium/mediumBase.hpp"
#include <memory>

using Materials = std::vector<std::shared_ptr<Material>>;
enum class IntersectionMode
{
  DEFAULT
};

enum class Axis
{
  X = 0,
  Y = 1,
  Z = 2
};

namespace gl
{
  extern uint64_t hit_count;
}

class Hittable : public Object3D, public std::enable_shared_from_this<Hittable>
{
public:
  virtual bool intersect(const Ray &ray, HitRecord &record, float tmin,
                         float tmax) const = 0;
  // prepare for motion blur (not required tho)
  // all objects static for now
  virtual AABB getAABB(float t0, float t1) = 0;

  virtual void setIntersectionMode(IntersectionMode mode)
  {
    this->intersection_mode = mode;
  };

  virtual float pdf_value(const gl::vec3 &origin,
                          const gl::vec3 &direction) const
  {
    return 0.f;
  }

  virtual gl::vec3 get_sample(const gl::vec3 &origin) const
  {
    return gl::vec3(1.f, 0.f, 0.f);
  }

  virtual std::shared_ptr<Material> get_material() const
  {
    return nullptr;
  }

  /**
   * @brief Returns a pointer to the underlying geometric shape, bypassing decorators.
   * Base geometric shapes (Sphere, AARectangle, TriangleMesh) will return shared_from_this().
   * Decorator classes (FlipFace, Translate, Rotate) will override this to return
   * the result of calling get_underlying_shape() on their wrapped object.
   */
  virtual std::shared_ptr<Hittable> get_underlying_shape()
  {
    return std::enable_shared_from_this<Hittable>::shared_from_this();
  }

  virtual std::shared_ptr<MediumInterface> get_medium_interface() const
  {
    return medium_interface;
  }

  ObjType objtype;
  IntersectionMode intersection_mode = IntersectionMode::DEFAULT;
  std::shared_ptr<MediumInterface> medium_interface = nullptr;
};

class Sphere : public Hittable
{
private:
  std::shared_ptr<Material> material;
  std::shared_ptr<MediumInterface> medium_interface;

public:
  float radius;
  gl::vec3 center;

  void setMaterial(std::shared_ptr<Material> material)
  {
    this->material = material;
  }

  Sphere(const gl::vec3 &center, float radius)
      : center(center), radius(radius)
  {
    this->objtype = ObjType::SPHERE_OBJ;
  };

  Sphere(const gl::vec3 &center, float radius,
         std::shared_ptr<Material> material)
      : center(center), radius(radius), material(material)
  {
    this->objtype = ObjType::SPHERE_OBJ;
  };

  Sphere(const gl::vec3 &center, float radius,
         std::shared_ptr<Material> material,
         std::shared_ptr<MediumInterface> medium_interface)
      : center(center), radius(radius), material(material), medium_interface(medium_interface)
  {
    this->objtype = ObjType::SPHERE_OBJ;
  };

  AABB getAABB(float t0, float t1) override
  {
    AABB aabb(this->center - gl::vec3(this->radius),
              this->center + gl::vec3(this->radius));
    return aabb;
  };

  bool intersect(const Ray &ray, HitRecord &hit_record, float tmin = 0.0,
                 float tmax = 10000.f) const override
  {

    if (this->intersection_mode != IntersectionMode::DEFAULT)
      throw std::runtime_error("CUSTOM intersection not supported for Curve");

    gl::hit_count++;
    auto ray_dir = ray.getDirection().normalize();
    auto ray_origin = ray.getOrigin();
    auto oc = ray_origin - this->center;
    auto a = dot(ray_dir, ray_dir);
    auto b = 2 * dot(ray_dir, oc);
    auto c = dot(oc, oc) - this->radius * this->radius;
    auto discriminant = b * b - 4 * a * c;

    if (discriminant < 0)
      return false;
    auto delta = sqrtf(discriminant);
    auto t = (-b - delta) / (2 * a);
    if (t < tmin || t > tmax)
    {
      t = (-b + delta) / (2 * a);
      if (t < tmin || t > tmax)
      {
        return false;
      }
    }

    hit_record.t = t;
    hit_record.position = ray_origin + t * ray_dir;
    hit_record.set_normal(ray,
                          (hit_record.position - this->center).normalize());
    // calculate the uv coords of a sphere
    auto p = (hit_record.position - this->center).normalize();
    auto phi = atan2(p.z(), p.x());
    auto theta = asin(p.y());
    hit_record.texCoords =
        gl::vec2(1 - (phi + M_PI) / (2 * M_PI), (theta + M_PI / 2) / M_PI);
    // remap the uv coords, so that (0,0,1) is (0,0.5)
    hit_record.texCoords.u() = fmodf(hit_record.texCoords.u() + 0.75f, 1.0f);
    hit_record.material = this->material;
    hit_record.medium_interface = this->medium_interface;

    return true;
  };

  float pdf_value(const gl::vec3 &origin, const gl::vec3 &dir) const override
  {
    HitRecord hit_record;
    if (!this->intersect(Ray(origin, dir), hit_record))
      return 0.f;

    auto cos_theta_max =
        sqrtf(1 - this->radius * this->radius /
                      gl::dot(this->center - origin, this->center - origin));
    auto angle = 2 * M_PI * (1 - cos_theta_max);

    return 1 / angle;
  };

  gl::vec3 get_sample(const gl::vec3 &origin) const override
  {
    auto direction = this->center - origin;
    auto distance_squared = gl::dot(direction, direction);
    OrthoBasis onb(direction.normalize());
    return onb.at(gl::randomToSphere(this->radius, distance_squared));
  };

  std::shared_ptr<Material> get_material() const override
  {
    return this->material;
  }

  std::shared_ptr<MediumInterface> get_medium_interface() const override
  {
    return this->medium_interface;
  }
};

template <Axis axis>
class AARectangle : public Hittable
{

private:
  std::shared_ptr<Material> material;
  std::shared_ptr<MediumInterface> medium_interface;

public:
  float _d0_min, _d0_max, _d1_min, _d1_max, _k;
  AARectangle(float k, float d0_min, float d0_max, float d1_min, float d1_max)
      : _d0_min(d0_min), _d0_max(d0_max), _d1_min(d1_min), _d1_max(d1_max),
        _k(k)
  {
    this->objtype = ObjType::RECTANGLE_OBJ;
  };

  AARectangle(float k, float d0_min, float d0_max, float d1_min, float d1_max,
              std::shared_ptr<Material> material)
      : _d0_min(d0_min), _d0_max(d0_max), _d1_min(d1_min), _d1_max(d1_max),
        _k(k), material(material)
  {
    this->objtype = ObjType::RECTANGLE_OBJ;
  };

  AARectangle(float k, float d0_min, float d0_max, float d1_min, float d1_max,
              std::shared_ptr<Material> material,
              std::shared_ptr<MediumInterface> medium_interface)
      : _d0_min(d0_min), _d0_max(d0_max), _d1_min(d1_min), _d1_max(d1_max),
        _k(k), material(material), medium_interface(medium_interface)
  {
    this->objtype = ObjType::RECTANGLE_OBJ;
  };

  AABB getAABB(float t0, float t1) override;

  bool intersect(const Ray &ray, HitRecord &hit_record, float tmin = 0.0001,
                 float tmax = 10000.f) const override;

  float pdf_value(const gl::vec3 &origin, const gl::vec3 &dir) const override
  {
    HitRecord hit_record;
    if (!this->intersect(Ray(origin, dir), hit_record))
      return 0.f;

    auto area =
        (this->_d0_max - this->_d0_min) * (this->_d1_max - this->_d1_min);
    auto distance_squared = hit_record.t * hit_record.t * gl::dot(dir, dir);
    auto cosine = std::fabs(gl::dot(dir, hit_record.normal) / dir.length());

    return distance_squared / (cosine * area);
  }

  gl::vec3 get_sample(const gl::vec3 &origin) const override;

  std::shared_ptr<Material> get_material() const override
  {
    return this->material;
  }

  std::shared_ptr<MediumInterface> get_medium_interface() const override
  {
    return this->medium_interface;
  }
};

using XYRectangle = AARectangle<Axis::Z>;
using XZRectangle = AARectangle<Axis::Y>;
using YZRectangle = AARectangle<Axis::X>;

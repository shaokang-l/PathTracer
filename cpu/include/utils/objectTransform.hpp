#pragma once
#include "base/objectList.hpp"
#include "base/primitive.hpp"

class FlipFace : public Hittable
{
public:
  FlipFace(std::shared_ptr<Hittable> object) : object(object)
  {
    this->objtype = object->objtype;
  };

  bool intersect(const Ray &ray, HitRecord &hit_record, float tmin = 0.0001,
                 float tmax = 10000.f) const override
  {
    if (!object->intersect(ray, hit_record, tmin, tmax))
    {
      return false;
    }

    hit_record.is_inside = !hit_record.is_inside;
    return true;
  };

  std::shared_ptr<Material> get_material() const override
  {

    return object ? object->get_material() : nullptr; // Current safe version
  }

  std::shared_ptr<Hittable> get_underlying_shape() override
  {
    return object ? object->get_underlying_shape() : nullptr; // Current safe version
  }

  std::shared_ptr<MediumInterface> get_medium_interface() const override
  {
    return object ? object->get_medium_interface() : nullptr; // Current safe version
  }

  AABB getAABB(float t0, float t1) override { return object->getAABB(t0, t1); };
  std::shared_ptr<Hittable> object;
};

class Translate : public Hittable
{
public:
  Translate(std::shared_ptr<Hittable> object, const gl::vec3 &offset)
      : object(object), offset(offset)
  {
    this->objtype = object->objtype;
  };

  bool intersect(const Ray &ray, HitRecord &hit_record, float tmin = 0.0001,
                 float tmax = 10000.f) const override
  {

    Ray new_ray(ray.getOrigin() - offset, ray.getDirection());
    if (!object->intersect(new_ray, hit_record, tmin, tmax))
    {
      return false;
    }

    hit_record.position += offset;
    hit_record.set_normal(new_ray, hit_record.normal);
    return true;
  };

  AABB getAABB(float t0, float t1) override
  {
    AABB box = object->getAABB(t0, t1);
    return AABB(box.get_min() + offset, box.get_max() + offset);
  };

  std::shared_ptr<Material> get_material() const override
  {

    return object ? object->get_material() : nullptr; // Current safe version
  }

  std::shared_ptr<Hittable> get_underlying_shape() override
  {
    return object ? object->get_underlying_shape() : nullptr; // Current safe version
  }

  std::shared_ptr<MediumInterface> get_medium_interface() const override
  {
    return object ? object->get_medium_interface() : nullptr; // Current safe version
  }

  std::shared_ptr<Hittable> object;
  gl::vec3 offset;
};

template <Axis axis>
class Rotate : public Hittable
{
public:
  Rotate(std::shared_ptr<Hittable> object, float angle)
      : object(object), angle(angle)
  {
    this->objtype = object->objtype;
    this->sin_theta = std::sin(angle);
    this->cos_theta = std::cos(angle);
  };

  AABB getAABB(float t0, float t1) override;
  bool intersect(const Ray &ray, HitRecord &hit_record, float tmin = 0.0001,
                 float tmax = 10000.f) const override;

  std::shared_ptr<Hittable> object;
  float angle;
  float sin_theta;
  float cos_theta;

  std::shared_ptr<Material> get_material() const override
  {

    return object ? object->get_material() : nullptr; // Current safe version
  }

  std::shared_ptr<Hittable> get_underlying_shape() override
  {
    return object ? object->get_underlying_shape() : nullptr; // Current safe version
  }

  std::shared_ptr<MediumInterface> get_medium_interface() const override
  {
    return object ? object->get_medium_interface() : nullptr; // Current safe version
  }
};

class Scale : public Hittable
{
public:
  Scale(std::shared_ptr<Hittable> obj, float s)
      : object(obj), scale(s), inv_scale(1.0f / s)
  {
    objtype = object->objtype;
  }

  bool intersect(const Ray &ray, HitRecord &rec, float tmin = 0.0001f,
                 float tmax = FLT_MAX) const override
  {

    using namespace gl;
    // 1) only shrink the origin into object-space
    vec3 o_obj = ray.getOrigin() * inv_scale;
    vec3 d_obj = ray.getDirection(); // leave it alone!

    // 2) adjust the t-range so we don't accidentally clip
    float t0 = tmin * inv_scale;
    float t1 = tmax * inv_scale;

    Ray obj_ray(o_obj, d_obj);
    if (!object->intersect(obj_ray, rec, t0, t1))
      return false;

    // 3) the child gave us rec.t in object-space;
    //    convert it back to world-space
    rec.t *= scale;
    // 4) scale the hit-point back into world-space
    rec.position *= scale;

    // 5) transform the normal by the inverse-transpose:
    //    for uniform scale that's just dividing and renormalizing
    vec3 n = rec.normal * inv_scale;
    vec3 n_world = normalize(n);

    // 6) set front/back with the original world-ray
    rec.set_normal(ray, n_world);
    return true;
  }

  AABB getAABB(float t0, float t1) override
  {
    // scale each corner of the child's AABB
    auto box = object->getAABB(t0, t1);
    return AABB(box.get_min() * scale, box.get_max() * scale);
  }

  std::shared_ptr<Material> get_material() const override
  {

    return object ? object->get_material() : nullptr; // Current safe version
  }

  std::shared_ptr<Hittable> get_underlying_shape() override
  {
    return object ? object->get_underlying_shape() : nullptr; // Current safe version
  }

  std::shared_ptr<MediumInterface> get_medium_interface() const override
  {
    return object ? object->get_medium_interface() : nullptr; // Current safe version
  }

private:
  std::shared_ptr<Hittable> object;
  float scale, inv_scale;
};

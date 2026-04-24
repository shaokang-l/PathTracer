#pragma once
#include "base/camera.hpp"
#include "light/light.hpp"
#include "base/lightList.hpp"
#include "base/primitive.hpp"
#include "utils/aabb.hpp"
#include "utils/utility.hpp"
// a manager for hittable objects
class ObjectList : public Hittable
{
public:
  ObjectList() = default;
  ~ObjectList() = default;

  ObjectList(const LightList &lights)
  {
    for (const auto &light : lights.get())
    {
      if (light->type == LightType::QUAD_LIGHT)
      {

        auto quad_light = std::dynamic_pointer_cast<QuadLight>(light);
        auto vertices = quad_light->vertices;
        // get min and max xyz
        gl::vec3 min = vertices[0];
        gl::vec3 max = vertices[0];
        for (int i = 1; i < 4; i++)
        {
          min = gl::vec3(std::min(min.x(), vertices[i].x()),
                         std::min(min.y(), vertices[i].y()),
                         std::min(min.z(), vertices[i].z()));
          max = gl::vec3(std::max(max.x(), vertices[i].x()),
                         std::max(max.y(), vertices[i].y()),
                         std::max(max.z(), vertices[i].z()));
        }

        // determin which AARect it is
        if (std::abs(min.x() - max.x()) < 0.001f)
        {
          this->addObject(std::make_shared<YZRectangle>(
              min.x(), min.y(), max.y(), min.z(), max.z()));
        }
        else if (std::abs(min.y() - max.y()) < 0.001f)
        {
          this->addObject(std::make_shared<XZRectangle>(
              min.y(), min.x(), max.x(), min.z(), max.z()));
        }
        else if (std::abs(min.z() - max.z()))
        {
          this->addObject(std::make_shared<XYRectangle>(
              min.z(), min.x(), max.x(), min.y(), max.y()));
        }
        else
        {
          std::cout << "Quad light is not axis aligned" << std::endl;
        }
      }
      else if (light->type == LightType::SPHERE_LIGHT)
      {
        auto sphere_light = std::dynamic_pointer_cast<SphereLight>(light);
        this->addObject(std::make_shared<Sphere>(sphere_light->center,
                                                 sphere_light->radius));
      }
      else
      {
        std::cout << "Light type not supported" << std::endl;
      }
    }
  };

  AABB getAABB(float t0, float t1) override;

  void addObject(std::shared_ptr<Hittable> object)
  {
    this->objects.push_back(object);
  };

  std::vector<std::shared_ptr<Hittable>> getLists() const
  {
    return this->objects;
  };

  // this determins the closest object that the ray hits
  bool intersect(const Ray &ray, HitRecord &hit_record, float tmin = 0.0001,
                 float tmax = 10000.f) const override
  {

    float closest_point = tmax;
    bool any_hit = false;
    HitRecord temp_hit_record;

    for (auto &object : this->objects)
    {
      bool is_hit =
          object->intersect(ray, temp_hit_record, tmin, closest_point);
      if (is_hit)
      {
        closest_point = temp_hit_record.t;
        hit_record = temp_hit_record;
        any_hit = true;
      }
    }

    return any_hit;
  }

  // this determins the list of hit object, used for handling transparent
  // objects shadow ray
  std::vector<HitRecord> hit_list(const Ray &ray, float tmin = 0.01f,
                                  float tmax = 10000.f) const
  {
    std::vector<HitRecord> hit_list;
    for (auto &object : this->objects)
    {
      HitRecord hit_record;
      bool is_hit = object->intersect(ray, hit_record, tmin, tmax);
      // if there is a hit
      if (is_hit)
      {
        hit_list.push_back(hit_record);
      }
    }

    // sort the hit list by t
    std::sort(hit_list.begin(), hit_list.end(),
              [](const auto &a, const auto &b)
              { return a.t < b.t; });

    return hit_list;
  }

  std::shared_ptr<Hittable> operator[](int index) const
  {
    return this->objects[index];
  };

  std::shared_ptr<Hittable> uniform_get() const
  {
    int index = (int)(gl::rand_num() * objects.size());
    return this->objects[index % objects.size()];
  };

  void clear()
  {
    this->objects.clear();
  }

private:
  std::vector<std::shared_ptr<Hittable>> objects;
};

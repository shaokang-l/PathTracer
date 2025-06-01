#pragma once
#include "base/object3D.hpp"
#include "base/primitive.hpp"
#include "external/scene_io.hpp"
#include "utils/utility.hpp"
#include <iostream>
#include <memory>
#include <variant>

struct PolyLightInfo
{
  std::vector<gl::vec3> vertices;
  gl::vec3 color;
  float intensity;
};

struct LightSampleContext
{
  gl::vec3 p;
  gl::vec3 n;
};

class Light : public Object3D
{
public:
  Light()
  {
    position = gl::vec3(0.f, 0.f, 0.0f);
    texture = std::make_shared<ConstantTexture>(gl::vec3(1.0f, 1.0f, 1.0f));
    intensity = 1.0f;
  };

  Light(const gl::vec3 &position, const ColorVariant &color_var, float intensity)
  {
    this->position = position;
    this->texture = gl::texture::to_texture2d(color_var);
    this->intensity = intensity;
  }

  virtual gl::vec3 uniform_sample() const = 0;
  virtual gl::vec3 get_sample(float u, float v) const = 0;
  virtual float get_area() const { return 1.f; }
  virtual gl::vec3 get_normal_at(const gl::vec3 &p) const = 0;
  virtual float pdf_value(const gl::vec3 &origin, const gl::vec3 &dir) const
  {
    return 0.f;
  }

  // --- NEW/UPDATED: Emitted Radiance ---
  // Returns the radiance Le emitted from a point on the light (light_hit_rec.position, .normal, .texCoords)
  // in the direction w_from_light_normalized (world space, normalized, pointing away from the light surface).
  virtual gl::vec3 L_emit(const HitRecord &light_hit_rec, const gl::vec3 &w_from_light_normalized) const = 0;

  ~Light() = default;

  float intensity = 1.0f;
  std::shared_ptr<Texture2D> texture = nullptr;
  LightType type = LightType::POINT_LIGHT;
};

class QuadLight : public Light
{
public:
  std::array<gl::vec3, 4> vertices;
  QuadLight(const PolyLightInfo &info)
  {
    this->type = LightType::QUAD_LIGHT;
    assert(info.vertices.size() == 4);
    for (int i = 0; i < 4; i++)
    {
      this->vertices[i] = info.vertices[i];
    }
    this->texture = std::make_shared<ConstantTexture>(info.color);
    this->intensity = info.intensity;
  }

  QuadLight(std::shared_ptr<AARectangle<Axis::X>> quad, const ColorVariant &color_var,
            float intensity)
  {
    this->type = LightType::QUAD_LIGHT;
    gl::vec3 v0 = {quad->_k, quad->_d0_min, quad->_d1_min};
    gl::vec3 v1 = {quad->_k, quad->_d0_max, quad->_d1_min};
    gl::vec3 v2 = {quad->_k, quad->_d0_max, quad->_d1_max};
    gl::vec3 v3 = {quad->_k, quad->_d0_min, quad->_d1_max};
    this->vertices = {v0, v1, v2, v3};
    this->texture = gl::texture::to_texture2d(color_var);
    this->intensity = intensity;
  }

  QuadLight(std::shared_ptr<AARectangle<Axis::Y>> quad, const ColorVariant &color_var,
            float intensity)
  {
    this->type = LightType::QUAD_LIGHT;
    gl::vec3 v0 = {quad->_d0_min, quad->_k, quad->_d1_min};
    gl::vec3 v1 = {quad->_d0_max, quad->_k, quad->_d1_min};
    gl::vec3 v2 = {quad->_d0_max, quad->_k, quad->_d1_max};
    gl::vec3 v3 = {quad->_d0_min, quad->_k, quad->_d1_max};
    this->vertices = {v0, v1, v2, v3};
    this->texture = gl::texture::to_texture2d(color_var);
    this->intensity = intensity;
  }

  QuadLight(std::shared_ptr<AARectangle<Axis::Z>> quad, const ColorVariant &color_var,
            float intensity)
  {
    this->type = LightType::QUAD_LIGHT;
    gl::vec3 v0 = {quad->_d0_min, quad->_d1_min, quad->_k};
    gl::vec3 v1 = {quad->_d0_max, quad->_d1_min, quad->_k};
    gl::vec3 v2 = {quad->_d0_max, quad->_d1_max, quad->_k};
    gl::vec3 v3 = {quad->_d0_min, quad->_d1_max, quad->_k};
    this->vertices = {v0, v1, v2, v3};
    this->texture = gl::texture::to_texture2d(color_var);
    this->intensity = intensity;
  }

  QuadLight(std::array<gl::vec3, 4> vertices, const ColorVariant &color_var,
            float intensity)
  {
    this->type = LightType::QUAD_LIGHT;
    this->vertices = vertices;
    this->texture = gl::texture::to_texture2d(color_var);
    this->intensity = intensity;
  };

  // uniformly sample a point on the light
  virtual gl::vec3 uniform_sample() const override
  {
    using namespace gl;
    float u = rand_num();
    float v = rand_num();
    vec3 v1 = vertices[1] - vertices[0];
    vec3 v2 = vertices[3] - vertices[0];
    vec3 p = vertices[0] + u * v1 + v * v2;
    return p;
  };

  virtual gl::vec3 get_sample(float u, float v) const override
  {
    using namespace gl;
    vec3 v1 = vertices[1] - vertices[0];
    vec3 v2 = vertices[3] - vertices[0];
    vec3 p = vertices[0] + u * v1 + v * v2;
    return p;
  };

  virtual gl::vec3 get_normal_at(const gl::vec3 &p) const override
  {
    using namespace gl;
    vec3 v1 = vertices[1] - vertices[0];
    vec3 v2 = vertices[3] - vertices[0];
    vec3 normal = cross(v1, v2).normalize();
    return normal;
  }

  virtual float get_area() const override
  {
    using namespace gl;
    vec3 v1 = vertices[1] - vertices[0];
    vec3 v2 = vertices[3] - vertices[0];
    return cross(v1, v2).length();
  }

  gl::vec3 L_emit(const HitRecord &light_hit_rec, const gl::vec3 &w_from_light_normalized) const override
  {

    if (gl::dot(light_hit_rec.normal, w_from_light_normalized) > 0.0f)
    {
      return this->texture->getTexelColor(light_hit_rec.texCoords) * this->intensity;
    }
    return gl::vec3(0.f);
  };

  virtual float pdf_value(const gl::vec3 &origin,
                          const gl::vec3 &dir) const override
  {
    using namespace gl;
    // 1) intersect ray (origin + t*dir) with plane of the quad
    vec3 v0 = vertices[0];
    vec3 edge1 = vertices[1] - v0;
    vec3 edge2 = vertices[3] - v0;
    vec3 N = cross(edge1, edge2).normalize();
    float denom = dot(N, dir);
    if (fabs(denom) < 1e-6f)
      return 0.f; // parallel

    float t = dot(v0 - origin, N) / denom;
    if (t <= 0)
      return 0.f; // behind origin

    vec3 P = origin + dir * t;

    // 2) test if P is inside the quad via two‐triangle test
    auto inTri = [&](const vec3 &A, const vec3 &B, const vec3 &C)
    {
      // barycentric‐style edge tests
      vec3 nABC = N;
      if (dot(cross(B - A, C - A), nABC) < 0)
        return false;
      if (dot(cross(C - B, P - B), nABC) < 0)
        return false;
      if (dot(cross(A - C, P - C), nABC) < 0)
        return false;
      return true;
    };
    if (!inTri(v0, vertices[1], vertices[2]) &&
        !inTri(v0, vertices[2], vertices[3]))
      return 0.f; // outside the quad

    // 3) convert area‐pdf to solid‐angle pdf
    float area = get_area();
    float dist2 = dot(P - origin, P - origin);
    float cosθ = fabs(dot(N, -dir)); // cos of angle at P
    // uniform‐area pdf = 1/area
    // PDF_Ω = PDF_A * (r² / cosθ)
    return dist2 / (area * cosθ);
  }

  ~QuadLight() = default;
};

class SphereLight : public Light
{
public:
  gl::vec3 center;
  float radius;
  SphereLight(const gl::vec3 &center, float radius, const ColorVariant &color_var,
              float intensity)
  {
    this->type = LightType::SPHERE_LIGHT;
    this->center = center;
    this->radius = radius;
    this->texture = gl::texture::to_texture2d(color_var);
    this->intensity = intensity;
  };

  SphereLight(std::shared_ptr<Sphere> sphere, const ColorVariant &color_var,
              float intensity)
  {
    this->type = LightType::SPHERE_LIGHT;
    this->center = sphere->center;
    this->radius = sphere->radius;
    this->texture = gl::texture::to_texture2d(color_var);
    this->intensity = intensity;
  };

  virtual gl::vec3 uniform_sample() const override
  {
    using namespace gl;
    float u = rand_num();
    float v = rand_num();
    float theta = 2 * M_PI * u;
    float phi = std::acos(2 * v - 1);
    float x = std::sin(phi) * std::cos(theta);
    float y = std::sin(phi) * std::sin(theta);
    float z = std::cos(phi);
    vec3 p = center + radius * vec3(x, y, z);
    return p;
  };

  virtual gl::vec3 get_sample(float u, float v) const override
  {
    using namespace gl;
    float theta = 2 * M_PI * u;
    float phi = std::acos(2 * v - 1);
    float x = std::sin(phi) * std::cos(theta);
    float y = std::sin(phi) * std::sin(theta);
    float z = std::cos(phi);
    vec3 p = center + radius * vec3(x, y, z);
    return p;
  };

  virtual gl::vec3 get_normal_at(const gl::vec3 &p) const override
  {
    using namespace gl;
    vec3 normal = (p - center).normalize();
    return normal;
  }

  virtual float get_area() const override
  {
    using namespace gl;
    return 4 * M_PI * radius * radius;
  }

  virtual float pdf_value(const gl::vec3 &origin,
                          const gl::vec3 &dir) const override
  {
    using namespace gl;
    // Vector from shading point to sphere center
    vec3 wc = center - origin;
    float dc2 = dot(wc, wc), r2 = radius * radius;
    if (dc2 <= r2)
      return 1.f / (4.f * M_PI); // we're inside: uniform over whole sphere

    // cone half‐angle θ_max from origin
    float cosΘmax = sqrtf(1.f - r2 / dc2);
    // solid angle of visible cap
    float omega = 2.f * M_PI * (1.f - cosΘmax);
    return 1.f / omega;
  }

  gl::vec3 L_emit(const HitRecord &light_hit_rec, const gl::vec3 &w_from_light_normalized) const override
  {

    if (gl::dot(light_hit_rec.normal, w_from_light_normalized) > 0.0f)
      return this->texture->getTexelColor(light_hit_rec.texCoords) * this->intensity;

    return gl::vec3(0.f);
  }

  ~SphereLight() = default;
};

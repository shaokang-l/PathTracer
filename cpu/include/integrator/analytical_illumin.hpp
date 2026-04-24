#pragma once
#include "base/lightList.hpp"
#include "base/objectList.hpp"
#include "config.hpp"
#include "primitives/bvh.hpp"
#include "sampler/sampler.hpp"
static inline gl::vec3
getIrradianceVector(const std::vector<gl::vec3> &vertices,
                    const HitRecord &hit_record)
{
  using namespace gl;
  using namespace std;

  vec3 p = hit_record.position;
  vec3 n = hit_record.normal;

  std::vector<vec3> u_k;
  std::vector<float> theta_k;
  std::vector<vec3> gamma_k;
  for (const auto &v : vertices)
    u_k.push_back((v - p).normalize());

  for (int i = 0; i < u_k.size(); i++)
  {
    float cos_theta = dot(u_k[i], u_k[(i + 1) % u_k.size()]);
    float theta = std::acos(cos_theta);
    theta_k.push_back(theta);
  }

  for (int i = 0; i < u_k.size(); i++)
  {
    vec3 gamma = cross(u_k[i], u_k[(i + 1) % u_k.size()]).normalize();
    gamma_k.push_back(gamma);
  }

  vec3 irr_vec = vec3(0.0);
  for (int i = 0; i < u_k.size(); i++)
  {
    irr_vec += 0.5f * gamma_k[i] * theta_k[i];
  }

  return irr_vec;
};

inline gl::vec3 getRayColor(const Ray &ray, const ObjectList &prims,
                            gl::vec3 bg_color, const LightList &light_info,
                            std::shared_ptr<BVHNode> bvh = nullptr)
{
  using namespace gl;

  HitRecord hit_record;
  bool is_hit = false;
  if (bvh == nullptr)
    is_hit = prims.intersect(ray, hit_record);
  else
    is_hit = bvh->intersect(ray, hit_record);

  if (!is_hit)
    return bg_color;

  Ray out_ray;
  vec3 albedo(0.f);
  ScatterRecord srec;
  float pdf = 0.f;
  auto mat = hit_record.material;
  float uc = halton_sampler.get1D();
  vec2 u = halton_sampler.get2D();
  if (mat->scatter(ray, hit_record, srec, uc, u, MODE))
  {
    auto light = light_info.uniform_get();
    if (light->type == LightType::SPHERE_LIGHT)
      throw std::runtime_error(
          "Analytical Illumination is not supported for sphere light");

    // convert to quad light
    auto quad_light = std::dynamic_pointer_cast<QuadLight>(light);
    std::vector<gl::vec3> light_vertices(quad_light->vertices.begin(),
                                         quad_light->vertices.end());

    albedo = srec.attenuation;
    vec3 irr_vec = getIrradianceVector(light_vertices, hit_record);
    float irr_term = fabs(dot(irr_vec, hit_record.normal));

    return mat->emit(ray, hit_record) +
           albedo / M_PI * light->texture->getTexelColor(u) * light->intensity * irr_term;
  }

  return mat->emit(ray, hit_record);
};

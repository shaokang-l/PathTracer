#pragma once
#include "base/lightList.hpp"
#include "base/objectList.hpp"
#include "primitives/bvh.hpp"
// Whitted Ray Tracing
// recursive function,where shading happens
// I suppose the amount of reflection and refraction can be better handled with
// fresnel but it's not implemented here. As whitted RT uses coeffecient to
// determine the amount of reflection and refraction

// inline gl::vec3 getRayColor(const Ray &ray, const ObjectList &prims,
//                             uint max_depth, const LightList &lights) {
//   using namespace gl;

//   vec3 color(0.0f, 0.0f, 0.0f);
//   vec3 bg(0.0f, 0.0f, 0.0f);

//   if (max_depth == 0 || ray.intensity < 0.0001f)
//     return vec3(0.0f, 0.0f, 0.0f);

//   auto hit_record = prims.hit(ray);

//   // background color
//   if (hit_record == nullptr)
//     return bg;

//   auto hit_point = hit_record->position;
//   auto normal = hit_record->normal;
//   auto view_dir = -ray.getDirection();
//   auto material = hit_record->material;

//   auto local_diff = vec3(0.f);
//   auto local_spec = vec3(0.f);
//   auto local_ambient = vec3(0.f);

//   // calculate ambient and diffuse color
//   for (const auto &light : lights.get()) {
//     gl::vec3 light_dir(0.0f, 0.0f, 0.0f);

//     if (light->type == LightType::POINT_LIGHT) {
//       auto light_pos = light->position;
//       light_dir = (light_pos - hit_point).normalize();
//     }

//     else if (light->type == LightType::DIRECTIONAL_LIGHT) {
//       light_dir = static_cast<DirectionalLight *>(light.get())->defaultFront;
//       light_dir = -light_dir;
//     }

//     auto light_color = light->color;

//     Ray shadow_ray(hit_point, light_dir);
//     // Shadow ray factor
//     vec3 Si(1.0f);
//     // Get the hitlist of the shadow ray
//     auto hit_list = prims.hit_list(shadow_ray);
//     if (hit_list.size() != 0) {
//       for (const auto hit : hit_list) {
//         if (material->ktran != 0.f) {
//           // if the shadow ray hits a transparent surface, attenuate the
//           light
//           // find the largest channel of the diffuse color, divide by it
//           auto normalized_diff = material->diff_color /
//                                  std::max(std::max(material->diff_color.x(),
//                                                    material->diff_color.y()),
//                                           material->diff_color.z());
//           Si = (material->ktran) * normalized_diff * Si;
//         } else {
//           // if the shadow ray hits a non-transparent surface, just return
//           black Si *= 0.0f; break;
//         }
//       }
//     }

//     auto NoL = std::max(0.f, gl::dot(normal, light_dir));
//     auto diffuse = material->diff_color * NoL;

//     auto half_dir = (view_dir + light_dir).normalize();
//     auto NoH = std::max(0.f, gl::dot(normal, half_dir));
//     auto specular = material->spec_color * pow(NoH, material->shininess *
//     128);

//     float attenuation = 1.0f;
//     if (light->type == LightType::POINT_LIGHT)
//       attenuation = attenuate((light->position - hit_point).length());

//     local_diff += (Si * attenuation * light_color * diffuse);
//     local_spec += (Si * attenuation * light_color * specular);
//   }

//   local_ambient = material->diff_color * material->ambient_color;
//   local_ambient *= (1 - material->ktran);
//   local_diff *= (1 - material->ktran);

//   // if hits a transparent surface, generate a refraction ray
//   // possible problem with refraction
//   if (hit_record->material->ktran != 0.f) {
//     bool is_refract = false;
//     gl::vec3 out_refract_dir;
//     gl::vec3 out_refract_pos;

//     // the ray is trying to get out of the object
//     if (hit_record->is_inside)
//       out_refract_dir = gl::refract(ray.getDirection(), hit_record->normal,
//                                     1.f / 1.5f, is_refract);
//     // the ray is trying to get into the object
//     else
//       out_refract_dir = gl::refract(ray.getDirection(), hit_record->normal,
//                                     1.5f / 1.f, is_refract);

//     // if the ray is refracted, generate a refraction ray
//     if (is_refract) {
//       // this is used for avoid self-intersection
//       gl::vec3 out_refract_pos = hit_record->position - 1e-4 * normal;
//       Ray out_refract(out_refract_pos, out_refract_dir,
//                       ray.intensity * material->ktran);
//       color += material->ktran *
//                getRayColor(out_refract, prims, max_depth - 1, lights);
//     }
//     // if the ray is totally reflected, generate a reflection ray
//     else {
//       gl::vec3 out_reflect_dir =
//           gl::reflect(ray.getDirection(), hit_record->normal);
//       // this is used for avoid self-intersection
//       Ray out_reflect(hit_record->position + 1e-4 * normal, out_reflect_dir,
//                       ray.intensity);
//       color += material->spec_color.x() *
//                getRayColor(out_reflect, prims, max_depth - 1, lights);
//     }
//   }

//   // // if hits a specular surface, generate a reflection ray
//   if (hit_record->material->spec_color != gl::vec3(0.f)) {
//     gl::vec3 out_reflect_dir =
//         gl::reflect(ray.getDirection(), hit_record->normal);
//     // this is used for avoid self-intersection
//     Ray out_reflect(hit_record->position + 1e-4 * normal, out_reflect_dir,
//                     ray.intensity * material->spec_color.x());
//     color += material->spec_color.x() *
//              getRayColor(out_reflect, prims, max_depth - 1, lights);
//   }

//   return color + local_ambient + local_diff + local_spec +
//          material->emissive_color;
// }

// inline gl::vec3 getRayColor(const Ray &ray, const ObjectList &prims,
//                             const BVHNode &bvh, uint max_depth,
//                             const LightList &lights) {
//   using namespace gl;

//   vec3 color(0.0f, 0.0f, 0.0f);
//   vec3 bg(0.0f, 0.0f, 0.0f);

//   if (max_depth == 0 || ray.intensity < 0.0001f)
//     return vec3(0.0f, 0.0f, 0.0f);

//   auto hit_record = bvh.intersect(ray);

//   // background color
//   if (hit_record == nullptr)
//     return bg;

//   auto hit_point = hit_record->position;
//   auto normal = hit_record->normal;
//   auto view_dir = -ray.getDirection();
//   auto material = hit_record->material;

//   auto local_diff = vec3(0.f);
//   auto local_spec = vec3(0.f);
//   auto local_ambient = vec3(0.f);

//   // calculate ambient and diffuse color
//   for (const auto &light : lights.get()) {
//     gl::vec3 light_dir(0.0f, 0.0f, 0.0f);

//     if (light->type == LightType::POINT_LIGHT) {
//       auto light_pos = light->position;
//       light_dir = (light_pos - hit_point).normalize();
//     }

//     else if (light->type == LightType::DIRECTIONAL_LIGHT) {
//       light_dir = static_cast<DirectionalLight *>(light.get())->defaultFront;
//       light_dir = -light_dir;
//     }

//     auto light_color = light->color;

//     Ray shadow_ray(hit_point, light_dir);
//     // Shadow ray factor
//     vec3 Si(1.0f);
//     // Get the hitlist of the shadow ray
//     auto shadow_hit_record = bvh.intersect(shadow_ray);
//     if (shadow_hit_record != nullptr)
//       Si = vec3(0.0f, 0.0f, 0.0f);

//     auto NoL = std::max(0.f, gl::dot(normal, light_dir));
//     auto diffuse = material->diff_color * NoL;

//     auto half_dir = (view_dir + light_dir).normalize();
//     auto NoH = std::max(0.f, gl::dot(normal, half_dir));
//     auto specular = material->spec_color * pow(NoH, material->shininess *
//     128);

//     float attenuation = 1.0f;
//     if (light->type == LightType::POINT_LIGHT)
//       attenuation = attenuate((light->position - hit_point).length());

//     local_diff += (Si * attenuation * light_color * diffuse);
//     local_spec += (Si * attenuation * light_color * specular);
//   }

//   local_ambient = material->diff_color * material->ambient_color;
//   local_ambient *= (1 - material->ktran);
//   local_diff *= (1 - material->ktran);

//   // if hits a transparent surface, generate a refraction ray
//   // possible problem with refraction
//   if (hit_record->material->ktran != 0.f) {
//     bool is_refract = false;
//     gl::vec3 out_refract_dir;
//     gl::vec3 out_refract_pos;

//     // the ray is trying to get out of the object
//     if (hit_record->is_inside)
//       out_refract_dir = gl::refract(ray.getDirection(), hit_record->normal,
//                                     1.f / 1.5f, is_refract);
//     // the ray is trying to get into the object
//     else
//       out_refract_dir = gl::refract(ray.getDirection(), hit_record->normal,
//                                     1.5f / 1.f, is_refract);

//     // if the ray is refracted, generate a refraction ray
//     if (is_refract) {
//       // this is used for avoid self-intersection
//       gl::vec3 out_refract_pos = hit_record->position - 1e-4 * normal;
//       Ray out_refract(out_refract_pos, out_refract_dir,
//                       ray.intensity * material->ktran);
//       color += material->ktran *
//                getRayColor(out_refract, prims, bvh, max_depth - 1, lights);
//     }
//     // if the ray is totally reflected, generate a reflection ray
//     else {
//       gl::vec3 out_reflect_dir =
//           gl::reflect(ray.getDirection(), hit_record->normal);
//       // this is used for avoid self-intersection
//       Ray out_reflect(hit_record->position + 1e-4 * normal, out_reflect_dir,
//                       ray.intensity);
//       color += material->spec_color.x() *
//                getRayColor(out_reflect, prims, bvh, max_depth - 1, lights);
//     }
//   }

//   // // if hits a specular surface, generate a reflection ray
//   if (hit_record->material->spec_color != gl::vec3(0.f)) {
//     gl::vec3 out_reflect_dir =
//         gl::reflect(ray.getDirection(), hit_record->normal);
//     // this is used for avoid self-intersection
//     Ray out_reflect(hit_record->position + 1e-4 * normal, out_reflect_dir,
//                     ray.intensity * material->spec_color.x());
//     color += material->spec_color.x() *
//              getRayColor(out_reflect, prims, bvh, max_depth - 1, lights);
//   }

//   return color + local_ambient + local_diff + local_spec +
//          material->emissive_color;
// }
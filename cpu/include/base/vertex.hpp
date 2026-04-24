#pragma once
#include "material/material.hpp"
#include "utils/matrix.hpp"
#include "utils/transformations.hpp"
#include <memory>
struct Vertex {
  gl::vec3 position;
  gl::vec3 normal;
  gl::vec2 texCoords;
  std::shared_ptr<Material> material;

  Vertex(gl::vec3 position = {0.0f, 0.0f, 0.0f},
         gl::vec3 normal = {0.0f, 0.0f, 0.0f},
         gl::vec2 texCoords = {0.0f, 0.0f},
         std::shared_ptr<Material> material = nullptr) {
    this->position = position;
    this->normal = normal;
    this->texCoords = texCoords;
    this->material = material;
  };

  bool operator==(const Vertex &other) {
    return position == other.position && normal == other.normal &&
           texCoords == other.texCoords;
  }
};

static Vertex barycentric_lerp(const Vertex &v1, const Vertex &v2,
                               const Vertex &v3, gl::vec2 &&uv) {
  auto position = (1 - uv.x() - uv.y()) * v1.position + uv.x() * v2.position +
                  uv.y() * v3.position;
  auto normal = (1 - uv.x() - uv.y()) * v1.normal + uv.x() * v2.normal +
                uv.y() * v3.normal;
  auto texCoords = (1 - uv.x() - uv.y()) * v1.texCoords +
                   uv.x() * v2.texCoords + uv.y() * v3.texCoords;

  // Material material;
  // material.ambient_color = (1 - uv.x() - uv.y()) * v1.material->ambient_color
  // + uv.x() * v2.material->ambient_color + uv.y() *
  // v3.material->ambient_color; material.diff_color = (1 - uv.x() - uv.y()) *
  // v1.material->diff_color + uv.x() * v2.material->diff_color + uv.y() *
  // v3.material->diff_color; material.spec_color = (1 - uv.x() - uv.y()) *
  // v1.material->spec_color + uv.x() * v2.material->spec_color + uv.y() *
  // v3.material->spec_color; material.shininess = (1 - uv.x() - uv.y()) *
  // v1.material->shininess + uv.x() * v2.material->shininess + uv.y() *
  // v3.material->shininess; material.ktran = (1 - uv.x() - uv.y()) *
  // v1.material->ktran + uv.x() * v2.material->ktran + uv.y() *
  // v3.material->ktran; material.emissive_color = (1 - uv.x() - uv.y()) *
  // v1.material->emissive_color + uv.x() * v2.material->emissive_color + uv.y()
  // * v3.material->emissive_color;

  return Vertex(position, normal, texCoords, nullptr);
}
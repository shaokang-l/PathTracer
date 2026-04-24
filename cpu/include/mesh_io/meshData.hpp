// MeshData.hpp
#pragma once
#include "utils/matrix.hpp"
#include <vector>

struct MeshData {
  std::vector<gl::vec3> positions;
  std::vector<gl::vec3> normals;                 // optional
  std::vector<gl::vec2> uvs;                     // optional
  std::vector<std::array<int, 3>> indices;       // each face as 3 indices
  std::vector<std::array<int, 3>> normalIndices; // normal‐idx per vertex
  std::vector<std::array<int, 3>> uvIndices;     // uv‐idx   per vertex
};

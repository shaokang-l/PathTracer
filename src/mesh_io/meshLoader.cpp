// MeshLoader.cpp
#define TINYOBJLOADER_IMPLEMENTATION
#include "mesh_io/meshLoader.hpp"
#include "external/tiny_obj_loader.h"
#include <stdexcept>

std::shared_ptr<TriangleMesh> loadOBJMesh(const std::string &path,
                                          std::shared_ptr<Material> mat,
                                          std::shared_ptr<MediumInterface> medium_interface)
{
  MeshData data;
  tinyobj::attrib_t attrib;
  std::vector<tinyobj::shape_t> shapes;
  std::string warn, err;

  if (!tinyobj::LoadObj(&attrib, &shapes, nullptr, &warn, &err, path.c_str()))
    throw std::runtime_error(warn + err);

  // copy positions
  for (size_t i = 0; i + 2 < attrib.vertices.size(); i += 3)
  {
    data.positions.emplace_back(attrib.vertices[i + 0], attrib.vertices[i + 1],
                                attrib.vertices[i + 2]);
  }
  // optional normals
  for (size_t i = 0; i + 2 < attrib.normals.size(); i += 3)
  {
    data.normals.emplace_back(attrib.normals[i + 0], attrib.normals[i + 1],
                              attrib.normals[i + 2]);
  }
  // optional uvs
  for (size_t i = 0; i + 1 < attrib.texcoords.size(); i += 2)
  {
    data.uvs.emplace_back(attrib.texcoords[i + 0], attrib.texcoords[i + 1]);
  }

  // faces → indices (use shape.mesh.indices, not attrib.indices)
  for (const auto &shape : shapes)
  {
    size_t indexOffset = 0;
    for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); ++f)
    {
      size_t fv = shape.mesh.num_face_vertices[f];
      if (fv != 3)
        throw std::runtime_error("Non-triangle face in OBJ");

      auto &I0 = shape.mesh.indices[indexOffset + 0];
      auto &I1 = shape.mesh.indices[indexOffset + 1];
      auto &I2 = shape.mesh.indices[indexOffset + 2];

      data.indices.push_back(
          {I0.vertex_index, I1.vertex_index, I2.vertex_index});

      data.normalIndices.push_back(
          {I0.normal_index, I1.normal_index, I2.normal_index});

      data.uvIndices.push_back(
          {I0.texcoord_index, I1.texcoord_index, I2.texcoord_index});

      indexOffset += fv;
    }
  }

  return std::make_shared<TriangleMesh>(std::move(data), mat, medium_interface);
}

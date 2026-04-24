#pragma once
#include "base/objectList.hpp"
#include "base/primitive.hpp"
#include "utils/utility.hpp"
#include <algorithm>
#include <cstdlib>

class MeshBVHNode : public Hittable {
public:
  MeshBVHNode(const std::vector<gl::vec3> &verts,
              const std::vector<std::array<int, 3>> &tris,
              const std::vector<gl::vec3> &normals,
              const std::vector<std::array<int, 3>> &normalIdx,
              const std::vector<gl::vec2> &uvs,
              const std::vector<std::array<int, 3>> &uvIdx,
              const std::vector<int> &ids, int start, int end);

  bool intersect(const Ray &ray, HitRecord &rec, float tmin,
                 float tmax) const override;

  AABB getAABB(float t0, float t1) override { return box; }

private:
  AABB box;
  const std::vector<gl::vec3> &vertices;
  const std::vector<std::array<int, 3>> &triangles;
  const std::vector<gl::vec3> &normals;
  const std::vector<std::array<int, 3>> &normalIdx;
  const std::vector<gl::vec2> &uvs;
  const std::vector<std::array<int, 3>> &uvIdx;
  std::unique_ptr<MeshBVHNode> left, right;
  std::vector<int> tri_ids;

  bool hitTriangle(int triIdx, const Ray &ray, float tmin, float tmax,
                   HitRecord &rec) const;
};
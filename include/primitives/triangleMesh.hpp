// TriangleMesh.hpp
#pragma once
#include "./meshBVH.hpp"
#include "base/primitive.hpp"
#include "mesh_io/meshData.hpp"
#include <memory>

class TriangleMesh : public Hittable
{
public:
  TriangleMesh(MeshData &&data, std::shared_ptr<Material> mat,
               std::shared_ptr<MediumInterface> medium_interface = nullptr);

  bool intersect(const Ray &ray, HitRecord &rec, float tMin,
                 float tMax) const override;

  AABB getAABB(float t0, float t1) override;

  const MeshData &getMeshData() const { return mesh; }
  size_t getLinearBVHNodeCount() const { return linear_bvh_nodes.size(); }
  size_t getLinearBVHTriangleRefCount() const { return linear_bvh_tri_indices.size(); }

  float pdf_value(const gl::vec3 &origin,
                  const gl::vec3 &direction) const override
  {
    // skip for now, don't treat this as a light
    // in the future, we can use the surface area of the mesh
    throw std::runtime_error("pdf_value not implemented for TriangleMesh");
    return 0.f;
  }

  gl::vec3 get_sample(const gl::vec3 &origin) const override
  {
    // skip for now, don't treat this as a light
    throw std::runtime_error("get_sample not implemented for TriangleMesh");
    return gl::vec3(1.f, 0.f, 0.f);
  }

  std::shared_ptr<MediumInterface> get_medium_interface() const override
  {
    return this->medium_interface;
  }

  void set_use_flat_bvh(bool use_flat_bvh) { this->use_flat_bvh = use_flat_bvh; }

private:
  bool intersectFlat(const Ray &ray, HitRecord &rec, float tmin,
                     float tmax) const;
  bool hitTriangleFlat(int triIdx, const Ray &ray, float tmin, float tmax,
                       HitRecord &rec) const;

  bool use_flat_bvh = false;
  std::shared_ptr<Material> material;
  std::shared_ptr<MediumInterface> medium_interface;
  MeshData mesh; // owns positions, normals, indices
  AABB meshAABB;
  std::unique_ptr<MeshBVHNode> bvh; // internal BVH over mesh.indices
  std::vector<LinearMeshBVHNode> linear_bvh_nodes;
  std::vector<int> linear_bvh_tri_indices;
};

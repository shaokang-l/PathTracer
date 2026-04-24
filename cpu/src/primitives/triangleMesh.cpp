#include "primitives/triangleMesh.hpp"
#include <limits>
#include <numeric>

TriangleMesh::TriangleMesh(MeshData &&data, std::shared_ptr<Material> mat,
                           std::shared_ptr<MediumInterface> medium_interface)
    : mesh(std::move(data)), material(std::move(mat)), medium_interface(medium_interface)
{

  // compute mesh AABB once
  gl::vec3 mn(+INFINITY), mx(-INFINITY);
  for (auto &v : mesh.positions)
  {
    mn = gl::min(mn, v);
    mx = gl::max(mx, v);
  }

  meshAABB = AABB(mn - gl::epsilon, mx + gl::epsilon);
  // build internal BVH
  std::vector<int> ids(mesh.indices.size());
  std::iota(ids.begin(), ids.end(), 0);
  bvh = std::make_unique<MeshBVHNode>(
      mesh.positions, mesh.indices, mesh.normals, mesh.normalIndices, mesh.uvs,
      mesh.uvIndices, ids, 0, (int)ids.size());
  this->objtype = ObjType::MESH_OBJ;
}

bool TriangleMesh::intersect(const Ray &ray, HitRecord &rec, float tmin,
                             float tmax) const
{

  if (!bvh->intersect(ray, rec, tmin, tmax))
    return false;

  rec.set_normal(ray, rec.normal);
  rec.material = material;
  rec.medium_interface = this->medium_interface;
  // planar UV fallback
  if (mesh.uvs.empty())
  {
    auto n = gl::abs(rec.normal);
    int axis_u = (n.x() > n.y() && n.x() > n.z()) ? 1 : 0;
    int axis_v = (axis_u == 1) ? 2 : (n.y() > n.z() ? 2 : 1);
    auto mnv = meshAABB.get_min();
    auto mxv = meshAABB.get_max();
    float u =
        (rec.position[axis_u] - mnv[axis_u]) / (mxv[axis_u] - mnv[axis_u]);
    float v =
        (rec.position[axis_v] - mnv[axis_v]) / (mxv[axis_v] - mnv[axis_v]);
    rec.texCoords = gl::clamp(gl::vec2(u, v), 0.f, 1.f);
  }

  return true;
}

AABB TriangleMesh::getAABB(float, float) { return meshAABB; }
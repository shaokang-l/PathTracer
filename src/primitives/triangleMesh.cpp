#include "primitives/triangleMesh.hpp"
#include "feature_flags.hpp"
#include <array>
#include <cassert>
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
  linear_bvh_nodes.reserve(ids.size() * 2);
  linear_bvh_tri_indices.reserve(ids.size());
  bvh->flatten(linear_bvh_nodes, linear_bvh_tri_indices);
#ifdef USE_FLAT_MESH_BVH
  use_flat_bvh = true;
#endif
  this->objtype = ObjType::MESH_OBJ;
}

bool TriangleMesh::intersect(const Ray &ray, HitRecord &rec, float tmin,
                             float tmax) const
{

  bool is_hit = use_flat_bvh ? intersectFlat(ray, rec, tmin, tmax)
                             : bvh->intersect(ray, rec, tmin, tmax);
  if (!is_hit)
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

bool TriangleMesh::intersectFlat(const Ray &ray, HitRecord &rec, float tmin,
                                 float tmax) const
{
  if (linear_bvh_nodes.empty())
    return false;

  bool hit = false;
  float closest = tmax;

  struct StackEntry
  {
    int node_index;
    float t_enter;
    bool box_tested;
  };

  std::array<StackEntry, 128> stack;
  int stack_size = 0;
  stack[stack_size++] = {0, tmin, false}; // root node

  // DFS traversal over linear_bvh_nodes
  while (stack_size > 0)
  {
    StackEntry entry = stack[--stack_size];

    int node_index = entry.node_index;
    const auto &node = linear_bvh_nodes[node_index];
    if (entry.box_tested)
    {
      if (entry.t_enter > closest)
        continue;
    }
    else
    {
      float node_t = 0.0f;
      if (!node.box.intersect(ray, tmin, closest, node_t))
        continue;
    }

    if (node.isLeaf())
    {
      for (int i = 0; i < node.tri_count; ++i)
      {
        int tri_id = linear_bvh_tri_indices[node.first_tri + i];
        if (hitTriangleFlat(tri_id, ray, tmin, closest, rec))
        {
          hit = true;
          closest = rec.t;
        }
      }
    }
    else
    {
      float left_t = 0.0f;
      float right_t = 0.0f;
      bool hit_left_box =
          node.left >= 0 &&
          linear_bvh_nodes[node.left].box.intersect(ray, tmin, closest, left_t);
      bool hit_right_box =
          node.right >= 0 &&
          linear_bvh_nodes[node.right].box.intersect(ray, tmin, closest, right_t);

      if (hit_left_box && hit_right_box)
      {
        // Stack is LIFO, so push the farther child first.
        if (left_t < right_t)
        {
          assert(stack_size + 2 <= static_cast<int>(stack.size()));
          stack[stack_size++] = {node.right, right_t, true};
          stack[stack_size++] = {node.left, left_t, true};
        }
        else
        {
          assert(stack_size + 2 <= static_cast<int>(stack.size()));
          stack[stack_size++] = {node.left, left_t, true};
          stack[stack_size++] = {node.right, right_t, true};
        }
      }
      else if (hit_left_box)
      {
        assert(stack_size + 1 <= static_cast<int>(stack.size()));
        stack[stack_size++] = {node.left, left_t, true};
      }
      else if (hit_right_box)
      {
        assert(stack_size + 1 <= static_cast<int>(stack.size()));
        stack[stack_size++] = {node.right, right_t, true};
      }
    }
  }

  return hit;
}

AABB TriangleMesh::getAABB(float, float) { return meshAABB; }


bool TriangleMesh::hitTriangleFlat(int triIdx, const Ray &ray, float tmin,
                                   float tmax, HitRecord &rec) const
{
  gl::hit_count++;
  auto &tri = mesh.indices[triIdx];
  const gl::vec3 &p0 = mesh.positions[tri[0]];
  const gl::vec3 &p1 = mesh.positions[tri[1]];
  const gl::vec3 &p2 = mesh.positions[tri[2]];

  gl::vec3 e1 = p1 - p0;
  gl::vec3 e2 = p2 - p0;
  gl::vec3 P = gl::cross(ray.getDirection(), e2);
  float det = gl::dot(e1, P);
  if (fabs(det) < 1e-8f)
    return false;
  float invDet = 1.0f / det;

  gl::vec3 T = ray.getOrigin() - p0;
  float u = gl::dot(T, P) * invDet;
  if (u < 0 || u > 1)
    return false;

  gl::vec3 Q = gl::cross(T, e1);
  float v = gl::dot(ray.getDirection(), Q) * invDet;
  if (v < 0 || u + v > 1)
    return false;

  float t = gl::dot(e2, Q) * invDet;
  if (t < tmin || t > tmax)
    return false;

  if (!mesh.normals.empty() && !mesh.normalIndices.empty()) {
    auto &ni = mesh.normalIndices[triIdx];
    gl::vec3 n0 = mesh.normals[ni[0]], n1 = mesh.normals[ni[1]],
             n2 = mesh.normals[ni[2]];
    rec.normal = gl::normalize((1 - u - v) * n0 + u * n1 + v * n2);
  } else {
    rec.normal = gl::normalize(gl::cross(e1, e2));
  }

  gl::vec3 Ng = gl::normalize(gl::cross(e1, e2));
  if (gl::dot(rec.normal, Ng) < 0.f)
    rec.normal = -rec.normal;

  if (!mesh.uvs.empty() && !mesh.uvIndices.empty()) {
    auto &ui = mesh.uvIndices[triIdx];
    gl::vec2 uv0 = mesh.uvs[ui[0]], uv1 = mesh.uvs[ui[1]],
             uv2 = mesh.uvs[ui[2]];
    rec.texCoords = (1 - u - v) * uv0 + u * uv1 + v * uv2;
  }

  rec.t = t;
  rec.position = ray.at(t);
  return true;
}
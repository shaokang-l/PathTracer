// MeshBVHNode.cpp
#include "primitives/meshBVH.hpp"
#include "utils/utility.hpp"
#include <algorithm>

MeshBVHNode::MeshBVHNode(const std::vector<gl::vec3> &verts,
                         const std::vector<std::array<int, 3>> &tris,
                         const std::vector<gl::vec3> &normals,
                         const std::vector<std::array<int, 3>> &normalIdx,
                         const std::vector<gl::vec2> &uvs,
                         const std::vector<std::array<int, 3>> &uvIdx,
                         const std::vector<int> &ids, int start, int end)
    : vertices(verts), triangles(tris), normals(normals), normalIdx(normalIdx),
      uvs(uvs), uvIdx(uvIdx) {
  // Compute bounding box for triangles[start..end)
  gl::vec3 mn(+INFINITY), mx(-INFINITY);
  for (int i = start; i < end; ++i) {
    auto &t = triangles[ids[i]];
    for (int k : {t[0], t[1], t[2]}) {
      mn = gl::min(mn, vertices[k]);
      mx = gl::max(mx, vertices[k]);
    }
  }

  box = AABB(mn - gl::epsilon, mx + gl::epsilon);

  int count = end - start;
  if (count <= 4) {
    // leaf
    tri_ids.assign(ids.begin() + start, ids.begin() + end);
  } else {
    // choose split axis by longest extent
    gl::vec3 ext = mx - mn;
    int axis = ext.x() > ext.y() && ext.x() > ext.z() ? 0
               : ext.y() > ext.z()                    ? 1
                                                      : 2;

    // sort indices by triangle centroid on that axis
    std::vector<int> sorted(ids.begin() + start, ids.begin() + end);
    std::sort(sorted.begin(), sorted.end(), [&](int a, int b) {
      auto ca =
          (vertices[tris[a][0]] + vertices[tris[a][1]] + vertices[tris[a][2]]) /
          3.0f;
      auto cb =
          (vertices[tris[b][0]] + vertices[tris[b][1]] + vertices[tris[b][2]]) /
          3.0f;
      return ca[axis] < cb[axis];
    });
    int mid = count / 2;
    left = std::make_unique<MeshBVHNode>(verts, tris, normals, normalIdx, uvs,
                                         uvIdx, sorted, 0, mid);
    right = std::make_unique<MeshBVHNode>(verts, tris, normals, normalIdx, uvs,
                                          uvIdx, sorted, mid, count);
  }
}

bool MeshBVHNode::intersect(const Ray &ray, HitRecord &rec, float tmin,
                            float tmax) const {
  if (!box.intersect(ray, tmin, tmax))
    return false;

  bool hit = false;
  float closest = tmax;

  if (!left && !right) {
    // leaf: test all triangles
    for (int id : tri_ids) {
      if (hitTriangle(id, ray, tmin, closest, rec)) {
        hit = true;
        closest = rec.t;
      }
    }
  } else {
    if (left && left->intersect(ray, rec, tmin, closest)) {
      hit = true;
      closest = rec.t;
    }
    if (right && right->intersect(ray, rec, tmin, closest)) {
      hit = true;
    }
  }
  return hit;
}

bool MeshBVHNode::hitTriangle(int triIdx, const Ray &ray, float tmin,
                              float tmax, HitRecord &rec) const {
  gl::hit_count++;
  auto &tri = triangles[triIdx];
  const gl::vec3 &p0 = vertices[tri[0]];
  const gl::vec3 &p1 = vertices[tri[1]];
  const gl::vec3 &p2 = vertices[tri[2]];

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

  // 1) interpolate smooth normal if available, else geometric
  if (!normals.empty() && !normalIdx.empty()) {
    auto &ni = normalIdx[triIdx];
    gl::vec3 n0 = normals[ni[0]], n1 = normals[ni[1]], n2 = normals[ni[2]];
    rec.normal = gl::normalize((1 - u - v) * n0 + u * n1 + v * n2);
  } else {
    rec.normal = gl::normalize(gl::cross(e1, e2));
  }

  // ★ 2) make sure rec.normal lies on the same side as the face normal ★
  {
    // geometric face normal
    gl::vec3 Ng = gl::normalize(gl::cross(e1, e2));
    // if the dot is negative, flip rec.normal
    if (gl::dot(rec.normal, Ng) < 0.f)
      rec.normal = -rec.normal;
  }

  // 3) uv interpolation
  if (!uvs.empty() && !uvIdx.empty()) {
    auto &ui = uvIdx[triIdx];
    gl::vec2 uv0 = uvs[ui[0]], uv1 = uvs[ui[1]], uv2 = uvs[ui[2]];
    rec.texCoords = (1 - u - v) * uv0 + u * uv1 + v * uv2;
  }

  rec.t = t;
  rec.position = ray.at(t);
  return true;
}
#include "Scene.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "../../cpu/include/external/tiny_obj_loader.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>

#ifndef PATHTRACER_ASSET_DIR
#define PATHTRACER_ASSET_DIR "assets"
#endif

namespace mypt {

  void Scene::computeBounds()
  {
    bounds = owl::box3f();
    for (const auto &m : meshes)
      for (const auto &v : m.vertices)
        bounds.extend(v);
  }

  static void pushBox(TriangleMesh &mesh,
                      const owl::vec3f &lo,
                      const owl::vec3f &hi)
  {
    using owl::vec3f;
    using owl::vec3i;
    const int base = int(mesh.vertices.size());
    const vec3f v[8] = {
      { lo.x, lo.y, lo.z }, { hi.x, lo.y, lo.z },
      { lo.x, hi.y, lo.z }, { hi.x, hi.y, lo.z },
      { lo.x, lo.y, hi.z }, { hi.x, lo.y, hi.z },
      { lo.x, hi.y, hi.z }, { hi.x, hi.y, hi.z },
    };
    for (int i = 0; i < 8; ++i) mesh.vertices.push_back(v[i]);
    const vec3i tris[12] = {
      { 0, 1, 3 }, { 0, 3, 2 },
      { 4, 6, 7 }, { 4, 7, 5 },
      { 0, 2, 6 }, { 0, 6, 4 },
      { 1, 5, 7 }, { 1, 7, 3 },
      { 2, 3, 7 }, { 2, 7, 6 },
      { 0, 4, 5 }, { 0, 5, 1 },
    };
    for (int i = 0; i < 12; ++i)
      mesh.indices.push_back(vec3i(tris[i].x + base,
                                   tris[i].y + base,
                                   tris[i].z + base));
  }

  static MaterialGPU makeLambertian(const owl::vec3f &albedo)
  {
    MaterialGPU m = {};
    m.kind = MATERIAL_LAMBERTIAN;
    m.albedo = albedo;
    return m;
  }

  static MaterialGPU makeMirror(const owl::vec3f &albedo)
  {
    MaterialGPU m = {};
    m.kind = MATERIAL_MIRROR;
    m.albedo = albedo;
    return m;
  }

  static MaterialGPU makeConductor(const owl::vec3f &eta,
                                   const owl::vec3f &k,
                                   float             alpha)
  {
    MaterialGPU m = {};
    m.kind = MATERIAL_CONDUCTOR;
    m.albedo = owl::vec3f(1.f);
    m.eta = eta;
    m.k = k;
    m.alpha_x = alpha;
    m.alpha_y = alpha;
    return m;
  }

  static MaterialGPU makeDielectric(float ior, float alpha)
  {
    MaterialGPU m = {};
    m.kind = MATERIAL_DIELECTRIC;
    m.ior = ior;
    m.alpha_x = alpha;
    m.alpha_y = alpha;
    return m;
  }

  static MaterialGPU makeThinDielectric(float ior)
  {
    MaterialGPU m = {};
    m.kind = MATERIAL_THIN_DIELECTRIC;
    m.ior = ior;
    return m;
  }

  static MaterialGPU makeEmissive(const owl::vec3f &emission)
  {
    MaterialGPU m = {};
    m.kind = MATERIAL_EMISSIVE;
    m.emission = emission;
    return m;
  }

  enum class ObjUpAxis {
    Y,
    Z,
  };

  static TriangleMesh loadObjMesh(const std::string &path,
                                  int32_t            materialId,
                                  float              targetHeight,
                                  const owl::vec3f  &baseCenter,
                                  ObjUpAxis          sourceUpAxis)
  {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> objMaterials;
    std::string warn;
    std::string err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &objMaterials,
                          &warn, &err, path.c_str(), nullptr,
                          /*triangulate=*/true)) {
      throw std::runtime_error("Failed to load OBJ '" + path + "': " + warn + err);
    }
    if (!warn.empty()) {
      std::cerr << "[mypt] OBJ warning: " << warn << std::endl;
    }

    owl::box3f srcBounds;
    for (size_t i = 0; i + 2 < attrib.vertices.size(); i += 3) {
      srcBounds.extend(owl::vec3f(attrib.vertices[i + 0],
                                  attrib.vertices[i + 1],
                                  attrib.vertices[i + 2]));
    }

    const owl::vec3f srcCenter = srcBounds.center();
    const float srcHeight = (sourceUpAxis == ObjUpAxis::Z)
      ? (srcBounds.upper.z - srcBounds.lower.z)
      : (srcBounds.upper.y - srcBounds.lower.y);
    const float scale = srcHeight > 0.f ? targetHeight / srcHeight : 1.f;

    TriangleMesh mesh;
    mesh.materialId = materialId;
    mesh.vertices.reserve(attrib.vertices.size() / 3);

    for (size_t i = 0; i + 2 < attrib.vertices.size(); i += 3) {
      const owl::vec3f p(attrib.vertices[i + 0],
                         attrib.vertices[i + 1],
                         attrib.vertices[i + 2]);

      if (sourceUpAxis == ObjUpAxis::Z) {
        mesh.vertices.push_back(owl::vec3f(
          baseCenter.x + (p.x - srcCenter.x) * scale,
          baseCenter.y + (p.z - srcBounds.lower.z) * scale,
          baseCenter.z - (p.y - srcCenter.y) * scale));
      } else {
        mesh.vertices.push_back(owl::vec3f(
          baseCenter.x + (p.x - srcCenter.x) * scale,
          baseCenter.y + (p.y - srcBounds.lower.y) * scale,
          baseCenter.z + (p.z - srcCenter.z) * scale));
      }
    }

    for (const auto &shape : shapes) {
      size_t indexOffset = 0;
      for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); ++f) {
        const size_t fv = shape.mesh.num_face_vertices[f];
        if (fv != 3) {
          throw std::runtime_error("Non-triangle face after OBJ triangulation");
        }

        const auto &i0 = shape.mesh.indices[indexOffset + 0];
        const auto &i1 = shape.mesh.indices[indexOffset + 1];
        const auto &i2 = shape.mesh.indices[indexOffset + 2];
        mesh.indices.push_back(owl::vec3i(i0.vertex_index,
                                          i1.vertex_index,
                                          i2.vertex_index));
        indexOffset += fv;
      }
    }

    return mesh;
  }

  Scene Scene::makeTestScene()
  {
    using owl::vec3f;
    Scene s;

    s.materials.push_back(makeLambertian(vec3f(0.72f, 0.70f, 0.66f))); // floor/back
    s.materials.push_back(makeLambertian(vec3f(0.65f, 0.08f, 0.06f))); // left wall
    s.materials.push_back(makeConductor(vec3f(1.35f, 0.97f, 0.62f),
                                        vec3f(7.62f, 6.62f, 5.31f),
                                        0.35f)); // rough gold wall
    s.materials.push_back(makeDielectric(1.5f, 0.08f)); // rough glass bunny
    s.materials.push_back(makeLambertian(vec3f(0.34f, 0.36f, 0.42f))); // pedestal
    s.materials.push_back(makeEmissive(vec3f(16.f, 15.f, 13.f)));

    TriangleMesh floor;
    floor.materialId = 0;
    pushBox(floor, vec3f(-6.f, -0.1f, -6.f), vec3f(6.f, 0.f, 6.f));
    s.meshes.push_back(std::move(floor));

    TriangleMesh leftWall;
    leftWall.materialId = 1;
    pushBox(leftWall, vec3f(-6.0f, 0.f, -6.f), vec3f(-5.9f, 5.5f, 6.f));
    s.meshes.push_back(std::move(leftWall));

    TriangleMesh rightWall;
    rightWall.materialId = 2;
    pushBox(rightWall, vec3f(5.9f, 0.f, -6.f), vec3f(6.0f, 5.5f, 6.f));
    s.meshes.push_back(std::move(rightWall));

    TriangleMesh backWall;
    backWall.materialId = 0;
    pushBox(backWall, vec3f(-6.f, 0.f, 5.9f), vec3f(6.f, 5.5f, 6.f));
    s.meshes.push_back(std::move(backWall));

    TriangleMesh pedestal;
    pedestal.materialId = 4;
    pushBox(pedestal, vec3f(-1.5f, 0.f, -1.5f), vec3f(1.5f, 0.35f, 1.5f));
    s.meshes.push_back(std::move(pedestal));

    const std::string bunnyPath = std::string(PATHTRACER_ASSET_DIR) + "/bunny.obj";
    TriangleMesh bunny = loadObjMesh(bunnyPath,
                                     /*materialId=*/3,
                                     /*targetHeight=*/2.4f,
                                     vec3f(0.f, 0.35f, 0.f),
                                     ObjUpAxis::Y);
    s.meshes.push_back(std::move(bunny));

    TriangleMesh light;
    light.materialId = 5;
    pushBox(light, vec3f(-2.0f, 5.35f, -2.0f), vec3f(2.0f, 5.45f, 2.0f));
    s.meshes.push_back(std::move(light));

    LightGPU quadLight;
    quadLight.kind = LIGHT_QUAD;
    quadLight.emission = s.materials[5].emission;
    quadLight.v0 = vec3f(-2.0f, 5.35f, -2.0f);
    quadLight.edgeU = vec3f(4.f, 0.f, 0.f);
    quadLight.edgeV = vec3f(0.f, 0.f, 4.f);
    quadLight.normal = vec3f(0.f, -1.f, 0.f);
    quadLight.area = 16.f;
    s.lights.push_back(quadLight);

    s.computeBounds();
    return s;
  }

} // namespace mypt

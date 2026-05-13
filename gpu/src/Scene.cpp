#include "Scene.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "../../cpu/include/external/tiny_obj_loader.h"

#include <cmath>
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

  static owl::vec3f normalizeVec(const owl::vec3f &v)
  {
    const float len = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    return len > 0.f ? v * (1.f / len) : owl::vec3f(0.f);
  }

  static owl::vec3f rotateY(const owl::vec3f &p, float angle)
  {
    const float c = std::cos(angle);
    const float s = std::sin(angle);
    return owl::vec3f(c * p.x + s * p.z, p.y, -s * p.x + c * p.z);
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

  static TriangleMesh loadObjMeshCpuStyle(const std::string &path,
                                          int32_t            materialId,
                                          float              scale,
                                          float              rotateYRadians,
                                          const owl::vec3f  &translate)
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

    TriangleMesh mesh;
    mesh.materialId = materialId;
    mesh.vertices.reserve(attrib.vertices.size() / 3);

    for (size_t i = 0; i + 2 < attrib.vertices.size(); i += 3) {
      const owl::vec3f p(attrib.vertices[i + 0],
                         attrib.vertices[i + 1],
                         attrib.vertices[i + 2]);
      mesh.vertices.push_back(rotateY(p * scale, rotateYRadians) + translate);
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

  struct ObjData {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
  };

  static ObjData loadObjData(const std::string &path)
  {
    ObjData data;
    std::vector<tinyobj::material_t> objMaterials;
    std::string warn;
    std::string err;

    if (!tinyobj::LoadObj(&data.attrib, &data.shapes, &objMaterials,
                          &warn, &err, path.c_str(), nullptr,
                          /*triangulate=*/true)) {
      throw std::runtime_error("Failed to load OBJ '" + path + "': " + warn + err);
    }
    if (!warn.empty()) {
      std::cerr << "[mypt] OBJ warning: " << warn << std::endl;
    }

    return data;
  }

  static TriangleMesh makeMeshFromObjData(const ObjData      &data,
                                          int32_t             materialId,
                                          float               scale,
                                          float               rotateYRadians,
                                          const owl::vec3f   &translate)
  {
    TriangleMesh mesh;
    mesh.materialId = materialId;
    mesh.vertices.reserve(data.attrib.vertices.size() / 3);

    for (size_t i = 0; i + 2 < data.attrib.vertices.size(); i += 3) {
      const owl::vec3f p(data.attrib.vertices[i + 0],
                         data.attrib.vertices[i + 1],
                         data.attrib.vertices[i + 2]);
      mesh.vertices.push_back(rotateY(p * scale, rotateYRadians) + translate);
    }

    for (const auto &shape : data.shapes) {
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

    s.materials.push_back(makeLambertian(vec3f(1.f))); // ground
    s.materials.push_back(makeEmissive(vec3f(10.f)));   // top light
    s.materials.push_back(makeConductor(vec3f(0.14f, 0.43f, 1.38f),
                                        vec3f(4.54f, 2.455f, 1.914f),
                                        0.0001f));       // GOLD_MAT
    s.materials.push_back(makeConductor(vec3f(0.14f, 0.43f, 1.38f),
                                        vec3f(4.54f, 2.455f, 1.914f),
                                        0.1f));          // ROUGH_GOLD_MAT
    s.materials.push_back(makeConductor(vec3f(0.04f, 0.06f, 0.04f),
                                        vec3f(4.8f, 3.586f, 2.657f),
                                        0.0001f));       // SILVER_MAT
    s.materials.push_back(makeDielectric(1.5f, 0.0001f)); // GLASS_MAT
    s.materials.push_back(makeThinDielectric(1.5f));      // THIN_GLASS
    s.materials.push_back(makeLambertian(vec3f(0.8f, 0.2f, 0.2f))); // red lambertian

    TriangleMesh floor;
    floor.materialId = 0;
    pushBox(floor, vec3f(-40.f, -0.55f, -40.f), vec3f(40.f, -0.5f, 40.f));
    s.meshes.push_back(std::move(floor));

    const ObjData bunnyObj = loadObjData(std::string(PATHTRACER_ASSET_DIR) + "/bunny.obj");
    const float bunnyScale = 24.f;
    const float bunnyRotY = float(M_PI_2);
    const float baseY = -0.5f;
    const vec3f bunnyPositions[] = {
      vec3f(-5.8f, baseY, -1.5f),
      vec3f(-3.4f, baseY,  1.6f),
      vec3f(-0.9f, baseY, -1.4f),
      vec3f( 1.6f, baseY,  1.5f),
      vec3f( 4.1f, baseY, -1.3f),
      vec3f( 6.2f, baseY,  1.6f),
    };
    const int32_t bunnyMaterials[] = {
      7, // lambertian
      5, // glass
      6, // thin glass
      2, // gold
      3, // rough gold
      4, // silver
    };

    for (int i = 0; i < 6; ++i) {
      s.meshes.push_back(makeMeshFromObjData(bunnyObj,
                                             bunnyMaterials[i],
                                             bunnyScale,
                                             bunnyRotY,
                                             bunnyPositions[i]));
    }

    TriangleMesh light;
    light.materialId = 1;
    pushBox(light, vec3f(-2.f, 13.95f, -3.f), vec3f(6.f, 14.0f, 5.f));
    s.meshes.push_back(std::move(light));

    TriangleMesh goldBackdrop;
    goldBackdrop.materialId = 2;
    pushBox(goldBackdrop, vec3f(-12.05f, -40.f, -40.f), vec3f(-12.f, 40.f, 40.f));
    s.meshes.push_back(std::move(goldBackdrop));

    TriangleMesh roughGoldBackdrop;
    roughGoldBackdrop.materialId = 3;
    pushBox(roughGoldBackdrop, vec3f(-40.f, -40.f, -8.05f), vec3f(40.f, 40.f, -8.f));
    s.meshes.push_back(std::move(roughGoldBackdrop));

    LightGPU quadLight;
    quadLight.kind = LIGHT_QUAD;
    quadLight.emission = s.materials[1].emission;
    quadLight.v0 = vec3f(-2.f, 13.95f, -3.f);
    quadLight.edgeU = vec3f(8.f, 0.f, 0.f);
    quadLight.edgeV = vec3f(0.f, 0.f, 8.f);
    quadLight.normal = vec3f(0.f, -1.f, 0.f);
    quadLight.area = 64.f;
    s.lights.push_back(quadLight);

    s.computeBounds();
    return s;
  }

} // namespace mypt

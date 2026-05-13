#include "Scene.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "../../cpu/include/external/tiny_obj_loader.h"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

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

  static MaterialGPU makeDisneyPrincipled(const owl::vec3f &baseColor,
                                          float             specularTransmission,
                                          float             metallic,
                                          float             subsurface,
                                          float             specular,
                                          float             roughness,
                                          float             specularTint,
                                          float             anisotropic,
                                          float             sheen,
                                          float             sheenTint,
                                          float             clearcoat,
                                          float             clearcoatGloss,
                                          float             eta)
  {
    MaterialGPU m = {};
    m.kind = MATERIAL_DISNEY_PRINCIPLED;
    m.albedo = baseColor;
    m.baseColor = baseColor;
    m.specularTransmission = specularTransmission;
    m.metallic = metallic;
    m.subsurface = subsurface;
    m.specular = specular;
    m.roughness = roughness;
    m.specularTint = specularTint;
    m.anisotropic = anisotropic;
    m.sheen = sheen;
    m.sheenTint = sheenTint;
    m.clearcoat = clearcoat;
    m.clearcoatGloss = clearcoatGloss;
    m.ior = eta;
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

  static owl::vec3f cornellCpuToGpu(float cx, float cy, float cz)
  {
    constexpr float kScale = 1.f / 50.f;
    constexpr float ox = 277.5f;
    constexpr float oy = 277.5f;
    constexpr float oz = 277.5f;
    return owl::vec3f((cx - ox) * kScale, (cy - oy) * kScale, (cz - oz) * kScale);
  }

  static void appendUvSphere(TriangleMesh &mesh,
                             const owl::vec3f &center,
                             float             radius,
                             int               latBands,
                             int               lonBands)
  {
    using owl::vec3f;
    using owl::vec3i;
    const int base = int(mesh.vertices.size());
    for (int lat = 0; lat <= latBands; ++lat) {
      const float theta = float(M_PI) * float(lat) / float(latBands);
      const float sinT = std::sin(theta);
      const float cosT = std::cos(theta);
      for (int lon = 0; lon <= lonBands; ++lon) {
        const float phi = float(2.0 * M_PI) * float(lon) / float(lonBands);
        const float cosP = std::cos(phi);
        const float sinP = std::sin(phi);
        const float x = cosP * sinT;
        const float y = cosT;
        const float z = sinP * sinT;
        mesh.vertices.push_back(center + radius * vec3f(x, y, z));
      }
    }
    for (int lat = 0; lat < latBands; ++lat) {
      for (int lon = 0; lon < lonBands; ++lon) {
        const int first = base + lat * (lonBands + 1) + lon;
        const int second = first + lonBands + 1;
        mesh.indices.push_back(vec3i(first, second, first + 1));
        mesh.indices.push_back(vec3i(second, second + 1, first + 1));
      }
    }
  }

  Scene Scene::makeDisneyCornellScene()
  {
    using owl::vec3f;
    Scene s;

    constexpr float wallT = 0.035f;
    constexpr float L = 5.55f;
    constexpr float kR = 50.f / 50.f;

    const vec3f whiteDiffuse(0.73f);
    const vec3f redDiffuse(0.65f, 0.05f, 0.05f);
    const vec3f greenDiffuse(0.12f, 0.45f, 0.15f);

    s.materials.push_back(makeLambertian(whiteDiffuse)); // 0
    s.materials.push_back(makeLambertian(redDiffuse));   // 1
    s.materials.push_back(makeLambertian(greenDiffuse));  // 2
    s.materials.push_back(makeEmissive(vec3f(10.f)));    // 3 — ceiling quad / mesh

    s.materials.push_back(makeDisneyPrincipled(
      vec3f(0.82f, 0.67f, 0.16f), 0.f, 0.f, 0.5f, 0.5f, 0.5f, 0.5f, 0.f,
      0.f, 0.5f, 0.f, 0.5f, 1.5f));
    s.materials.push_back(makeDisneyPrincipled(
      vec3f(0.25f, 0.83f, 0.36f), 0.f, 0.8f, 0.5f, 0.5f, 0.5f, 0.5f, 0.f,
      0.f, 0.5f, 1.f, 0.5f, 1.5f));
    s.materials.push_back(makeDisneyPrincipled(
      vec3f(0.75f, 0.83f, 0.46f), 0.f, 0.1f, 0.5f, 0.5f, 0.5f, 0.5f, 0.f,
      1.f, 0.5f, 0.5f, 0.5f, 1.5f));
    s.materials.push_back(makeDisneyPrincipled(
      vec3f(0.75f, 0.83f, 0.46f), 0.5f, 0.1f, 0.f, 1.f, 0.5f, 0.5f, 0.f,
      0.f, 0.5f, 0.f, 0.5f, 1.5f));
    s.materials.push_back(makeDisneyPrincipled(
      vec3f(0.5f, 0.23f, 0.84f), 0.f, 0.5f, 1.f, 1.f, 0.5f, 0.5f, 0.1f,
      0.5f, 0.5f, 0.5f, 0.5f, 1.5f));
    s.materials.push_back(makeDisneyPrincipled(
      vec3f(0.5f, 0.9f, 0.84f), 1.f, 0.f, 1.f, 1.f, 0.1f, 0.5f, 0.1f,
      0.5f, 0.5f, 0.5f, 0.5f, 1.5f));
    s.materials.push_back(makeDisneyPrincipled(
      vec3f(0.9f, 0.9f, 0.84f), 0.f, 1.f, 1.f, 1.f, 0.1f, 0.5f, 0.1f,
      0.5f, 0.5f, 0.5f, 0.5f, 1.5f));
    s.materials.push_back(makeDisneyPrincipled(
      vec3f(0.2f, 0.2f, 0.3f), 0.5f, 0.5f, 0.5f, 0.5f, 0.2f, 0.5f, 0.1f,
      0.5f, 0.5f, 0.5f, 0.5f, 1.5f));
    s.materials.push_back(makeDisneyPrincipled(
      vec3f(0.5f, 0.6f, 0.7f), 0.2f, 0.8f, 0.2f, 0.7f, 0.1f, 0.f, 0.3f,
      0.5f, 0.5f, 0.5f, 0.5f, 1.5f));
    s.materials.push_back(makeDisneyPrincipled(
      vec3f(0.9f, 0.2f, 0.3f), 0.f, 0.f, 0.8f, 0.3f, 0.1f, 0.f, 0.f,
      1.f, 0.5f, 1.f, 0.5f, 1.5f));
    s.materials.push_back(makeDisneyPrincipled(
      vec3f(0.3f, 0.5f, 0.3f), 1.f, 0.9f, 0.8f, 0.3f, 0.2f, 0.f, 0.3f,
      1.f, 0.5f, 1.f, 0.5f, 1.5f));
    s.materials.push_back(makeDisneyPrincipled(
      vec3f(0.1f, 0.1f, 0.3f), 0.5f, 0.5f, 0.5f, 0.3f, 0.9f, 0.f, 0.1f,
      0.f, 0.5f, 0.f, 0.5f, 1.5f));

    auto pushWallBox = [&](float x0, float x1, float y0, float y1,
                           float z0, float z1, int32_t matId) {
      TriangleMesh w;
      w.materialId = matId;
      pushBox(w, vec3f(x0, y0, z0), vec3f(x1, y1, z1));
      s.meshes.push_back(std::move(w));
    };

    pushWallBox(-L - wallT, -L + wallT, -L, L, -L, L, 1);
    pushWallBox(L - wallT, L + wallT, -L, L, -L, L, 2);
    pushWallBox(-L, L, -L - wallT, -L + wallT, -L, L, 0);
    pushWallBox(-L, L, L - wallT, L + wallT, -L, L, 0);
    pushWallBox(-L, L, -L, L, L - wallT, L + wallT, 0);

    const float xi0 = (150.f - 277.5f) / 50.f;
    const float xi1 = (400.f - 277.5f) / 50.f;
    const float zi0 = (100.f - 277.5f) / 50.f;
    const float zi1 = (400.f - 277.5f) / 50.f;
    const float yCeil = (555.f - 277.5f) / 50.f;

    pushWallBox(-L, L, yCeil - wallT, yCeil + wallT, zi1, L, 0);
    pushWallBox(-L, L, yCeil - wallT, yCeil + wallT, -L, zi0, 0);
    pushWallBox(-L, xi0, yCeil - wallT, yCeil + wallT, zi0, zi1, 0);
    pushWallBox(xi1, L, yCeil - wallT, yCeil + wallT, zi0, zi1, 0);

    TriangleMesh lightMesh;
    lightMesh.materialId = 3;
    const float yLight = (554.f - 277.5f) / 50.f;
    pushBox(lightMesh,
            vec3f(xi0, yLight - wallT, zi0),
            vec3f(xi1, yLight + wallT, zi1));
    s.meshes.push_back(std::move(lightMesh));

    const float sphereCx[] = {90.f, 210.f, 330.f, 450.f};
    const float sphereCy[] = {130.f, 250.f, 370.f};
    const int32_t shellMatBase = 4;
    int shellIdx = 0;
    for (float cy : sphereCy) {
      for (float cx : sphereCx) {
        TriangleMesh sp;
        sp.materialId = shellMatBase + shellIdx;
        appendUvSphere(sp,
                       cornellCpuToGpu(cx, cy, 190.f),
                       kR,
                       28,
                       28);
        s.meshes.push_back(std::move(sp));
        ++shellIdx;
      }
    }

    LightGPU quadLight;
    quadLight.kind = LIGHT_QUAD;
    quadLight.emission = s.materials[3].emission;
    const vec3f v0 = cornellCpuToGpu(150.f, 554.f, 100.f);
    quadLight.v0 = v0;
    quadLight.edgeU = cornellCpuToGpu(400.f, 554.f, 100.f) - v0;
    quadLight.edgeV = cornellCpuToGpu(150.f, 554.f, 400.f) - v0;
    quadLight.normal = vec3f(0.f, -1.f, 0.f);
    quadLight.area = owl::length(quadLight.edgeU) * owl::length(quadLight.edgeV);
    s.lights.push_back(quadLight);

    s.computeBounds();
    return s;
  }

  static std::vector<MaterialGPU> disneyPrincipledPresetsFromCommonMaterials()
  {
    using owl::vec3f;
    std::vector<MaterialGPU> v;
    v.reserve(17);

    v.push_back(makeDisneyPrincipled(
      vec3f(0.8f, 0.1f, 0.1f), 0.f, 0.f, 0.f, 0.5f, 0.6f, 0.f, 0.f, 0.f, 0.f, 0.f,
      0.f, 1.460f)); // MatteRedPlastic
    v.push_back(makeDisneyPrincipled(
      vec3f(0.1f, 0.2f, 0.8f), 0.f, 0.f, 0.f, 0.5f, 0.1f, 0.f, 0.f, 0.f, 0.f, 0.f,
      0.f, 1.5f)); // GlossyBluePlastic
    v.push_back(makeDisneyPrincipled(
      vec3f(1.f, 1.f, 1.f), 1.f, 0.f, 0.f, 0.5f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f,
      1.52f)); // ClearGlass
    v.push_back(makeDisneyPrincipled(
      vec3f(1.f, 0.766f, 0.336f), 0.f, 1.f, 0.f, 1.f, 0.2f, 0.f, 0.f, 0.f, 0.f, 0.f,
      0.f, 0.47f)); // Gold
    v.push_back(makeDisneyPrincipled(
      vec3f(0.7f, 0.05f, 0.05f), 0.f, 0.75f, 0.f, 0.6f, 0.3f, 0.9f, 0.f, 0.f, 0.f,
      1.f, 0.95f, 1.5f)); // MetallicRedCarPaint

    v.push_back(makeDisneyPrincipled(
      vec3f(0.82f, 0.67f, 0.16f), 0.f, 0.f, 0.5f, 0.5f, 0.5f, 0.5f, 0.f, 0.f, 0.5f,
      0.f, 0.5f, 1.5f)); // Shell0
    v.push_back(makeDisneyPrincipled(
      vec3f(0.25f, 0.83f, 0.36f), 0.f, 0.8f, 0.5f, 0.5f, 0.5f, 0.5f, 0.f, 0.f, 0.5f,
      1.f, 0.5f, 1.5f)); // Shell1
    v.push_back(makeDisneyPrincipled(
      vec3f(0.75f, 0.83f, 0.46f), 0.f, 0.1f, 0.5f, 0.5f, 0.5f, 0.5f, 0.f, 1.f, 0.5f,
      0.5f, 0.5f, 1.5f)); // Shell2
    v.push_back(makeDisneyPrincipled(
      vec3f(0.75f, 0.83f, 0.46f), 0.5f, 0.1f, 0.f, 1.f, 0.5f, 0.5f, 0.f, 0.f, 0.5f,
      0.f, 0.5f, 1.5f)); // Shell3
    v.push_back(makeDisneyPrincipled(
      vec3f(0.5f, 0.23f, 0.84f), 0.f, 0.5f, 1.f, 1.f, 0.5f, 0.5f, 0.1f, 0.5f, 0.5f,
      0.5f, 0.5f, 1.5f)); // Shell4
    v.push_back(makeDisneyPrincipled(
      vec3f(0.5f, 0.9f, 0.84f), 1.f, 0.f, 1.f, 1.f, 0.1f, 0.5f, 0.1f, 0.5f, 0.5f,
      0.5f, 0.5f, 1.5f)); // Shell5
    v.push_back(makeDisneyPrincipled(
      vec3f(0.9f, 0.9f, 0.84f), 0.f, 1.f, 1.f, 1.f, 0.1f, 0.5f, 0.1f, 0.5f, 0.5f,
      0.5f, 0.5f, 1.5f)); // Shell6
    v.push_back(makeDisneyPrincipled(
      vec3f(0.2f, 0.2f, 0.3f), 0.5f, 0.5f, 0.5f, 0.5f, 0.2f, 0.5f, 0.1f, 0.5f, 0.5f,
      0.5f, 0.5f, 1.5f)); // Shell7
    v.push_back(makeDisneyPrincipled(
      vec3f(0.5f, 0.6f, 0.7f), 0.2f, 0.8f, 0.2f, 0.7f, 0.1f, 0.f, 0.3f, 0.5f, 0.5f,
      0.5f, 0.5f, 1.5f)); // Shell8
    v.push_back(makeDisneyPrincipled(
      vec3f(0.9f, 0.2f, 0.3f), 0.f, 0.f, 0.8f, 0.3f, 0.1f, 0.f, 0.f, 1.f, 0.5f, 1.f,
      0.5f, 1.5f)); // Shell9
    v.push_back(makeDisneyPrincipled(
      vec3f(0.3f, 0.5f, 0.3f), 1.f, 0.9f, 0.8f, 0.3f, 0.2f, 0.f, 0.3f, 1.f, 0.5f,
      1.f, 0.5f, 1.5f)); // Shell10
    v.push_back(makeDisneyPrincipled(
      vec3f(0.1f, 0.1f, 0.3f), 0.5f, 0.5f, 0.5f, 0.3f, 0.9f, 0.f, 0.1f, 0.f, 0.5f,
      0.f, 0.5f, 1.5f)); // Shell11

    return v;
  }

  Scene Scene::makeDisneyPrincipledGalleryScene()
  {
    using owl::vec3f;
    Scene s;

    s.materials.push_back(makeLambertian(vec3f(1.f)));
    s.materials.push_back(makeEmissive(vec3f(10.f)));
    s.materials.push_back(makeConductor(vec3f(0.14f, 0.43f, 1.38f),
                                        vec3f(4.54f, 2.455f, 1.914f),
                                        0.0001f));
    s.materials.push_back(makeConductor(vec3f(0.14f, 0.43f, 1.38f),
                                        vec3f(4.54f, 2.455f, 1.914f),
                                        0.1f));

    std::vector<MaterialGPU> disneyMats = disneyPrincipledPresetsFromCommonMaterials();
    const int32_t kDisneyMatBase = int32_t(s.materials.size());
    for (auto &m : disneyMats)
      s.materials.push_back(std::move(m));

    TriangleMesh floor;
    floor.materialId = 0;
    pushBox(floor, vec3f(-40.f, -0.55f, -40.f), vec3f(40.f, -0.5f, 40.f));
    s.meshes.push_back(std::move(floor));

    const ObjData bunnyObj = loadObjData(std::string(PATHTRACER_ASSET_DIR) + "/bunny.obj");
    const float bunnyScale = 24.f;
    const float bunnyRotY = float(M_PI_2);
    const float baseY = -0.5f;

    constexpr int kNBunnies = 17;
    const float dx = 2.65f;
    const float dz = 3.6f;
    const float xRow0 = -10.6f;
    const float z0 = -2.f;
    for (int i = 0; i < kNBunnies; ++i) {
      const int row = (i < 9) ? 0 : 1;
      const int j = (i < 9) ? i : (i - 9);
      const float x = (row == 0) ? (xRow0 + float(j) * dx)
                                 : (xRow0 + 0.5f * dx + float(j) * dx);
      const float z = (row == 0) ? z0 : (z0 + dz);
      TriangleMesh bunny =
        makeMeshFromObjData(bunnyObj,
                            kDisneyMatBase + i,
                            bunnyScale,
                            bunnyRotY,
                            vec3f(x, baseY, z));
      s.meshes.push_back(std::move(bunny));
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
    s.materials.push_back(makeDisneyPrincipled(
      vec3f(0.7f, 0.05f, 0.05f), // CPU DisneyBSDF::MetallicRedCarPaint
      0.0f,
      0.75f,
      0.0f,
      0.6f,
      0.3f,
      0.9f,
      0.0f,
      0.0f,
      0.0f,
      1.0f,
      0.95f,
      1.5f));

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
      vec3f( 8.0f, baseY, -1.2f),
    };
    const int32_t bunnyMaterials[] = {
      7, // lambertian
      5, // glass
      6, // thin glass
      2, // gold
      3, // rough gold
      4, // silver
      8, // Disney metallic red car paint
    };

    for (int i = 0; i < 7; ++i) {
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

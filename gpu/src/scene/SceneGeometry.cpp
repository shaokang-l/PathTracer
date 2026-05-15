#include "scene/SceneBuilders.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "../../cpu/include/external/tiny_obj_loader.h"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <utility>

namespace mypt {
namespace scene_detail {

  void pushBox(TriangleMesh &mesh,
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

  MaterialGPU makeLambertian(const owl::vec3f &albedo)
  {
    MaterialGPU m = {};
    m.kind = MATERIAL_LAMBERTIAN;
    m.albedo = albedo;
    return m;
  }

  MaterialGPU makeMirror(const owl::vec3f &albedo)
  {
    MaterialGPU m = {};
    m.kind = MATERIAL_MIRROR;
    m.albedo = albedo;
    return m;
  }

  MaterialGPU makeConductor(const owl::vec3f &eta,
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

  MaterialGPU makeDielectric(float ior, float alpha)
  {
    MaterialGPU m = {};
    m.kind = MATERIAL_DIELECTRIC;
    m.ior = ior;
    m.alpha_x = alpha;
    m.alpha_y = alpha;
    return m;
  }

  MaterialGPU makeThinDielectric(float ior)
  {
    MaterialGPU m = {};
    m.kind = MATERIAL_THIN_DIELECTRIC;
    m.ior = ior;
    return m;
  }

  MaterialGPU makeEmissive(const owl::vec3f &emission)
  {
    MaterialGPU m = {};
    m.kind = MATERIAL_EMISSIVE;
    m.emission = emission;
    return m;
  }

  MaterialGPU makeDisneyPrincipled(const owl::vec3f &baseColor,
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

  owl::vec3f normalizeVec(const owl::vec3f &v)
  {
    const float len = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    return len > 0.f ? v * (1.f / len) : owl::vec3f(0.f);
  }

  owl::vec3f rotateY(const owl::vec3f &p, float angle)
  {
    const float c = std::cos(angle);
    const float s = std::sin(angle);
    return owl::vec3f(c * p.x + s * p.z, p.y, -s * p.x + c * p.z);
  }

  TriangleMesh loadObjMesh(const std::string &path,
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

  TriangleMesh loadObjMeshCpuStyle(const std::string &path,
                                   int32_t            materialId,
                                   float              scale,
                                   float              rotateYRadians,
                                   const owl::vec3f  &translate)
  {
    const ObjData data = loadObjData(path);
    return makeMeshFromObjData(data, materialId, scale, rotateYRadians, translate);
  }

  ObjData loadObjData(const std::string &path)
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

  TriangleMesh makeMeshFromObjData(const ObjData      &data,
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

  owl::vec3f cornellCpuToGpu(float cx, float cy, float cz)
  {
    constexpr float kScale = 1.f / 50.f;
    constexpr float ox = 277.5f;
    constexpr float oy = 277.5f;
    constexpr float oz = 277.5f;
    return owl::vec3f((cx - ox) * kScale, (cy - oy) * kScale, (cz - oz) * kScale);
  }

  void appendUvSphere(TriangleMesh &mesh,
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

  std::vector<MaterialGPU> disneyPrincipledPresetsFromCommonMaterials()
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

  void boostDisneyTransmissionForLab(MaterialGPU &m)
  {
    if (m.kind != MATERIAL_DISNEY_PRINCIPLED) return;

    // The CPU presets are mostly metallic/clearcoat/sheen tests. For the lab
    // scene, bias the copied presets toward visible glass so transmission can
    // be inspected without changing the original preset definitions.
    m.metallic = fminf(m.metallic, 0.65f);
    const float currentGlassWeight = (1.f - m.metallic) * m.specularTransmission;
    const float targetGlassWeight = fmaxf(currentGlassWeight, 0.28f);
    m.specularTransmission = fminf(1.f, targetGlassWeight / fmaxf(1e-4f, 1.f - m.metallic));
  }

} // namespace scene_detail
} // namespace mypt

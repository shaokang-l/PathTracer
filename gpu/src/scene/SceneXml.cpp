#include "scene/Scene.h"
#include "pt/scene/mitsuba_xml.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace pt {
namespace {

  struct XmlElement {
    std::map<std::string, std::string> attrs;
    std::string body;
  };

  struct SimpleTransform {
    owl::vec3f translate = owl::vec3f(0.f);
    owl::vec3f scale = owl::vec3f(1.f);
  };

  static std::string readTextFile(const std::string &path)
  {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
      throw std::runtime_error("Failed to open Mitsuba XML: " + path);
    }
    std::ostringstream out;
    out << in.rdbuf();
    return out.str();
  }

  static std::map<std::string, std::string> parseAttributes(const std::string &tag)
  {
    std::map<std::string, std::string> attrs;
    size_t pos = 0;
    while (pos < tag.size()) {
      while (pos < tag.size() &&
             !(std::isalpha(static_cast<unsigned char>(tag[pos])) || tag[pos] == '_')) {
        ++pos;
      }
      const size_t keyBegin = pos;
      while (pos < tag.size() &&
             (std::isalnum(static_cast<unsigned char>(tag[pos])) ||
              tag[pos] == '_' || tag[pos] == '-')) {
        ++pos;
      }
      if (pos == keyBegin) break;

      const std::string key = tag.substr(keyBegin, pos - keyBegin);
      while (pos < tag.size() && std::isspace(static_cast<unsigned char>(tag[pos]))) ++pos;
      if (pos >= tag.size() || tag[pos] != '=') continue;
      ++pos;
      while (pos < tag.size() && std::isspace(static_cast<unsigned char>(tag[pos]))) ++pos;
      if (pos >= tag.size() || tag[pos] != '"') continue;
      ++pos;
      const size_t valueBegin = pos;
      const size_t valueEnd = tag.find('"', pos);
      if (valueEnd == std::string::npos) break;
      attrs[key] = tag.substr(valueBegin, valueEnd - valueBegin);
      pos = valueEnd + 1;
    }
    return attrs;
  }

  static std::vector<XmlElement> findElements(const std::string &xml,
                                              const std::string &name)
  {
    std::vector<XmlElement> elements;
    const std::string openNeedle = "<" + name;
    const std::string closeNeedle = "</" + name + ">";
    size_t pos = 0;
    while (true) {
      const size_t open = xml.find(openNeedle, pos);
      if (open == std::string::npos) break;

      const size_t tagEnd = xml.find('>', open);
      if (tagEnd == std::string::npos) break;
      const std::string openTag = xml.substr(open, tagEnd - open + 1);
      const bool selfClosing = openTag.size() >= 2 && openTag[openTag.size() - 2] == '/';
      if (selfClosing) {
        elements.push_back({parseAttributes(openTag), std::string()});
        pos = tagEnd + 1;
        continue;
      }

      const size_t close = xml.find(closeNeedle, tagEnd + 1);
      if (close == std::string::npos) break;
      elements.push_back({
        parseAttributes(openTag),
        xml.substr(tagEnd + 1, close - tagEnd - 1)
      });
      pos = close + closeNeedle.size();
    }
    return elements;
  }

  static XmlElement findNamedChild(const std::string &body,
                                   const std::string &tagName,
                                   const std::string &propName)
  {
    for (const XmlElement &e : findElements(body, tagName)) {
      const auto it = e.attrs.find("name");
      if (it != e.attrs.end() && it->second == propName) return e;
    }
    return {};
  }

  static std::string attr(const XmlElement &e,
                          const std::string &name,
                          const std::string &fallback = "")
  {
    const auto it = e.attrs.find(name);
    return it == e.attrs.end() ? fallback : it->second;
  }

  static float parseFloat(const std::string &value, float fallback = 0.f)
  {
    if (value.empty()) return fallback;
    return std::stof(value);
  }

  static float floatAttr(const XmlElement &e, const std::string &name, float fallback = 0.f)
  {
    return parseFloat(attr(e, name), fallback);
  }

  static float floatProp(const std::string &body,
                         const std::string &name,
                         float fallback)
  {
    const XmlElement e = findNamedChild(body, "float", name);
    return parseFloat(attr(e, "value"), fallback);
  }

  static int intProp(const std::string &body, const std::string &name, int fallback)
  {
    const XmlElement e = findNamedChild(body, "integer", name);
    const std::string value = attr(e, "value");
    return value.empty() ? fallback : std::stoi(value);
  }

  static owl::vec3f parseRgbValue(const std::string &value, const owl::vec3f &fallback)
  {
    if (value.empty()) return fallback;
    std::string normalized = value;
    std::replace(normalized.begin(), normalized.end(), ',', ' ');
    std::istringstream in(normalized);
    float x = fallback.x;
    float y = fallback.y;
    float z = fallback.z;
    in >> x;
    if (!(in >> y)) y = x;
    if (!(in >> z)) z = y;
    return owl::vec3f(x, y, z);
  }

  static owl::vec3f vec3Attr(const XmlElement &e,
                             const std::string &name,
                             const owl::vec3f &fallback)
  {
    return parseRgbValue(attr(e, name), fallback);
  }

  static owl::vec3f rgbProp(const std::string &body,
                            const std::string &name,
                            const owl::vec3f &fallback)
  {
    const XmlElement e = findNamedChild(body, "rgb", name);
    return parseRgbValue(attr(e, "value"), fallback);
  }

  static MaterialGPU makeLambertianXml(const owl::vec3f &albedo)
  {
    MaterialGPU m = {};
    m.kind = MATERIAL_LAMBERTIAN;
    m.albedo = albedo;
    return m;
  }

  static MaterialGPU makeEmissiveXml(const owl::vec3f &emission)
  {
    MaterialGPU m = {};
    m.kind = MATERIAL_EMISSIVE;
    m.emission = emission;
    return m;
  }

  static MaterialGPU makeDisneyXml(const std::string &body)
  {
    MaterialGPU m = {};
    m.kind = MATERIAL_DISNEY_PRINCIPLED;
    m.baseColor = rgbProp(body, "baseColor", owl::vec3f(1.f));
    m.albedo = m.baseColor;
    m.specularTransmission = floatProp(body, "specularTransmission", 0.f);
    m.metallic = floatProp(body, "metallic", 0.f);
    m.subsurface = floatProp(body, "subsurface", 0.f);
    m.specular = floatProp(body, "specular", 0.5f);
    m.roughness = floatProp(body, "roughness", 0.5f);
    m.specularTint = floatProp(body, "specularTint", 0.f);
    m.anisotropic = floatProp(body, "anisotropic", 0.f);
    m.sheen = floatProp(body, "sheen", 0.f);
    m.sheenTint = floatProp(body, "sheenTint", 0.f);
    m.clearcoat = floatProp(body, "clearcoat", 0.f);
    m.clearcoatGloss = floatProp(body, "clearcoatGloss", 0.f);
    m.ior = floatProp(body, "eta", 1.5f);
    return m;
  }

  static owl::vec3f toOwl(const pt::Vec3f &v)
  {
    return owl::vec3f(v.x, v.y, v.z);
  }

  static MaterialGPU makeMaterialXml(const pt::SceneMaterialDesc &desc)
  {
    MaterialGPU m = {};
    switch (desc.kind) {
    case pt::SceneMaterialKind::Diffuse:
      m.kind = MATERIAL_LAMBERTIAN;
      m.albedo = toOwl(desc.reflectance);
      break;
    case pt::SceneMaterialKind::Conductor:
      m.kind = MATERIAL_CONDUCTOR;
      m.eta = toOwl(desc.eta);
      m.k = toOwl(desc.k);
      m.alpha_x = desc.alpha;
      m.alpha_y = desc.alpha;
      m.albedo = desc.reflectance.x > 0.f || desc.reflectance.y > 0.f || desc.reflectance.z > 0.f
        ? toOwl(desc.reflectance)
        : owl::vec3f(0.8f);
      break;
    case pt::SceneMaterialKind::Dielectric:
      m.kind = MATERIAL_DIELECTRIC;
      m.ior = desc.ior;
      m.alpha_x = desc.alpha;
      m.alpha_y = desc.alpha;
      m.albedo = owl::vec3f(1.f);
      break;
    case pt::SceneMaterialKind::DisneyPrincipled:
      m.kind = MATERIAL_DISNEY_PRINCIPLED;
      m.baseColor = toOwl(desc.baseColor);
      m.albedo = m.baseColor;
      m.specularTransmission = desc.specularTransmission;
      m.metallic = desc.metallic;
      m.subsurface = desc.subsurface;
      m.specular = desc.specular;
      m.roughness = desc.roughness;
      m.specularTint = desc.specularTint;
      m.anisotropic = desc.anisotropic;
      m.sheen = desc.sheen;
      m.sheenTint = desc.sheenTint;
      m.clearcoat = desc.clearcoat;
      m.clearcoatGloss = desc.clearcoatGloss;
      m.ior = desc.ior;
      break;
    case pt::SceneMaterialKind::Emissive:
      m.kind = MATERIAL_EMISSIVE;
      m.emission = toOwl(desc.emission);
      break;
    }
    return m;
  }

  static void pushBoxXml(TriangleMesh &mesh,
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
    for (const vec3f &p : v) mesh.vertices.push_back(p);
    const vec3i tris[12] = {
      { 0, 1, 3 }, { 0, 3, 2 },
      { 4, 6, 7 }, { 4, 7, 5 },
      { 0, 2, 6 }, { 0, 6, 4 },
      { 1, 5, 7 }, { 1, 7, 3 },
      { 2, 3, 7 }, { 2, 7, 6 },
      { 0, 4, 5 }, { 0, 5, 1 },
    };
    for (const vec3i &t : tris)
      mesh.indices.push_back(vec3i(t.x + base, t.y + base, t.z + base));
  }

  static void appendUvSphereXml(TriangleMesh &mesh,
                                const owl::vec3f &center,
                                float radius,
                                int latBands,
                                int lonBands)
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
        const float x = std::cos(phi) * sinT;
        const float y = std::sin(phi) * sinT;
        const float z = cosT;
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

  static int parseObjVertexIndex(const std::string &token)
  {
    const size_t slash = token.find('/');
    const std::string indexText =
      slash == std::string::npos ? token : token.substr(0, slash);
    return std::stoi(indexText) - 1;
  }

  static TriangleMesh loadObjMeshXml(const std::filesystem::path &path, int materialId)
  {
    std::ifstream in(path);
    if (!in) {
      throw std::runtime_error("Failed to open OBJ: " + path.string());
    }

    TriangleMesh mesh;
    mesh.materialId = materialId;

    std::string line;
    while (std::getline(in, line)) {
      std::istringstream ls(line);
      std::string tag;
      ls >> tag;
      if (tag == "v") {
        float x = 0.f;
        float y = 0.f;
        float z = 0.f;
        ls >> x >> y >> z;
        mesh.vertices.push_back(owl::vec3f(x, y, z));
      } else if (tag == "f") {
        std::vector<int> face;
        std::string token;
        while (ls >> token) {
          face.push_back(parseObjVertexIndex(token));
        }
        if (face.size() < 3) continue;
        for (size_t i = 1; i + 1 < face.size(); ++i) {
          mesh.indices.push_back(owl::vec3i(face[0], face[i], face[i + 1]));
        }
      }
    }

    return mesh;
  }

  static void applyTransform(TriangleMesh &mesh, const SimpleTransform &transform)
  {
    for (owl::vec3f &p : mesh.vertices) {
      p = owl::vec3f(p.x * transform.scale.x,
                     p.y * transform.scale.y,
                     p.z * transform.scale.z) + transform.translate;
    }
  }

  static void addQuadLightFromMeshBounds(Scene &scene,
                                         const TriangleMesh &mesh,
                                         const owl::vec3f &emission)
  {
    if (mesh.vertices.empty()) return;

    owl::box3f bounds;
    for (const owl::vec3f &p : mesh.vertices) bounds.extend(p);
    const owl::vec3f lo = bounds.lower;
    const owl::vec3f hi = bounds.upper;
    const owl::vec3f size = bounds.size();

    LightGPU light = {};
    light.kind = LIGHT_QUAD;
    light.emission = emission;

    if (size.x <= size.y && size.x <= size.z) {
      light.v0 = owl::vec3f(lo.x, lo.y, lo.z);
      light.edgeU = owl::vec3f(0.f, size.y, 0.f);
      light.edgeV = owl::vec3f(0.f, 0.f, size.z);
      light.normal = owl::vec3f(size.x >= 0.f ? -1.f : 1.f, 0.f, 0.f);
    } else if (size.y <= size.x && size.y <= size.z) {
      light.v0 = owl::vec3f(lo.x, lo.y, lo.z);
      light.edgeU = owl::vec3f(size.x, 0.f, 0.f);
      light.edgeV = owl::vec3f(0.f, 0.f, size.z);
      light.normal = owl::vec3f(0.f, -1.f, 0.f);
    } else {
      light.v0 = owl::vec3f(lo.x, lo.y, lo.z);
      light.edgeU = owl::vec3f(size.x, 0.f, 0.f);
      light.edgeV = owl::vec3f(0.f, size.y, 0.f);
      light.normal = owl::vec3f(0.f, 0.f, -1.f);
    }
    light.area = owl::length(owl::cross(light.edgeU, light.edgeV));
    if (light.area > 0.f) scene.lights.push_back(light);
  }

  static SimpleTransform parseTransform(const std::string &shapeBody)
  {
    SimpleTransform result;
    const std::vector<XmlElement> transforms = findElements(shapeBody, "transform");
    if (transforms.empty()) return result;

    for (const XmlElement &scale : findElements(transforms.front().body, "scale")) {
      if (!attr(scale, "value").empty()) {
        const float s = floatAttr(scale, "value", 1.f);
        result.scale = result.scale * s;
        continue;
      }

      const bool hasX = !attr(scale, "x").empty();
      const bool hasY = !attr(scale, "y").empty();
      const bool hasZ = !attr(scale, "z").empty();
      const float uniform = hasX ? floatAttr(scale, "x", 1.f) : 1.f;
      result.scale.x *= hasX ? uniform : 1.f;
      result.scale.y *= hasY ? floatAttr(scale, "y", 1.f) : uniform;
      result.scale.z *= hasZ ? floatAttr(scale, "z", 1.f) : uniform;
    }

    for (const XmlElement &translate : findElements(transforms.front().body, "translate")) {
      result.translate.x += floatAttr(translate, "x", 0.f);
      result.translate.y += floatAttr(translate, "y", 0.f);
      result.translate.z += floatAttr(translate, "z", 0.f);
    }

    return result;
  }

  static int addMaterial(Scene &scene, const MaterialGPU &m)
  {
    scene.materials.push_back(m);
    return int(scene.materials.size() - 1);
  }

  static void addMatpreviewShape(Scene &scene,
                                 int shapeIndex,
                                 int materialId,
                                 const SimpleTransform &transform)
  {
    TriangleMesh mesh;
    mesh.materialId = materialId;

    const float maxScale = std::max(transform.scale.x,
                             std::max(transform.scale.y, transform.scale.z));
    if (shapeIndex == 2) {
      appendUvSphereXml(mesh, transform.translate, 0.65f * maxScale, 32, 32);
    } else if (shapeIndex == 1) {
      appendUvSphereXml(mesh,
                        transform.translate + owl::vec3f(0.f, 0.f, 0.02f),
                        0.42f * maxScale,
                        24,
                        24);
    } else {
      const owl::vec3f half(3.5f * transform.scale.x,
                            3.5f * transform.scale.y,
                            0.025f * transform.scale.z);
      pushBoxXml(mesh, transform.translate - half, transform.translate + half);
    }

    scene.meshes.push_back(std::move(mesh));
  }

  static void addEnvLightApprox(Scene &scene, float scale)
  {
    scene.computeBounds();
    const owl::vec3f center = scene.bounds.center();
    const owl::vec3f size = scene.bounds.size();
    const float extent = std::max(4.f, owl::length(size));
    const float y = scene.bounds.upper.y + 0.6f * extent;

    LightGPU light;
    light.kind = LIGHT_QUAD;
    light.emission = owl::vec3f(scale);
    light.v0 = owl::vec3f(center.x - extent, y, center.z - extent);
    light.edgeU = owl::vec3f(2.f * extent, 0.f, 0.f);
    light.edgeV = owl::vec3f(0.f, 0.f, 2.f * extent);
    light.normal = owl::vec3f(0.f, -1.f, 0.f);
    light.area = 4.f * extent * extent;
    scene.lights.push_back(light);
  }

  static void parseCameraXml(const std::string &xml, Scene &scene)
  {
    const std::vector<XmlElement> sensors = findElements(xml, "sensor");
    if (sensors.empty()) return;

    const XmlElement &sensor = sensors.front();
    scene.hasCamera = true;
    scene.cameraFovy = floatProp(sensor.body, "fov", scene.cameraFovy);

    const std::vector<XmlElement> lookAts = findElements(sensor.body, "lookAt");
    if (!lookAts.empty()) {
      scene.cameraFrom = vec3Attr(lookAts.front(), "origin", scene.cameraFrom);
      scene.cameraAt = vec3Attr(lookAts.front(), "target", scene.cameraAt);
      scene.cameraUp = vec3Attr(lookAts.front(), "up", scene.cameraUp);
    }
  }

} // namespace

Scene Scene::loadMitsubaXml(const std::string &path)
{
  const pt::SceneDesc desc = pt::loadMitsubaXmlSceneDesc(path);
  const std::filesystem::path xmlPath(path);
  const std::filesystem::path baseDir =
    xmlPath.has_parent_path() ? xmlPath.parent_path() : std::filesystem::path(".");

  Scene scene;
  std::map<std::string, int> materialIds;

  for (const pt::SceneMaterialDesc &material : desc.materials) {
    if (!material.id.empty()) {
      materialIds[material.id] = addMaterial(scene, makeMaterialXml(material));
    }
  }

  if (!materialIds.count("__planemat")) {
    materialIds["__planemat"] = addMaterial(scene, makeLambertianXml(owl::vec3f(0.3f)));
  }
  if (!materialIds.count("__diffmat")) {
    materialIds["__diffmat"] = addMaterial(scene, makeLambertianXml(owl::vec3f(0.18f)));
  }

  for (const pt::SceneShapeDesc &shape : desc.shapes) {
    int materialId = materialIds.count(shape.materialId)
      ? materialIds[shape.materialId]
      : materialIds["__diffmat"];
    owl::vec3f areaEmission(0.f);
    bool hasAreaEmitter = false;

    for (const pt::SceneMaterialDesc &material : desc.materials) {
      if (material.id == shape.materialId &&
          material.kind == pt::SceneMaterialKind::Emissive) {
        areaEmission = toOwl(material.emission);
        hasAreaEmitter = true;
        break;
      }
    }

    if (shape.kind == pt::SceneShapeKind::Obj) {
      TriangleMesh mesh = loadObjMeshXml(baseDir / shape.filename, materialId);
      SimpleTransform transform;
      transform.translate = toOwl(shape.transform.translate);
      transform.scale = toOwl(shape.transform.scale);
      applyTransform(mesh, transform);
      if (hasAreaEmitter) {
        addQuadLightFromMeshBounds(scene, mesh, areaEmission);
      }
      scene.meshes.push_back(std::move(mesh));
    } else {
      addMatpreviewShape(scene,
                         shape.shapeIndex,
                         materialId,
                         {toOwl(shape.transform.translate), toOwl(shape.transform.scale)});
    }
  }
  if (scene.lights.empty()) {
    addEnvLightApprox(scene, 1.f);
  }

  if (desc.camera.valid) {
    scene.hasCamera = true;
    scene.cameraFrom = toOwl(desc.camera.origin);
    scene.cameraAt = toOwl(desc.camera.target);
    scene.cameraUp = toOwl(desc.camera.up);
    scene.cameraFovy = desc.camera.fov;
  }
  scene.computeBounds();
  std::cout << "[mypt] loaded Mitsuba XML subset: " << path
            << " (" << scene.meshes.size() << " meshes, "
            << scene.materials.size() << " materials)" << std::endl;
  return scene;
}

} // namespace pt


#include "scenes/scene_xml.hpp"

#include "mesh_io/meshLoader.hpp"
#include "pt/scene/mitsuba_xml.h"
#include "pt/scene/scene_desc.h"
#include "render/renderManager.hpp"
#include "utils/objectTransform.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>

namespace {

struct XmlElement {
  std::map<std::string, std::string> attrs;
  std::string body;
};

std::string readTextFile(const std::string &path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    throw std::runtime_error("Failed to open Mitsuba XML: " + path);
  }
  std::ostringstream out;
  out << in.rdbuf();
  return out.str();
}

std::map<std::string, std::string> parseAttributes(const std::string &tag) {
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

std::vector<XmlElement> findElements(const std::string &xml, const std::string &name) {
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

XmlElement findNamedChild(const std::string &body,
                          const std::string &tagName,
                          const std::string &propName) {
  for (const XmlElement &e : findElements(body, tagName)) {
    const auto it = e.attrs.find("name");
    if (it != e.attrs.end() && it->second == propName) return e;
  }
  return {};
}

std::string attr(const XmlElement &e,
                 const std::string &name,
                 const std::string &fallback = "") {
  const auto it = e.attrs.find(name);
  return it == e.attrs.end() ? fallback : it->second;
}

float parseFloat(const std::string &value, float fallback = 0.f) {
  return value.empty() ? fallback : std::stof(value);
}

float floatAttr(const XmlElement &e, const std::string &name, float fallback = 0.f) {
  return parseFloat(attr(e, name), fallback);
}

float floatProp(const std::string &body, const std::string &name, float fallback) {
  const XmlElement e = findNamedChild(body, "float", name);
  return parseFloat(attr(e, "value"), fallback);
}

int intProp(const std::string &body, const std::string &name, int fallback) {
  const XmlElement e = findNamedChild(body, "integer", name);
  const std::string value = attr(e, "value");
  return value.empty() ? fallback : std::stoi(value);
}

pt::Vec3f parseVec3ValueLocal(std::string value, const pt::Vec3f &fallback) {
  if (value.empty()) return fallback;
  std::replace(value.begin(), value.end(), ',', ' ');
  std::istringstream in(value);
  float x = fallback.x;
  float y = fallback.y;
  float z = fallback.z;
  in >> x;
  if (!(in >> y)) y = x;
  if (!(in >> z)) z = y;
  return pt::Vec3f(x, y, z);
}

pt::Vec3f rgbProp(const std::string &body, const std::string &name,
                  const pt::Vec3f &fallback) {
  const XmlElement e = findNamedChild(body, "rgb", name);
  return parseVec3ValueLocal(attr(e, "value"), fallback);
}

pt::Vec3f toWorldAttr(const XmlElement &e, const std::string &name,
                      const pt::Vec3f &fallback) {
  return parseVec3ValueLocal(attr(e, name), fallback);
}

gl::vec3 toGl(const pt::Vec3f &v) {
  return gl::vec3(v.x, v.y, v.z);
}

std::shared_ptr<Material> makeCpuMaterial(const pt::SceneMaterialDesc &m) {
  switch (m.kind) {
  case pt::SceneMaterialKind::Diffuse:
    return std::make_shared<Lambertian>(toGl(m.reflectance));
  case pt::SceneMaterialKind::Conductor:
    return std::make_shared<Conductor>(toGl(m.eta), toGl(m.k), m.alpha, m.alpha);
  case pt::SceneMaterialKind::Dielectric:
    return std::make_shared<MFDielectric>(m.ior, m.alpha, m.alpha);
  case pt::SceneMaterialKind::DisneyPrincipled:
    return std::make_shared<DisneyPrincipledBSDF>(
      toGl(m.baseColor),
      m.specularTransmission,
      m.metallic,
      m.subsurface,
      m.specular,
      m.roughness,
      m.specularTint,
      m.anisotropic,
      m.sheen,
      m.sheenTint,
      m.clearcoat,
      m.clearcoatGloss,
      m.ior);
  case pt::SceneMaterialKind::Emissive:
    return std::make_shared<DiffuseEmitter>(toGl(m.emission), 1.f);
  }
  return gl::DefaultMaterial;
}

pt::SceneTransformDesc parseTransform(const std::string &shapeBody) {
  pt::SceneTransformDesc result;
  const std::vector<XmlElement> transforms = findElements(shapeBody, "transform");
  if (transforms.empty()) return result;

  for (const XmlElement &scale : findElements(transforms.front().body, "scale")) {
    if (!attr(scale, "value").empty()) {
      const float s = floatAttr(scale, "value", 1.f);
      result.scale = pt::Vec3f(result.scale.x * s, result.scale.y * s, result.scale.z * s);
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

void parseMaterial(const XmlElement &bsdf, pt::SceneDesc &desc) {
  pt::SceneMaterialDesc m;
  m.id = attr(bsdf, "id");
  const std::string type = attr(bsdf, "type");

  if (type == "diffuse") {
    m.kind = pt::SceneMaterialKind::Diffuse;
    m.reflectance = rgbProp(bsdf.body, "reflectance", pt::Vec3f(0.8f));
  } else if (type == "conductor") {
    m.kind = pt::SceneMaterialKind::Conductor;
    m.eta = rgbProp(bsdf.body, "eta", pt::Vec3f(0.14f, 0.43f, 1.38f));
    m.k = rgbProp(bsdf.body, "k", pt::Vec3f(4.54f, 2.455f, 1.914f));
    m.alpha = floatProp(bsdf.body, "alpha", 0.0001f);
    m.reflectance = rgbProp(bsdf.body, "specularReflectance", pt::Vec3f(0.8f));
  } else if (type == "dielectric") {
    m.kind = pt::SceneMaterialKind::Dielectric;
    m.ior = floatProp(bsdf.body, "intIOR", 1.5f);
    m.alpha = floatProp(bsdf.body, "alpha", 0.0001f);
  } else if (type == "disneybsdf") {
    m.kind = pt::SceneMaterialKind::DisneyPrincipled;
    m.baseColor = rgbProp(bsdf.body, "baseColor", pt::Vec3f(1.f));
    m.reflectance = m.baseColor;
    m.specularTransmission = floatProp(bsdf.body, "specularTransmission", 0.f);
    m.metallic = floatProp(bsdf.body, "metallic", 0.f);
    m.subsurface = floatProp(bsdf.body, "subsurface", 0.f);
    m.specular = floatProp(bsdf.body, "specular", 0.5f);
    m.roughness = floatProp(bsdf.body, "roughness", 0.5f);
    m.specularTint = floatProp(bsdf.body, "specularTint", 0.f);
    m.anisotropic = floatProp(bsdf.body, "anisotropic", 0.f);
    m.sheen = floatProp(bsdf.body, "sheen", 0.f);
    m.sheenTint = floatProp(bsdf.body, "sheenTint", 0.f);
    m.clearcoat = floatProp(bsdf.body, "clearcoat", 0.f);
    m.clearcoatGloss = floatProp(bsdf.body, "clearcoatGloss", 0.f);
    m.ior = floatProp(bsdf.body, "eta", 1.5f);
  } else {
    std::cerr << "[cpu] Mitsuba XML: unsupported bsdf type '" << type
              << "', using diffuse fallback\n";
    m.kind = pt::SceneMaterialKind::Diffuse;
    m.reflectance = pt::Vec3f(0.8f);
  }

  if (!m.id.empty()) desc.materials.push_back(m);
}

void parseCamera(const std::string &xml, pt::SceneDesc &desc) {
  const std::vector<XmlElement> sensors = findElements(xml, "sensor");
  if (sensors.empty()) return;

  const XmlElement &sensor = sensors.front();
  desc.camera.valid = true;
  desc.camera.fov = floatProp(sensor.body, "fov", 45.f);

  const std::vector<XmlElement> lookAts = findElements(sensor.body, "lookAt");
  if (!lookAts.empty()) {
    desc.camera.origin = toWorldAttr(lookAts.front(), "origin", desc.camera.origin);
    desc.camera.target = toWorldAttr(lookAts.front(), "target", desc.camera.target);
    desc.camera.up = toWorldAttr(lookAts.front(), "up", desc.camera.up);
  }

  const std::vector<XmlElement> films = findElements(sensor.body, "film");
  if (!films.empty()) {
    desc.camera.width = intProp(films.front().body, "width", desc.camera.width);
    desc.camera.height = intProp(films.front().body, "height", desc.camera.height);
  }

  const std::vector<XmlElement> samplers = findElements(sensor.body, "sampler");
  if (!samplers.empty()) {
    desc.camera.sampleCount = intProp(samplers.front().body, "sampleCount",
                                     desc.camera.sampleCount);
  }
}

pt::SceneDesc parseMitsubaXmlDesc(const std::string &path) {
  const std::string xml = readTextFile(path);
  pt::SceneDesc desc;

  for (const XmlElement &bsdf : findElements(xml, "bsdf")) {
    parseMaterial(bsdf, desc);
  }

  int generatedMaterialId = 0;
  for (const XmlElement &shape : findElements(xml, "shape")) {
    pt::SceneShapeDesc s;
    s.id = attr(shape, "id");
    s.transform = parseTransform(shape.body);
    const std::string type = attr(shape, "type");
    if (type == "obj") {
      s.kind = pt::SceneShapeKind::Obj;
      s.filename = attr(findNamedChild(shape.body, "string", "filename"), "value");
    } else if (type == "serialized") {
      s.kind = pt::SceneShapeKind::SerializedPreview;
      s.shapeIndex = intProp(shape.body, "shapeIndex", 0);
    } else {
      std::cerr << "[cpu] Mitsuba XML: skipping unsupported shape type '"
                << type << "'\n";
      continue;
    }

    std::vector<XmlElement> nestedBsdfs = findElements(shape.body, "bsdf");
    if (!nestedBsdfs.empty()) {
      const std::string generatedId = "__nested_" + std::to_string(generatedMaterialId++);
      nestedBsdfs.front().attrs["id"] = generatedId;
      parseMaterial(nestedBsdfs.front(), desc);
      s.materialId = generatedId;
    }

    const std::vector<XmlElement> emitters = findElements(shape.body, "emitter");
    if (!emitters.empty() && attr(emitters.front(), "type") == "area") {
      pt::SceneMaterialDesc m;
      m.id = "__emitter_" + std::to_string(generatedMaterialId++);
      m.kind = pt::SceneMaterialKind::Emissive;
      m.emission = rgbProp(emitters.front().body, "radiance", pt::Vec3f(1.f));
      desc.materials.push_back(m);
      s.materialId = m.id;
    } else if (s.materialId.empty()) {
      const std::vector<XmlElement> refs = findElements(shape.body, "ref");
      if (!refs.empty()) {
        s.materialId = attr(refs.front(), "id");
      }
    }

    desc.shapes.push_back(s);
  }

  parseCamera(xml, desc);
  return desc;
}

std::shared_ptr<Material> findMaterial(const pt::SceneDesc &desc,
                                       const std::string &id) {
  for (const pt::SceneMaterialDesc &m : desc.materials) {
    if (m.id == id) return makeCpuMaterial(m);
  }
  return gl::DefaultMaterial;
}

std::shared_ptr<Hittable> applyTransform(std::shared_ptr<Hittable> object,
                                         const pt::SceneTransformDesc &transform) {
  const float maxScale = std::max(transform.scale.x,
                         std::max(transform.scale.y, transform.scale.z));
  if (std::abs(maxScale - 1.f) > 1e-6f) {
    object = std::make_shared<Scale>(object, maxScale);
  }
  if (std::abs(transform.translate.x) > 1e-6f ||
      std::abs(transform.translate.y) > 1e-6f ||
      std::abs(transform.translate.z) > 1e-6f) {
    object = std::make_shared<Translate>(object, toGl(transform.translate));
  }
  return object;
}

std::shared_ptr<Hittable> makePreviewShape(const pt::SceneShapeDesc &shape,
                                           std::shared_ptr<Material> material) {
  if (shape.shapeIndex == 2) {
    return std::make_shared<Sphere>(toGl(shape.transform.translate), 0.65f, material);
  }
  if (shape.shapeIndex == 1) {
    return std::make_shared<Sphere>(
      toGl(shape.transform.translate + pt::Vec3f(0.f, 0.f, 0.02f)), 0.42f, material);
  }

  const gl::vec3 center = toGl(shape.transform.translate);
  const gl::vec3 half(3.5f * shape.transform.scale.x,
                      3.5f * shape.transform.scale.y,
                      0.025f * shape.transform.scale.z);
  return std::make_shared<Box>(center - half, center + half, material);
}

std::shared_ptr<Hittable> makeLightProxyFromAABB(const AABB &box,
                                                 std::shared_ptr<Material> material) {
  const gl::vec3 lo = box.get_min();
  const gl::vec3 hi = box.get_max();
  const gl::vec3 size = hi - lo;
  if (size.y() <= size.x() && size.y() <= size.z()) {
    return std::make_shared<FlipFace>(
      std::make_shared<XZRectangle>(lo.y(), lo.x(), hi.x(), lo.z(), hi.z(), material));
  }
  if (size.x() <= size.y() && size.x() <= size.z()) {
    return std::make_shared<FlipFace>(
      std::make_shared<YZRectangle>(lo.x(), lo.y(), hi.y(), lo.z(), hi.z(), material));
  }
  return std::make_shared<FlipFace>(
    std::make_shared<XYRectangle>(lo.z(), lo.x(), hi.x(), lo.y(), hi.y(), material));
}

} // namespace

SceneInfo loadMitsubaXmlScene(const std::string &path) {
  const pt::SceneDesc desc = pt::loadMitsubaXmlSceneDesc(path);
  const std::filesystem::path xmlPath(path);
  const std::filesystem::path baseDir =
    xmlPath.has_parent_path() ? xmlPath.parent_path() : std::filesystem::path(".");

  SceneInfo scene;
  ObjectList objects;
  bool hasEmissiveMaterial = false;
  for (const pt::SceneMaterialDesc &material : desc.materials) {
    if (material.kind == pt::SceneMaterialKind::Emissive) {
      hasEmissiveMaterial = true;
      break;
    }
  }

  for (const pt::SceneShapeDesc &shape : desc.shapes) {
    std::shared_ptr<Material> material = findMaterial(desc, shape.materialId);
    std::shared_ptr<Hittable> object;

    if (shape.kind == pt::SceneShapeKind::Obj) {
      const std::filesystem::path objPath = baseDir / shape.filename;
      object = loadOBJMesh(objPath.string(), material);
      object = applyTransform(object, shape.transform);
      if (material && material->is_emitter()) {
        objects.addObject(makeLightProxyFromAABB(object->getAABB(0.f, 1.f), material));
      }
    } else {
      object = makePreviewShape(shape, material);
    }

    if (object) objects.addObject(object);
  }

  if (!hasEmissiveMaterial) {
    auto fallbackLight = std::make_shared<DiffuseEmitter>(gl::vec3(1.f), 1.f);
    objects.addObject(std::make_shared<FlipFace>(
      std::make_shared<XZRectangle>(8.f, -8.f, 8.f, -8.f, 8.f, fallbackLight)));
  }

  scene.objects = objects;
  scene.use_config_defaults = false;
  if (desc.camera.valid) {
    scene._width = static_cast<uint>(desc.camera.width);
    scene._height = static_cast<uint>(desc.camera.height);
    const int spp = std::max(1, desc.camera.sampleCount);
    const int sppSide = std::max(1, static_cast<int>(std::round(std::sqrt(float(spp)))));
    scene.spp_x = static_cast<uint>(sppSide);
    scene.spp_y = static_cast<uint>(sppSide);
    const gl::vec3 origin = toGl(desc.camera.origin);
    const gl::vec3 target = toGl(desc.camera.target);
    scene.camera = std::make_shared<PerspectiveCamera>(
      gl::to_radian(desc.camera.fov),
      float(scene._width) / float(scene._height),
      10.f,
      1000.f,
      toGl(desc.camera.up),
      (target - origin).normalize(),
      origin);
  }
  if (!scene.camera) {
    scene.camera = std::make_shared<PerspectiveCamera>(
      gl::to_radian(45.f), 1.f, 10.f, 1000.f, gl::vec3(0, 1, 0),
      gl::vec3(0, 0, 1), gl::vec3(0, 0, -10));
  }
  scene.bg_color = gl::vec3(0.f);
  scene.global_medium = nullptr;

  std::cout << "[cpu] loaded Mitsuba XML subset: " << path
            << " (" << desc.shapes.size() << " shapes, "
            << desc.materials.size() << " materials)" << std::endl;
  return scene;
}

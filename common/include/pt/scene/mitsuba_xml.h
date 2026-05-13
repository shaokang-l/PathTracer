#pragma once

#include "scene_desc.h"
#include "render_settings.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace pt {

namespace detail {

struct XmlElement {
  std::map<std::string, std::string> attrs;
  std::string body;
};

inline std::string readTextFile(const std::string &path)
{
  std::ifstream in(path, std::ios::binary);
  if (!in) throw std::runtime_error("Failed to open Mitsuba XML: " + path);
  std::ostringstream out;
  out << in.rdbuf();
  return out.str();
}

inline std::map<std::string, std::string> parseAttributes(const std::string &tag)
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

inline std::vector<XmlElement> findElements(const std::string &xml, const std::string &name)
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
    elements.push_back({parseAttributes(openTag), xml.substr(tagEnd + 1, close - tagEnd - 1)});
    pos = close + closeNeedle.size();
  }
  return elements;
}

inline XmlElement findNamedChild(const std::string &body,
                                 const std::string &tagName,
                                 const std::string &propName)
{
  for (const XmlElement &e : findElements(body, tagName)) {
    const auto it = e.attrs.find("name");
    if (it != e.attrs.end() && it->second == propName) return e;
  }
  return {};
}

inline std::string attr(const XmlElement &e,
                        const std::string &name,
                        const std::string &fallback = "")
{
  const auto it = e.attrs.find(name);
  return it == e.attrs.end() ? fallback : it->second;
}

inline float parseFloat(const std::string &value, float fallback = 0.f)
{
  return value.empty() ? fallback : std::stof(value);
}

inline float floatAttr(const XmlElement &e, const std::string &name, float fallback = 0.f)
{
  return parseFloat(attr(e, name), fallback);
}

inline float floatProp(const std::string &body, const std::string &name, float fallback)
{
  const XmlElement e = findNamedChild(body, "float", name);
  return parseFloat(attr(e, "value"), fallback);
}

inline int intProp(const std::string &body, const std::string &name, int fallback)
{
  const XmlElement e = findNamedChild(body, "integer", name);
  const std::string value = attr(e, "value");
  return value.empty() ? fallback : std::stoi(value);
}

inline Vec3f rgbProp(const std::string &body, const std::string &name, const Vec3f &fallback)
{
  const XmlElement e = findNamedChild(body, "rgb", name);
  return parseVec3Value(attr(e, "value"), fallback);
}

inline Vec3f vec3Attr(const XmlElement &e, const std::string &name, const Vec3f &fallback)
{
  return parseVec3Value(attr(e, name), fallback);
}

inline void parseMaterial(const XmlElement &bsdf, SceneDesc &desc)
{
  SceneMaterialDesc m;
  m.id = attr(bsdf, "id");
  const std::string type = attr(bsdf, "type");

  if (type == "diffuse") {
    m.kind = SceneMaterialKind::Diffuse;
    m.reflectance = rgbProp(bsdf.body, "reflectance", Vec3f(0.8f));
  } else if (type == "conductor") {
    m.kind = SceneMaterialKind::Conductor;
    m.eta = rgbProp(bsdf.body, "eta", Vec3f(0.14f, 0.43f, 1.38f));
    m.k = rgbProp(bsdf.body, "k", Vec3f(4.54f, 2.455f, 1.914f));
    m.alpha = floatProp(bsdf.body, "alpha", 0.0001f);
    m.reflectance = rgbProp(bsdf.body, "specularReflectance", Vec3f(0.8f));
  } else if (type == "dielectric") {
    m.kind = SceneMaterialKind::Dielectric;
    m.ior = floatProp(bsdf.body, "intIOR", 1.5f);
    m.alpha = floatProp(bsdf.body, "alpha", 0.0001f);
  } else if (type == "disneybsdf") {
    m.kind = SceneMaterialKind::DisneyPrincipled;
    m.baseColor = rgbProp(bsdf.body, "baseColor", Vec3f(1.f));
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
    std::cerr << "[common] Mitsuba XML: unsupported bsdf type '" << type
              << "', using diffuse fallback\n";
    m.kind = SceneMaterialKind::Diffuse;
    m.reflectance = Vec3f(0.8f);
  }

  if (!m.id.empty()) desc.materials.push_back(m);
}

inline SceneTransformDesc parseTransform(const std::string &shapeBody)
{
  SceneTransformDesc result;
  const std::vector<XmlElement> transforms = findElements(shapeBody, "transform");
  if (transforms.empty()) return result;

  for (const XmlElement &scale : findElements(transforms.front().body, "scale")) {
    if (!attr(scale, "value").empty()) {
      const float s = floatAttr(scale, "value", 1.f);
      result.scale = Vec3f(result.scale.x * s, result.scale.y * s, result.scale.z * s);
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

inline void parseCamera(const std::string &xml, SceneDesc &desc)
{
  const std::vector<XmlElement> sensors = findElements(xml, "sensor");
  if (sensors.empty()) return;

  const XmlElement &sensor = sensors.front();
  desc.camera.valid = true;
  desc.camera.fov = floatProp(sensor.body, "fov", 45.f);

  const std::vector<XmlElement> lookAts = findElements(sensor.body, "lookAt");
  if (!lookAts.empty()) {
    desc.camera.origin = vec3Attr(lookAts.front(), "origin", desc.camera.origin);
    desc.camera.target = vec3Attr(lookAts.front(), "target", desc.camera.target);
    desc.camera.up = vec3Attr(lookAts.front(), "up", desc.camera.up);
  }

  const std::vector<XmlElement> films = findElements(sensor.body, "film");
  if (!films.empty()) {
    desc.camera.width = intProp(films.front().body, "width", desc.camera.width);
    desc.camera.height = intProp(films.front().body, "height", desc.camera.height);
  }

  const std::vector<XmlElement> samplers = findElements(sensor.body, "sampler");
  if (!samplers.empty()) {
    desc.camera.sampleCount = intProp(samplers.front().body, "sampleCount", desc.camera.sampleCount);
  }
}

} // namespace detail

inline SceneDesc loadMitsubaXmlSceneDesc(const std::string &path)
{
  const std::string xml = detail::readTextFile(path);
  SceneDesc desc;

  for (const detail::XmlElement &bsdf : detail::findElements(xml, "bsdf")) {
    detail::parseMaterial(bsdf, desc);
  }

  int generatedMaterialId = 0;
  for (const detail::XmlElement &shape : detail::findElements(xml, "shape")) {
    SceneShapeDesc s;
    s.id = detail::attr(shape, "id");
    s.transform = detail::parseTransform(shape.body);
    const std::string type = detail::attr(shape, "type");
    if (type == "obj") {
      s.kind = SceneShapeKind::Obj;
      s.filename = detail::attr(detail::findNamedChild(shape.body, "string", "filename"), "value");
    } else if (type == "serialized") {
      s.kind = SceneShapeKind::SerializedPreview;
      s.shapeIndex = detail::intProp(shape.body, "shapeIndex", 0);
    } else {
      std::cerr << "[common] Mitsuba XML: skipping unsupported shape type '"
                << type << "'\n";
      continue;
    }

    std::vector<detail::XmlElement> nestedBsdfs = detail::findElements(shape.body, "bsdf");
    if (!nestedBsdfs.empty()) {
      const std::string generatedId = "__nested_" + std::to_string(generatedMaterialId++);
      nestedBsdfs.front().attrs["id"] = generatedId;
      detail::parseMaterial(nestedBsdfs.front(), desc);
      s.materialId = generatedId;
    }

    const std::vector<detail::XmlElement> emitters = detail::findElements(shape.body, "emitter");
    if (!emitters.empty() && detail::attr(emitters.front(), "type") == "area") {
      SceneMaterialDesc m;
      m.id = "__emitter_" + std::to_string(generatedMaterialId++);
      m.kind = SceneMaterialKind::Emissive;
      m.emission = detail::rgbProp(emitters.front().body, "radiance", Vec3f(1.f));
      desc.materials.push_back(m);
      s.materialId = m.id;
    } else if (s.materialId.empty()) {
      const std::vector<detail::XmlElement> refs = detail::findElements(shape.body, "ref");
      if (!refs.empty()) s.materialId = detail::attr(refs.front(), "id");
    }

    desc.shapes.push_back(s);
  }

  detail::parseCamera(xml, desc);
  return desc;
}

} // namespace pt

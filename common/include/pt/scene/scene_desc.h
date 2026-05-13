#pragma once

#include "../math/vec.h"

#include <string>
#include <vector>

namespace pt {

enum class SceneMaterialKind {
  Diffuse,
  Conductor,
  Dielectric,
  DisneyPrincipled,
  Emissive,
};

struct SceneMaterialDesc {
  std::string id;
  SceneMaterialKind kind = SceneMaterialKind::Diffuse;

  Vec3f reflectance = Vec3f(0.8f);
  Vec3f eta = Vec3f(1.5f);
  Vec3f k = Vec3f(0.f);
  float alpha = 0.0001f;
  float ior = 1.5f;
  Vec3f emission = Vec3f(0.f);

  Vec3f baseColor = Vec3f(1.f);
  float specularTransmission = 0.f;
  float metallic = 0.f;
  float subsurface = 0.f;
  float specular = 0.5f;
  float roughness = 0.5f;
  float specularTint = 0.f;
  float anisotropic = 0.f;
  float sheen = 0.f;
  float sheenTint = 0.f;
  float clearcoat = 0.f;
  float clearcoatGloss = 0.f;
};

struct SceneTransformDesc {
  Vec3f translate = Vec3f(0.f);
  Vec3f scale = Vec3f(1.f);
};

enum class SceneShapeKind {
  Obj,
  SerializedPreview,
};

struct SceneShapeDesc {
  SceneShapeKind kind = SceneShapeKind::Obj;
  std::string id;
  std::string filename;
  std::string materialId;
  int shapeIndex = 0;
  SceneTransformDesc transform;
};

struct SceneCameraDesc {
  bool valid = false;
  Vec3f origin = Vec3f(0.f, 0.f, 1.f);
  Vec3f target = Vec3f(0.f);
  Vec3f up = Vec3f(0.f, 1.f, 0.f);
  float fov = 45.f;
  int width = 512;
  int height = 512;
  int sampleCount = 16;
};

struct SceneDesc {
  std::vector<SceneMaterialDesc> materials;
  std::vector<SceneShapeDesc> shapes;
  SceneCameraDesc camera;
};

} // namespace pt

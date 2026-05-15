#pragma once

#include "../math/vec.h"
#include "../render/debug_view_kind.h"
#include "../render/direct_light_mode.h"

#include <algorithm>
#include <cstdlib>
#include <sstream>
#include <string>
#include <string_view>

namespace pt {

enum class ToneMapKind {
  Clamp,
  Reinhard,
};

struct RenderSettings {
  int width = 64;
  int height = 64;
  int spp = 1;
  int maxDepth = 4;
  float gamma = 2.2f;
  ToneMapKind toneMap = ToneMapKind::Clamp;
  DebugViewKind debugView = DebugViewKind::Beauty;
  DirectLightMode directLightMode = DirectLightMode::Nee;
  int restirInitialCandidates = 1;
  Vec3f background = Vec3f(0.f);

  bool hasCameraOverride = false;
  Vec3f cameraOrigin = Vec3f(0.f, 0.f, -18.f);
  Vec3f cameraTarget = Vec3f(0.f);
  Vec3f cameraUp = Vec3f(0.f, 1.f, 0.f);
  float fov = 45.f;
};

inline bool hasArg(int argc, char **argv, std::string_view name)
{
  for (int i = 1; i < argc; ++i) {
    if (std::string_view(argv[i]) == name) return true;
  }
  return false;
}

inline int parseIntArg(int argc, char **argv, std::string_view name, int fallback)
{
  for (int i = 1; i + 1 < argc; ++i) {
    if (std::string_view(argv[i]) == name) return std::atoi(argv[i + 1]);
  }
  return fallback;
}

inline float parseFloatArg(int argc, char **argv, std::string_view name, float fallback)
{
  for (int i = 1; i + 1 < argc; ++i) {
    if (std::string_view(argv[i]) == name) return std::strtof(argv[i + 1], nullptr);
  }
  return fallback;
}

inline std::string_view parseStringArg(int argc,
                                       char **argv,
                                       std::string_view name,
                                       std::string_view fallback = {})
{
  for (int i = 1; i + 1 < argc; ++i) {
    if (std::string_view(argv[i]) == name) return argv[i + 1];
  }
  return fallback;
}

inline Vec3f parseVec3Value(std::string value, const Vec3f &fallback)
{
  if (value.empty()) return fallback;
  std::replace(value.begin(), value.end(), ',', ' ');
  std::istringstream in(value);
  float x = fallback.x;
  float y = fallback.y;
  float z = fallback.z;
  in >> x;
  if (!(in >> y)) y = x;
  if (!(in >> z)) z = y;
  return Vec3f(x, y, z);
}

inline Vec3f parseVec3Arg(int argc, char **argv, std::string_view name, const Vec3f &fallback)
{
  for (int i = 1; i + 1 < argc; ++i) {
    if (std::string_view(argv[i]) == name) return parseVec3Value(argv[i + 1], fallback);
  }
  return fallback;
}

inline RenderSettings parseRenderSettings(int argc, char **argv, const RenderSettings &defaults = {})
{
  RenderSettings settings = defaults;
  settings.width = parseIntArg(argc, argv, "--width", settings.width);
  settings.height = parseIntArg(argc, argv, "--height", settings.height);
  settings.spp = parseIntArg(argc, argv, "--spp", settings.spp);
  settings.maxDepth = parseIntArg(argc, argv, "--max-depth", settings.maxDepth);
  settings.gamma = parseFloatArg(argc, argv, "--gamma", settings.gamma);
  settings.background = parseVec3Arg(argc, argv, "--background", settings.background);

  const std::string_view toneMap = parseStringArg(argc, argv, "--tonemap", "clamp");
  settings.toneMap = toneMap == "reinhard" ? ToneMapKind::Reinhard : ToneMapKind::Clamp;

  const std::string_view debugView = parseStringArg(argc, argv, "--debug-view", "beauty");
  if (debugView == "normal") {
    settings.debugView = DebugViewKind::Normal;
  } else if (debugView == "albedo") {
    settings.debugView = DebugViewKind::Albedo;
  } else if (debugView == "visibility") {
    settings.debugView = DebugViewKind::Visibility;
  } else if (debugView == "material-id" || debugView == "material_id") {
    settings.debugView = DebugViewKind::MaterialId;
  } else if (debugView == "light-id" || debugView == "light_id") {
    settings.debugView = DebugViewKind::LightId;
  } else if (debugView == "reservoir-weight" || debugView == "reservoir_weight") {
    settings.debugView = DebugViewKind::ReservoirWeight;
  } else if (debugView == "reservoir-m" || debugView == "reservoir_m") {
    settings.debugView = DebugViewKind::ReservoirM;
  } else if (debugView == "reservoir-target" || debugView == "reservoir_target") {
    settings.debugView = DebugViewKind::ReservoirTarget;
  } else if (debugView == "restir-light-id" || debugView == "restir_light_id") {
    settings.debugView = DebugViewKind::RestirLightId;
  } else {
    settings.debugView = DebugViewKind::Beauty;
  }

  const std::string_view directLight =
    parseStringArg(argc, argv, "--direct-light", "nee");
  settings.directLightMode =
    directLight == "restir" ? DirectLightMode::Restir : DirectLightMode::Nee;
  settings.restirInitialCandidates =
    parseIntArg(argc,
                argv,
                "--restir-initial-candidates",
                settings.restirInitialCandidates);
  settings.restirInitialCandidates = std::max(1, settings.restirInitialCandidates);

  settings.hasCameraOverride =
    hasArg(argc, argv, "--camera-origin") || hasArg(argc, argv, "--camera-target");
  if (settings.hasCameraOverride) {
    settings.cameraOrigin = parseVec3Arg(argc, argv, "--camera-origin", settings.cameraOrigin);
    settings.cameraTarget = parseVec3Arg(argc, argv, "--camera-target", settings.cameraTarget);
    settings.cameraUp = parseVec3Arg(argc, argv, "--camera-up", settings.cameraUp);
    settings.fov = parseFloatArg(argc, argv, "--fov", settings.fov);
  }

  return settings;
}

} // namespace pt

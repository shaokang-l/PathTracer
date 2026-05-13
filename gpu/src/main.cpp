// ======================================================================== //
// main.cpp - entry point. Intentionally tiny: the interesting code
// lives in Renderer / Scene / Viewer.
// ======================================================================== //

#include "Renderer.h"
#include "Scene.h"
#include "SceneExport.h"
#include "Viewer.h"

#include <algorithm>
#include <cstdlib>
#include <cuda_runtime.h>
#include <iostream>
#include <string>
#include <string_view>
#include <sstream>
#include <vector>

#include "external/stb_image_write.h"

namespace {
  // Parse `--frames N` from argv. Returns -1 when not specified, meaning
  // "run interactively forever". Used by profilers (nsys/ncu) to make the
  // app self-terminate after a deterministic number of accumulated frames.
  int parseFramesArg(int argc, char **argv) {
    for (int i = 1; i + 1 < argc; ++i) {
      if (std::string_view(argv[i]) == "--frames") {
        return std::atoi(argv[i + 1]);
      }
    }
    return -1;
  }

  int parseIntArg(int argc, char **argv, std::string_view name, int fallback)
  {
    for (int i = 1; i + 1 < argc; ++i) {
      if (std::string_view(argv[i]) == name) {
        return std::atoi(argv[i + 1]);
      }
    }
    return fallback;
  }

  float parseFloatArg(int argc, char **argv, std::string_view name, float fallback)
  {
    for (int i = 1; i + 1 < argc; ++i) {
      if (std::string_view(argv[i]) == name) {
        return std::strtof(argv[i + 1], nullptr);
      }
    }
    return fallback;
  }

  std::string_view parseStringArg(int argc,
                                  char **argv,
                                  std::string_view name,
                                  std::string_view fallback = {})
  {
    for (int i = 1; i + 1 < argc; ++i) {
      if (std::string_view(argv[i]) == name) {
        return argv[i + 1];
      }
    }
    return fallback;
  }

  bool hasFlag(int argc, char **argv, std::string_view name)
  {
    for (int i = 1; i < argc; ++i) {
      if (std::string_view(argv[i]) == name) return true;
    }
    return false;
  }

  owl::vec3f parseVec3Arg(int argc,
                          char **argv,
                          std::string_view name,
                          const owl::vec3f &fallback)
  {
    for (int i = 1; i + 1 < argc; ++i) {
      if (std::string_view(argv[i]) == name) {
        std::string value(argv[i + 1]);
        std::replace(value.begin(), value.end(), ',', ' ');
        std::istringstream in(value);
        float x = fallback.x;
        float y = fallback.y;
        float z = fallback.z;
        in >> x;
        if (!(in >> y)) y = x;
        if (!(in >> z)) z = y;
        return owl::vec3f(x, y, z);
      }
    }
    return fallback;
  }

  std::string_view parseExportSceneXmlArg(int argc, char **argv)
  {
    for (int i = 1; i + 1 < argc; ++i) {
      if (std::string_view(argv[i]) == "--export-scene-xml") {
        return argv[i + 1];
      }
    }
    return {};
  }

  mypt::Scene loadSceneFromArgs(int argc, char **argv)
  {
    for (int i = 1; i + 1 < argc; ++i) {
      if (std::string_view(argv[i]) == "--scene-xml") {
        return mypt::Scene::loadMitsubaXml(argv[i + 1]);
      }
      if (std::string_view(argv[i]) == "--scene") {
        const std::string_view name(argv[i + 1]);
        if (name == "disney-cornell") return mypt::Scene::makeDisneyCornellScene();
        if (name == "disney-gallery")
          return mypt::Scene::makeDisneyPrincipledGalleryScene();
        if (name == "disney-lab") return mypt::Scene::makeDisneyMaterialLabScene();
      }
    }
    return mypt::Scene::makeDisneyMaterialLabScene();
  }

  bool saveDeviceFrameBuffer(const std::string &path,
                             const uint32_t *deviceFb,
                             int width,
                             int height)
  {
    std::vector<uint32_t> pixels(size_t(width) * size_t(height));
    const cudaError_t err = cudaMemcpy(pixels.data(),
                                       deviceFb,
                                       pixels.size() * sizeof(uint32_t),
                                       cudaMemcpyDeviceToHost);
    if (err != cudaSuccess) {
      std::cerr << "[mypt] failed to copy framebuffer: "
                << cudaGetErrorString(err) << std::endl;
      return false;
    }

    stbi_flip_vertically_on_write(1);
    const int ok = stbi_write_png(path.c_str(), width, height, 4,
                                  pixels.data(), width * 4);
    std::cout << "[mypt] wrote framebuffer: " << path
              << " (" << width << "x" << height << ")" << std::endl;
    return ok != 0;
  }

  void setDefaultCamera(mypt::Renderer &renderer,
                        int argc,
                        char **argv,
                        const mypt::Scene &scene,
                        float fovyDegrees = 45.f)
  {
    if (hasFlag(argc, argv, "--camera-origin") ||
        hasFlag(argc, argv, "--camera-target")) {
      const owl::vec3f origin =
        parseVec3Arg(argc, argv, "--camera-origin", owl::vec3f(0.f, 0.f, -18.f));
      const owl::vec3f target =
        parseVec3Arg(argc, argv, "--camera-target", owl::vec3f(0.f));
      const owl::vec3f up =
        parseVec3Arg(argc, argv, "--camera-up", owl::vec3f(0.f, 1.f, 0.f));
      const float fov = parseFloatArg(argc, argv, "--fov", fovyDegrees);
      renderer.setCamera(origin, target, up, fov);
      return;
    }

    if (scene.hasCamera) {
      renderer.setCamera(scene.cameraFrom,
                         scene.cameraAt,
                         scene.cameraUp,
                         scene.cameraFovy);
      return;
    }

    const owl::box3f &sceneBounds = scene.bounds;
    const owl::vec3f center = sceneBounds.center();
    const float radius = owl::length(sceneBounds.size()) * 0.5f;
    const owl::vec3f from = center + owl::vec3f(0.f, radius * 0.5f, radius * 1.8f);
    renderer.setCamera(from, center, owl::vec3f(0.f, 1.f, 0.f), fovyDegrees);
  }
}

int main(int argc, char **argv)
{
  std::cout << "[mypt] starting up" << std::endl;

  int maxFrames = parseFramesArg(argc, argv);
  const std::string_view outputPath = parseStringArg(argc, argv, "--output");
  if (maxFrames < 0 && !outputPath.empty()) {
    maxFrames = 1;
  }

  mypt::Scene scene = loadSceneFromArgs(argc, argv);
  const std::string_view exportXmlPath = parseExportSceneXmlArg(argc, argv);
  if (!exportXmlPath.empty()) {
    mypt::exportMitsubaXmlScene(scene, std::string(exportXmlPath));
    std::cout << "[mypt] exported scene XML: " << exportXmlPath << std::endl;
    return 0;
  }

  std::cout << "[mypt] scene: " << scene.meshes.size()
            << " meshes, bounds = ["
            << scene.bounds.lower << " .. " << scene.bounds.upper << "]"
            << std::endl;

  mypt::Renderer renderer;
  renderer.setSamplesPerPixel(parseIntArg(argc, argv, "--spp", 1));
  renderer.setMaxBounces(parseIntArg(argc, argv, "--max-depth", 8));
  renderer.setMissColor(parseVec3Arg(argc, argv, "--miss-color", owl::vec3f(0.f)));
  const std::string_view tonemap = parseStringArg(argc, argv, "--tonemap", "reinhard");
  renderer.setOutputTransform(parseFloatArg(argc, argv, "--gamma", 2.2f),
                              tonemap != "clamp");
  renderer.setScene(scene);

  const int width = parseIntArg(argc, argv, "--width", 1280);
  const int height = parseIntArg(argc, argv, "--height", 720);
  const bool visible = !hasFlag(argc, argv, "--headless");
  if (!visible && !outputPath.empty()) {
    uint32_t *deviceFb = nullptr;
    const size_t bytes = size_t(width) * size_t(height) * sizeof(uint32_t);
    cudaError_t err = cudaMalloc(reinterpret_cast<void **>(&deviceFb), bytes);
    if (err != cudaSuccess) {
      std::cerr << "[mypt] failed to allocate framebuffer: "
                << cudaGetErrorString(err) << std::endl;
      return 2;
    }

    renderer.resize(deviceFb, owl::vec2i(width, height));
    setDefaultCamera(renderer, argc, argv, scene);
    const int frames = std::max(1, maxFrames);
    for (int i = 0; i < frames; ++i) {
      renderer.render();
    }

    const bool ok = saveDeviceFrameBuffer(std::string(outputPath),
                                          deviceFb,
                                          width,
                                          height);
    cudaFree(deviceFb);
    return ok ? 0 : 2;
  }

  mypt::Viewer viewer(renderer, scene.bounds, owl::vec2i(width, height), visible);
  viewer.enableFlyMode();
  viewer.enableInspectMode(owl::box3f(scene.bounds.lower, scene.bounds.upper));

  if (maxFrames > 0) {
    std::cout << "[mypt] benchmark mode: exiting after "
              << maxFrames << " frames" << std::endl;
    viewer.showAndRun([&] { return renderer.accumID() < maxFrames; });
  } else {
    viewer.showAndRun();
  }

  if (!outputPath.empty() && !viewer.saveFrameBuffer(std::string(outputPath))) {
    return 2;
  }

  return 0;
}

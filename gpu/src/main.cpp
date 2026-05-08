// ======================================================================== //
// main.cpp - entry point. Intentionally tiny: the interesting code
// lives in Renderer / Scene / Viewer.
// ======================================================================== //

#include "Renderer.h"
#include "Scene.h"
#include "Viewer.h"

#include <cstdlib>
#include <iostream>
#include <string_view>

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
}

int main(int argc, char **argv)
{
  std::cout << "[mypt] starting up" << std::endl;

  const int maxFrames = parseFramesArg(argc, argv);

  mypt::Scene scene = mypt::Scene::makeTestScene();
  std::cout << "[mypt] scene: " << scene.meshes.size()
            << " meshes, bounds = ["
            << scene.bounds.lower << " .. " << scene.bounds.upper << "]"
            << std::endl;

  mypt::Renderer renderer;
  renderer.setSamplesPerPixel(4);
  renderer.setMaxBounces(8);
  renderer.setScene(scene);

  mypt::Viewer viewer(renderer, scene.bounds);
  viewer.enableFlyMode();
  viewer.enableInspectMode(owl::box3f(scene.bounds.lower, scene.bounds.upper));

  if (maxFrames > 0) {
    std::cout << "[mypt] benchmark mode: exiting after "
              << maxFrames << " frames" << std::endl;
    viewer.showAndRun([&] { return renderer.accumID() < maxFrames; });
  } else {
    viewer.showAndRun();
  }

  return 0;
}

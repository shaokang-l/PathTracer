// ======================================================================== //
// main.cpp - entry point. Intentionally tiny: the interesting code
// lives in Renderer / Scene / Viewer.
// ======================================================================== //

#include "Renderer.h"
#include "Scene.h"
#include "Viewer.h"

#include <iostream>

int main(int /*argc*/, char ** /*argv*/)
{
  std::cout << "[mypt] starting up" << std::endl;

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
  viewer.showAndRun();

  return 0;
}

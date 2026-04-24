#include "Viewer.h"

#include <iostream>

namespace mypt {

  Viewer::Viewer(Renderer &renderer, const owl::box3f &sceneBounds)
    : owl::viewer::OWLViewer("MyPT - OWL Path Tracer Scaffold",
                             owl::vec2i(1280, 720),
                             /*visible=*/true,
                             /*enableVsync=*/true),
      renderer_(renderer)
  {
    const owl::vec3f center = sceneBounds.center();
    const float      radius = length(sceneBounds.size()) * 0.5f;
    const owl::vec3f from   = center + owl::vec3f(0.f, radius * 0.5f, radius * 1.8f);

    setCameraOrientation(from, center, owl::vec3f(0.f, 1.f, 0.f), 45.f);
    setWorldScale(length(sceneBounds.size()));
    enableInspectMode(owl::box3f(sceneBounds.lower, sceneBounds.upper));
  }

  void Viewer::resize(const owl::vec2i &newSize)
  {
    OWLViewer::resize(newSize);
    renderer_.resize(fbPointer, newSize);
    cameraChanged();
  }

  void Viewer::cameraChanged()
  {
    renderer_.setCamera(camera.getFrom(),
                        camera.getAt(),
                        camera.getUp(),
                        camera.getFovyInDegrees());
  }

  void Viewer::render()
  {
    renderer_.render();
  }

  void Viewer::key(char k, const owl::vec2i &where)
  {
    switch (k) {
    case '+': case '=':
      std::cerr << "[mypt] (key) not yet bound to spp/bounces" << std::endl;
      break;
    default:
      OWLViewer::key(k, where);
    }
  }

} // namespace mypt

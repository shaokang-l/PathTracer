#include "Viewer.h"

#include "external/stb_image_write.h"

#include <cuda_runtime.h>
#include <iostream>
#include <vector>

namespace {

  const char *debugViewName(pt::DebugViewKind view)
  {
    switch (view) {
    case pt::DebugViewKind::Beauty: return "beauty";
    case pt::DebugViewKind::Normal: return "normal";
    case pt::DebugViewKind::Albedo: return "albedo";
    case pt::DebugViewKind::Visibility: return "visibility";
    case pt::DebugViewKind::MaterialId: return "material-id";
    case pt::DebugViewKind::LightId: return "light-id";
    default: return "unknown";
    }
  }

  bool debugViewFromKey(char key, pt::DebugViewKind &view)
  {
    switch (key) {
    case '1': view = pt::DebugViewKind::Beauty; return true;
    case '2': view = pt::DebugViewKind::Normal; return true;
    case '3': view = pt::DebugViewKind::Albedo; return true;
    case '4': view = pt::DebugViewKind::Visibility; return true;
    case '5': view = pt::DebugViewKind::MaterialId; return true;
    case '6': view = pt::DebugViewKind::LightId; return true;
    default: return false;
    }
  }

} // namespace

namespace mypt {

  Viewer::Viewer(Renderer &renderer,
                 const owl::box3f &sceneBounds,
                 const owl::vec2i &initialSize,
                 bool visible)
    : owl::viewer::OWLViewer("MyPT - OWL Path Tracer Scaffold",
                             initialSize,
                             visible,
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
    currentSize_ = newSize;
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
    pt::DebugViewKind debugView = pt::DebugViewKind::Beauty;
    if (debugViewFromKey(k, debugView)) {
      renderer_.setDebugView(debugView);
      std::cout << "[mypt] debug view: " << debugViewName(debugView) << std::endl;
      return;
    }

    switch (k) {
    case '+': case '=':
      std::cerr << "[mypt] (key) not yet bound to spp/bounces" << std::endl;
      break;
    case 'r':
      renderer_.updateMaterialBuffer();
      break;
    case 'o':
      renderer_.restoreOriginalMaterials();
      break;
    default:
      OWLViewer::key(k, where);
    }
  }

  bool Viewer::saveFrameBuffer(const std::string &path) const
  {
    if (!fbPointer || currentSize_.x <= 0 || currentSize_.y <= 0) {
      std::cerr << "[mypt] cannot save framebuffer before resize/render" << std::endl;
      return false;
    }

    std::vector<uint32_t> pixels(size_t(currentSize_.x) * size_t(currentSize_.y));
    const size_t bytes = pixels.size() * sizeof(uint32_t);
    cudaError_t err = cudaMemcpy(pixels.data(), fbPointer, bytes, cudaMemcpyDeviceToHost);
    if (err != cudaSuccess) {
      std::cerr << "[mypt] failed to copy framebuffer: "
                << cudaGetErrorString(err) << std::endl;
      return false;
    }

    stbi_flip_vertically_on_write(1);
    const int ok = stbi_write_png(path.c_str(),
                                  currentSize_.x,
                                  currentSize_.y,
                                  4,
                                  pixels.data(),
                                  currentSize_.x * 4);
    std::cout << "[mypt] wrote framebuffer: " << path << std::endl;
    return ok != 0;
  }

} // namespace mypt

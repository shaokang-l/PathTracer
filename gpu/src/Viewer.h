// ======================================================================== //
// Viewer.h - subclass of owl::viewer::OWLViewer that drives our Renderer.
//
// Responsibilities:
//   - open a GLFW window + manage a CUDA/GL-shared framebuffer texture
//   - route camera/resize events to Renderer
//   - call Renderer::render() each frame
// ======================================================================== //

#pragma once

#include "Renderer.h"
#include "owlViewer/OWLViewer.h"

namespace mypt {

  class Viewer : public owl::viewer::OWLViewer {
  public:
    Viewer(Renderer &renderer, const owl::box3f &sceneBounds);

    void render()        override;
    void resize(const owl::vec2i &newSize) override;
    void cameraChanged() override;
    void key(char key, const owl::vec2i &where) override;

  private:
    Renderer &renderer_;
  };

} // namespace mypt

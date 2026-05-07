// ======================================================================== //
// Scene.h - pure host-side scene description.
//
// This is where you plug your existing OOP scene into OWL. Keep it
// library-agnostic: no OWL handles live here, only POD data. The
// Renderer takes a Scene and builds all the OWL buffers / geoms /
// groups from it.
//
// When you port your path tracer:
//   - map your Mesh / Shape objects -> TriangleMesh below
//   - map your Material hierarchy   -> MaterialGPU in material.h
//     (you'll probably want a small translator function in Scene.cpp
//      that flattens your polymorphic Material* into MaterialGPU)
// ======================================================================== //

#pragma once

#include "light.h"
#include "material.h"

#include <owl/common/math/vec.h>
#include <owl/common/math/box.h>
#include <vector>

namespace mypt {

  struct TriangleMesh {
    std::vector<owl::vec3f> vertices;
    std::vector<owl::vec3i> indices;
    int32_t                 materialId;
  };

  struct Scene {
    std::vector<TriangleMesh> meshes;
    // global material buffer, use materialId to index into this buffer
    std::vector<MaterialGPU> materials;
    std::vector<LightGPU>    lights;

    // Optional: world-space bounds, used to set a sensible camera speed.
    owl::box3f bounds;

    void computeBounds();

    // Convenience: load a built-in test scene (Cornell-box-ish).
    static Scene makeTestScene();
  };

} // namespace mypt

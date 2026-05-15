// ======================================================================== //
// Scene.h - pure host-side scene description.
//
// This is where you plug your existing OOP scene into OWL. Keep it
// library-agnostic: no OWL handles live here, only POD data. The
// Renderer takes a Scene and builds all the OWL buffers / geoms /
// groups from it.
// ======================================================================== //

#pragma once

#include "pod/light.h"
#include "pod/material.h"

#include <owl/common/math/vec.h>
#include <owl/common/math/box.h>
#include <string>
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

    bool hasCamera = false;
    owl::vec3f cameraFrom = owl::vec3f(0.f, 5.f, 18.f);
    owl::vec3f cameraAt = owl::vec3f(0.f);
    owl::vec3f cameraUp = owl::vec3f(0.f, 1.f, 0.f);
    float cameraFovy = 45.f;

    void computeBounds();

    // Convenience: load a built-in test scene (Cornell-box-ish).
    static Scene makeTestScene();

    // Disney Principled showcase matching CPU `cornell_box_DisneyPrincipledBSDF`
    // (walls + 12 spheres with Shell0..Shell11 presets).
    static Scene makeDisneyCornellScene();

    // Same layout as `makeTestScene()` (floor / light / gold backdrops), but every
    // bunny uses a Disney Principled preset from `gl::DisneyBSDF` in
    // `cpu/include/material/commonMaterials.hpp` (all Principled entries: presets +
    // Shell0..Shell11).
    static Scene makeDisneyPrincipledGalleryScene();

    // Studio material lab / turntable-style Disney showcase: all Disney
    // Principled presets on small pedestals, arranged in a shallow arc under
    // large softbox-style area lights.
    static Scene makeDisneyMaterialLabScene();

    // Minimal Mitsuba XML subset loader. The first supported target is
    // assets/disney_bsdf_array.xml: disneybsdf/diffuse materials, serialized
    // matpreview shapes approximated with simple GPU meshes, and envmap
    // emitters approximated with a large quad light.
    static Scene loadMitsubaXml(const std::string &path);
  };

} // namespace mypt

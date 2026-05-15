// ======================================================================== //
// SceneBuilders.h - internal helpers for built-in GPU scenes.
// ======================================================================== //

#pragma once

#include "scene/Scene.h"

#include "../../cpu/include/external/tiny_obj_loader.h"

#include <string>
#include <vector>

#ifndef PATHTRACER_ASSET_DIR
#define PATHTRACER_ASSET_DIR "assets"
#endif

namespace pt {
namespace scene_detail {

  enum class ObjUpAxis {
    Y,
    Z,
  };

  struct ObjData {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
  };

  void pushBox(TriangleMesh &mesh,
               const owl::vec3f &lo,
               const owl::vec3f &hi);

  MaterialGPU makeLambertian(const owl::vec3f &albedo);
  MaterialGPU makeMirror(const owl::vec3f &albedo);
  MaterialGPU makeConductor(const owl::vec3f &eta,
                            const owl::vec3f &k,
                            float             alpha);
  MaterialGPU makeDielectric(float ior, float alpha);
  MaterialGPU makeThinDielectric(float ior);
  MaterialGPU makeEmissive(const owl::vec3f &emission);
  MaterialGPU makeDisneyPrincipled(const owl::vec3f &baseColor,
                                   float             specularTransmission,
                                   float             metallic,
                                   float             subsurface,
                                   float             specular,
                                   float             roughness,
                                   float             specularTint,
                                   float             anisotropic,
                                   float             sheen,
                                   float             sheenTint,
                                   float             clearcoat,
                                   float             clearcoatGloss,
                                   float             eta);

  owl::vec3f normalizeVec(const owl::vec3f &v);
  owl::vec3f rotateY(const owl::vec3f &p, float angle);
  owl::vec3f cornellCpuToGpu(float cx, float cy, float cz);

  TriangleMesh loadObjMesh(const std::string &path,
                           int32_t            materialId,
                           float              targetHeight,
                           const owl::vec3f  &baseCenter,
                           ObjUpAxis          sourceUpAxis);
  TriangleMesh loadObjMeshCpuStyle(const std::string &path,
                                   int32_t            materialId,
                                   float              scale,
                                   float              rotateYRadians,
                                   const owl::vec3f  &translate);
  ObjData loadObjData(const std::string &path);
  TriangleMesh makeMeshFromObjData(const ObjData      &data,
                                   int32_t             materialId,
                                   float               scale,
                                   float               rotateYRadians,
                                   const owl::vec3f   &translate);
  void appendUvSphere(TriangleMesh &mesh,
                      const owl::vec3f &center,
                      float             radius,
                      int               latBands,
                      int               lonBands);

  std::vector<MaterialGPU> disneyPrincipledPresetsFromCommonMaterials();
  void boostDisneyTransmissionForLab(MaterialGPU &m);

} // namespace scene_detail
} // namespace pt

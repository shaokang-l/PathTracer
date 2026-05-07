#include "Renderer.h"

#include "geometryData.h"
#include "launchParams.h"

#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>

extern "C" char deviceCode_ptx[];

namespace mypt {

  using owl::vec2i;
  using owl::vec3f;
  using owl::vec3i;

  Renderer::Renderer()
  {
    ctx_    = owlContextCreate(nullptr, 1);
    module_ = owlModuleCreate(ctx_, deviceCode_ptx);
    buildPrograms();
  }

  Renderer::~Renderer()
  {
    if (ctx_) owlContextDestroy(ctx_);
  }

  void Renderer::buildPrograms()
  {
    OWLVarDecl triMeshVars[] = {
      { "vertex",   OWL_BUFPTR,                OWL_OFFSETOF(TriangleMeshSBT, vertex)   },
      { "index",    OWL_BUFPTR,                OWL_OFFSETOF(TriangleMeshSBT, index)    },
      { "materialId", OWL_INT,                 OWL_OFFSETOF(TriangleMeshSBT, materialId) },
      {}
    };
    triMeshType_ = owlGeomTypeCreate(ctx_,
                                     OWL_TRIANGLES,
                                     sizeof(TriangleMeshSBT),
                                     triMeshVars, -1);
    owlGeomTypeSetClosestHit(triMeshType_, 0, module_, "TriangleMesh");

    OWLVarDecl missVars[] = {
      { "skyColorTop",    OWL_FLOAT3, OWL_OFFSETOF(MissProgData, skyColorTop)    },
      { "skyColorBottom", OWL_FLOAT3, OWL_OFFSETOF(MissProgData, skyColorBottom) },
      {}
    };
    missProg_ = owlMissProgCreate(ctx_, module_, "miss",
                                  sizeof(MissProgData),
                                  missVars, -1);
    owlMissProgSet3f(missProg_, "skyColorTop",    owl3f{ 0.55f, 0.75f, 1.0f });
    owlMissProgSet3f(missProg_, "skyColorBottom", owl3f{ 1.00f, 1.00f, 1.0f });

    OWLVarDecl rgVars[] = { {} };
    rayGen_ = owlRayGenCreate(ctx_, module_, "rayGen",
                              sizeof(RayGenData), rgVars, -1);

    OWLVarDecl lpVars[] = {
      { "accumBuffer",    OWL_RAW_POINTER, OWL_OFFSETOF(LaunchParams, accumBuffer)    },
      { "fbPtr",          OWL_RAW_POINTER, OWL_OFFSETOF(LaunchParams, fbPtr)          },
      { "fbSize",         OWL_INT2,        OWL_OFFSETOF(LaunchParams, fbSize)         },
      { "materials",      OWL_RAW_POINTER, OWL_OFFSETOF(LaunchParams, materials)      },
      { "lights",         OWL_RAW_POINTER, OWL_OFFSETOF(LaunchParams, lights)         },
      { "lightCount",     OWL_INT,         OWL_OFFSETOF(LaunchParams, lightCount)     },
      { "accumID",        OWL_INT,         OWL_OFFSETOF(LaunchParams, accumID)        },
      { "samplesPerPixel",OWL_INT,         OWL_OFFSETOF(LaunchParams, samplesPerPixel)},
      { "maxBounces",     OWL_INT,         OWL_OFFSETOF(LaunchParams, maxBounces)     },
      { "world",          OWL_GROUP,       OWL_OFFSETOF(LaunchParams, world)          },
      { "camera.pos",     OWL_FLOAT3,      OWL_OFFSETOF(LaunchParams, camera.pos)     },
      { "camera.dir_00",  OWL_FLOAT3,      OWL_OFFSETOF(LaunchParams, camera.dir_00)  },
      { "camera.dir_du",  OWL_FLOAT3,      OWL_OFFSETOF(LaunchParams, camera.dir_du)  },
      { "camera.dir_dv",  OWL_FLOAT3,      OWL_OFFSETOF(LaunchParams, camera.dir_dv)  },
      {}
    };
    lp_ = owlParamsCreate(ctx_, sizeof(LaunchParams), lpVars, -1);
  }

  void Renderer::setScene(const Scene &scene)
  {
    vertexBufs_.clear();
    indexBufs_.clear();
    geoms_.clear();
    vertexBufs_.reserve(scene.meshes.size());
    indexBufs_.reserve(scene.meshes.size());
    geoms_.reserve(scene.meshes.size());


    for (const auto &m : scene.meshes) {
      OWLBuffer vb = owlDeviceBufferCreate(ctx_, OWL_FLOAT3,
                                           m.vertices.size(),
                                           m.vertices.data());
      OWLBuffer ib = owlDeviceBufferCreate(ctx_, OWL_INT3,
                                           m.indices.size(),
                                           m.indices.data());

      vertexBufs_.push_back(vb);
      indexBufs_.push_back(ib);

      OWLGeom g = owlGeomCreate(ctx_, triMeshType_);
      owlTrianglesSetVertices(g, vb, m.vertices.size(), sizeof(vec3f), 0);
      owlTrianglesSetIndices (g, ib, m.indices.size(),  sizeof(vec3i), 0);
      owlGeomSetBuffer(g, "vertex", vb);
      owlGeomSetBuffer(g, "index",  ib);
      owlGeomSet1i(g, "materialId", m.materialId);
      geoms_.push_back(g);
    }

    // upload material buffer
    materialBuffer_ = owlDeviceBufferCreate(ctx_, OWL_USER_TYPE(MaterialGPU),
                                           scene.materials.size(),
                                           scene.materials.data());
    originalMaterials_ = scene.materials;

    lightCount_ = int(scene.lights.size());
    lightBuffer_ = lightCount_ > 0
      ? owlDeviceBufferCreate(ctx_, OWL_USER_TYPE(LightGPU),
                              scene.lights.size(),
                              scene.lights.data())
      : nullptr;

    buildAccel(scene);

    owlParamsSetGroup(lp_, "world", world_);

    owlBuildPrograms(ctx_);
    owlBuildPipeline(ctx_);
    owlBuildSBT(ctx_);
    hasScene_ = true;
  }

  void Renderer::buildAccel(const Scene &scene)
  {
    (void)scene;
    blas_  = owlTrianglesGeomGroupCreate(ctx_, (int)geoms_.size(), geoms_.data());
    owlGroupBuildAccel(blas_);
    world_ = owlInstanceGroupCreate(ctx_, 1, &blas_);
    owlGroupBuildAccel(world_);
  }

  void Renderer::resize(uint32_t *fbPtr, const vec2i &fbSize)
  {
    fbPtr_  = fbPtr;
    fbSize_ = fbSize;

    if (accumBuffer_) owlBufferRelease(accumBuffer_);
    accumBuffer_ = owlDeviceBufferCreate(ctx_, OWL_FLOAT4,
                                         fbSize.x * fbSize.y, nullptr);
    resetAccum();
  }

  void Renderer::updateMaterialBuffer()
  {
    const size_t count = owlBufferSizeInBytes(materialBuffer_) / sizeof(MaterialGPU);
    std::vector<MaterialGPU> newMats(count);
    for (auto &m : newMats) {
        m.kind   = MATERIAL_LAMBERTIAN;
        m.albedo = vec3f(1.0f, 0.0f, 0.0f);
    }
    owlBufferUpload(materialBuffer_, newMats.data());
    resetAccum();
  }

  void Renderer::restoreOriginalMaterials()
  {
    owlBufferUpload(materialBuffer_, originalMaterials_.data());
    resetAccum();
  }

  void Renderer::setCamera(const vec3f &from, const vec3f &at,
                           const vec3f &up,   float fovyDeg)
  {
    const float aspect = (fbSize_.y > 0)
      ? float(fbSize_.x) / float(fbSize_.y) : 1.f;
    const float fovyRad   = fovyDeg * float(M_PI) / 180.f;
    const float halfH     = std::tan(0.5f * fovyRad);
    const float halfW     = aspect * halfH;

    cam_.pos    = from;
    vec3f w     = normalize(at - from);
    vec3f u     = normalize(cross(w, up));
    vec3f v     = cross(u, w);
    cam_.dir_du = 2.f * halfW * u;
    cam_.dir_dv = 2.f * halfH * v;
    cam_.dir_00 = w - halfW * u - halfH * v;

    resetAccum();
  }

  void Renderer::resetAccum() { accumID_ = 0; }

  void Renderer::updateLaunchParams()
  {
    owlParamsSet1ul(lp_, "accumBuffer",
                    (uint64_t)(accumBuffer_
                      ? owlBufferGetPointer(accumBuffer_, 0)
                      : 0));
    owlParamsSet1ul(lp_, "fbPtr", (uint64_t)fbPtr_);
    owlParamsSet2i (lp_, "fbSize", fbSize_.x, fbSize_.y);
    owlParamsSet1ul(lp_, "materials",
      (uint64_t)(materialBuffer_
        ? owlBufferGetPointer(materialBuffer_, 0)
        : 0));
    owlParamsSet1ul(lp_, "lights",
      (uint64_t)(lightBuffer_
        ? owlBufferGetPointer(lightBuffer_, 0)
        : 0));
    owlParamsSet1i (lp_, "lightCount", lightCount_);
    owlParamsSet1i (lp_, "accumID", accumID_);
    owlParamsSet1i (lp_, "samplesPerPixel", samplesPerPixel_);
    owlParamsSet1i (lp_, "maxBounces", maxBounces_);
    owlParamsSet3f (lp_, "camera.pos",    owl3f{ cam_.pos.x,    cam_.pos.y,    cam_.pos.z    });
    owlParamsSet3f (lp_, "camera.dir_00", owl3f{ cam_.dir_00.x, cam_.dir_00.y, cam_.dir_00.z });
    owlParamsSet3f (lp_, "camera.dir_du", owl3f{ cam_.dir_du.x, cam_.dir_du.y, cam_.dir_du.z });
    owlParamsSet3f (lp_, "camera.dir_dv", owl3f{ cam_.dir_dv.x, cam_.dir_dv.y, cam_.dir_dv.z });
  }

  void Renderer::render()
  {
    if (!hasScene_ || !fbPtr_ || fbSize_.x <= 0 || fbSize_.y <= 0)
      return;

    updateLaunchParams();
    owlLaunch2D(rayGen_, fbSize_.x, fbSize_.y, lp_);
    ++accumID_;
  }

} // namespace mypt

// ======================================================================== //
// Renderer.h - host-side wrapper that owns ALL OWL handles.
//
// Everything OWL-specific lives behind this interface. The rest of
// the app (Viewer, main, whatever host systems you add later) only
// talks to Renderer via plain C++ types.
//
// Lifetime:
//   1. Construct renderer                -> context, module, programs
//   2. renderer.setScene(scene)          -> buffers, geoms, BLAS, IAS, SBT
//   3. renderer.resize(fbPtr, size)      -> (re)alloc accum buffer
//   4. renderer.setCamera(...)           -> updates launch params, resets accum
//   5. renderer.render()                 -> one progressive launch
// ======================================================================== //

#pragma once

#include "Denoiser.h"
#include "scene/Scene.h"
#include "pt/scene/render_settings.h"

#include <owl/owl.h>
#include <vector>

namespace mypt {

  class Renderer {
  public:
    Renderer();
    ~Renderer();

    Renderer(const Renderer &)            = delete;
    Renderer &operator=(const Renderer &) = delete;

    /*! Upload the scene, build BLAS/IAS, create programs/pipeline/SBT.
        Call exactly once for now. Dynamic scene reload would need to
        release existing groups before re-uploading. */
    void setScene(const Scene &scene);

    /*! Pointer comes from OWLViewer's CUDA/GL interop-owned texture;
        do NOT free it here. Reallocates our HDR accumulator. */
    void resize(uint32_t *fbPtr, const owl::vec2i &fbSize);

    /*! Camera in the "lookFrom / lookAt / up / fovY-degrees" form.
        Precomputes ray frame and uploads via launch params. Also
        resets the accumulator. */
    void setCamera(const owl::vec3f &from,
                   const owl::vec3f &at,
                   const owl::vec3f &up,
                   float             fovyDegrees);

    /*! One progressive frame. Increments internal accumID. */
    void render();

    /*! Sampling knobs. Safe to call any time; changes take effect
        starting with the next launch. Does not reset accumulation. */
    void setSamplesPerPixel(int spp) { samplesPerPixel_ = spp; }
    void setMaxBounces(int b)        { maxBounces_      = b;   }
    void setMissColor(const owl::vec3f &color);
    void setDebugView(pt::DebugViewKind view);
    void setDirectLightMode(pt::DirectLightMode mode,
                            int restirInitialCandidates,
                            bool restirTemporal,
                            int restirMaxHistory);
    void setOutputTransform(float gamma, bool useReinhard)
    {
      gamma_ = gamma;
      useReinhardTonemap_ = useReinhard;
    }

    int  accumID() const { return accumID_; }

    // Ex. 01: update the material buffer, so that all materials are lambertian red
    // This function is called when the user presses the 'r' key in the viewer
    // This illustrates the concept of "updating the material buffer" on the fly, 
    // without having to rebuild the SBT (i.e. bindless materials)
    void updateMaterialBuffer();
    void restoreOriginalMaterials();

  private:
    void buildAccel(const Scene &scene);
    void buildPrograms();
    void updateLaunchParams();
    void resetAccum();


    OWLContext       ctx_           = nullptr;
    OWLModule        module_        = nullptr;

    OWLGeomType      triMeshType_   = nullptr;
    OWLRayGen        rayGen_        = nullptr;
    OWLMissProg      missProg_      = nullptr;
    OWLMissProg      shadowMissProg_ = nullptr;
    OWLLaunchParams  lp_            = nullptr;

    // profiling variables
    cudaEvent_t eventStart_  = nullptr;
    cudaEvent_t eventEnd_    = nullptr;
    float       emaMs_       = 0.f;   // exponentially-smoothed launch time (ms)
    int         frameCount_  = 0;     // total render() calls; used for periodic logging

    std::vector<OWLBuffer> vertexBufs_;
    std::vector<OWLBuffer> indexBufs_;
    std::vector<OWLGeom>   geoms_;

    std::vector<MaterialGPU> originalMaterials_; // store the original materials, so that we can restore them later

    OWLGroup         blas_          = nullptr;
    OWLGroup         world_         = nullptr;

    OWLBuffer        accumBuffer_   = nullptr;
    OWLBuffer        materialBuffer_ = nullptr;
    OWLBuffer        lightBuffer_    = nullptr;
    OWLBuffer        restirReservoirBuffer_ = nullptr;
    OWLBuffer        restirSurfaceBuffer_ = nullptr;
    Denoiser         denoiser_;
    bool             denoiserEnabled_ = true;
    int              denoiserMinAccumulatedSpp_ = 32;
    int              denoiserInterval_ = 8;
    int              lightCount_     = 0;

    owl::vec2i       fbSize_        = { 0, 0 };
    uint32_t        *fbPtr_         = nullptr;
    int              accumID_       = 0;
    int              samplesPerPixel_ = 1;
    int              maxBounces_      = 8;
    pt::DebugViewKind debugView_      = pt::DebugViewKind::Beauty;
    pt::DirectLightMode directLightMode_ = pt::DirectLightMode::Nee;
    int              restirInitialCandidates_ = 1;
    bool             restirTemporal_ = false;
    int              restirMaxHistory_ = 20;
    owl::vec3f       missColor_       = owl::vec3f(0.f);
    float            gamma_           = 2.2f;
    bool             useReinhardTonemap_ = true;

    struct CamFrame {
      owl::vec3f pos;
      owl::vec3f dir_00;
      owl::vec3f dir_du;
      owl::vec3f dir_dv;
    } cam_{};

    bool             hasScene_      = false;
  };

} // namespace mypt

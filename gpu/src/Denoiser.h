// ======================================================================== //
// Denoiser.h - thin host-side wrapper around the OptiX AI denoiser.
//
// This class owns only denoiser resources. The renderer still owns the
// path-traced HDR accumulator and the display framebuffer.
// ======================================================================== //

#pragma once

#include <cstdint>
#include <cuda_runtime.h>

struct OptixDeviceContext_t;
using OptixDeviceContext = OptixDeviceContext_t *;

struct OptixDenoiser_t;
using OptixDenoiser = OptixDenoiser_t *;

namespace mypt {

  class Denoiser {
  public:
    Denoiser();
    ~Denoiser();

    Denoiser(const Denoiser &)            = delete;
    Denoiser &operator=(const Denoiser &) = delete;

    /*! Allocate/reallocate denoiser resources for the current framebuffer. */
    void resize(int width, int height, cudaStream_t stream);

    /*! Run color-only HDR denoising. Returns the denoised linear-HDR output.
        If the denoiser has not been sized yet, returns hdrIn unchanged. */
    const float4 *denoise(const float4 *hdrIn,
                          int           width,
                          int           height,
                          cudaStream_t  stream);

    const float4 *output() const { return output_; }
    bool valid() const { return denoiser_ != nullptr && output_ != nullptr; }

  private:
    void init();
    void release();
    void releaseImageResources();

    OptixDeviceContext optixContext_ = nullptr;
    OptixDenoiser      denoiser_     = nullptr;

    float4            *output_       = nullptr;
    void              *state_        = nullptr;
    void              *scratch_      = nullptr;

    uint32_t           width_        = 0;
    uint32_t           height_       = 0;
    size_t             stateBytes_   = 0;
    size_t             scratchBytes_ = 0;
  };

} // namespace mypt

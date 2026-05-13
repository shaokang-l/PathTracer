// ======================================================================== //
// Denoiser.cpp - OptiX denoiser host API setup/invoke.
// ======================================================================== //

#include "Denoiser.h"

#include <cuda.h>
#include <optix.h>
#include <optix_function_table_definition.h>
#include <optix_stubs.h>

#include <stdexcept>
#include <string>

namespace mypt {
namespace {

  void checkCuda(cudaError_t result, const char *where)
  {
    if (result != cudaSuccess) {
      throw std::runtime_error(std::string(where) + ": "
        + cudaGetErrorString(result));
    }
  }

  void checkOptix(OptixResult result, const char *where)
  {
    if (result != OPTIX_SUCCESS) {
      throw std::runtime_error(std::string(where) + ": OptiX error "
        + std::to_string(static_cast<int>(result)));
    }
  }

  OptixImage2D makeImage(const float4 *pixels, uint32_t width, uint32_t height)
  {
    OptixImage2D image = {};
    image.data               = reinterpret_cast<CUdeviceptr>(pixels);
    image.width              = width;
    image.height             = height;
    image.rowStrideInBytes   = width * sizeof(float4);
    image.pixelStrideInBytes = sizeof(float4);
    image.format             = OPTIX_PIXEL_FORMAT_FLOAT4;
    return image;
  }

} // namespace

  Denoiser::Denoiser()
  {
  }

  Denoiser::~Denoiser()
  {
    release();
  }

  void Denoiser::init()
  {
    if (denoiser_) return;

    checkOptix(optixInit(), "optixInit");

    CUcontext current = nullptr;
    cuCtxGetCurrent(&current);
    checkOptix(optixDeviceContextCreate(current, nullptr, &optixContext_),
               "optixDeviceContextCreate");

    OptixDenoiserOptions options = {};
    options.guideAlbedo = 0;
    options.guideNormal = 0;

    checkOptix(optixDenoiserCreate(optixContext_,
                                   OPTIX_DENOISER_MODEL_KIND_HDR,
                                   &options,
                                   &denoiser_),
               "optixDenoiserCreate");
  }

  void Denoiser::releaseImageResources()
  {
    if (scratch_) {
      cudaFree(scratch_);
      scratch_ = nullptr;
    }
    if (state_) {
      cudaFree(state_);
      state_ = nullptr;
    }
    if (output_) {
      cudaFree(output_);
      output_ = nullptr;
    }

    width_ = 0;
    height_ = 0;
    stateBytes_ = 0;
    scratchBytes_ = 0;
  }

  void Denoiser::release()
  {
    releaseImageResources();

    if (denoiser_) {
      optixDenoiserDestroy(denoiser_);
      denoiser_ = nullptr;
    }
    if (optixContext_) {
      optixDeviceContextDestroy(optixContext_);
      optixContext_ = nullptr;
    }
  }

  void Denoiser::resize(int width, int height, cudaStream_t stream)
  {
    if (width <= 0 || height <= 0) {
      releaseImageResources();
      return;
    }

    init();

    const uint32_t newWidth  = static_cast<uint32_t>(width);
    const uint32_t newHeight = static_cast<uint32_t>(height);
    if (newWidth == width_ && newHeight == height_ && valid()) return;

    releaseImageResources();

    OptixDenoiserSizes sizes = {};
    checkOptix(optixDenoiserComputeMemoryResources(denoiser_,
                                                   newWidth,
                                                   newHeight,
                                                   &sizes),
               "optixDenoiserComputeMemoryResources");

    width_ = newWidth;
    height_ = newHeight;
    stateBytes_ = sizes.stateSizeInBytes;
    scratchBytes_ = sizes.withoutOverlapScratchSizeInBytes;

    checkCuda(cudaMalloc(reinterpret_cast<void **>(&output_),
                         width_ * height_ * sizeof(float4)),
              "cudaMalloc denoiser output");
    checkCuda(cudaMalloc(&state_, stateBytes_), "cudaMalloc denoiser state");
    checkCuda(cudaMalloc(&scratch_, scratchBytes_), "cudaMalloc denoiser scratch");

    checkOptix(optixDenoiserSetup(denoiser_,
                                  stream,
                                  width_,
                                  height_,
                                  reinterpret_cast<CUdeviceptr>(state_),
                                  stateBytes_,
                                  reinterpret_cast<CUdeviceptr>(scratch_),
                                  scratchBytes_),
               "optixDenoiserSetup");
  }

  const float4 *Denoiser::denoise(const float4 *hdrIn,
                                  int           width,
                                  int           height,
                                  cudaStream_t  stream)
  {
    if (!hdrIn || width <= 0 || height <= 0) return hdrIn;

    resize(width, height, stream);
    if (!valid()) return hdrIn;

    OptixDenoiserParams params = {};
    params.blendFactor = 0.f;

    OptixDenoiserLayer layer = {};
    layer.input  = makeImage(hdrIn, width_, height_);
    layer.output = makeImage(output_, width_, height_);

    OptixDenoiserGuideLayer guideLayer = {};

    checkOptix(optixDenoiserInvoke(denoiser_,
                                   stream,
                                   &params,
                                   reinterpret_cast<CUdeviceptr>(state_),
                                   stateBytes_,
                                   &guideLayer,
                                   &layer,
                                   1,
                                   0,
                                   0,
                                   reinterpret_cast<CUdeviceptr>(scratch_),
                                   scratchBytes_),
               "optixDenoiserInvoke");

    return output_;
  }

} // namespace mypt

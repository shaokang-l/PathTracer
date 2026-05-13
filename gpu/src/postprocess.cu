// ======================================================================== //
// postprocess.cu - regular CUDA kernels that run after the OptiX raygen.
//
// This file is intentionally NOT in the embed_ptx() set: it compiles as
// ordinary CUDA device code and links directly into the mypt executable.
// Keep OptiX programs in deviceCode.cu and post-process kernels here.
//
// The current single post-process step is HDR tone-map + gamma + RGBA8
// pack, previously inlined at the tail of OPTIX_RAYGEN_PROGRAM(rayGen).
// Pulling it out gives us a clean linear-HDR accumulator handoff that
// the next step (OptiX denoiser) can sit between.
// ======================================================================== //

#include "postprocess.h"

#include <cuda_runtime.h>

namespace mypt {

  // ----------------------------------------------------------------------
  // Pack a [0,1] RGB triple into an RGBA8 word laid out the same way
  // owl::make_rgba does, so the visible result through OWLViewer's
  // CUDA/GL interop is bit-identical to the previous raygen output.
  // ----------------------------------------------------------------------
  __device__ inline uint32_t pack_rgba8(float r, float g, float b)
  {
    int ri = min(255, max(0, int(r * 256.f)));
    int gi = min(255, max(0, int(g * 256.f)));
    int bi = min(255, max(0, int(b * 256.f)));
    return uint32_t(ri)
         | (uint32_t(gi) << 8)
         | (uint32_t(bi) << 16)
         | (0xffu        << 24);
  }

  // Reinhard x/(1+x) tone-map + gamma 2.2, matches the prior raygen tail.
  __global__ void tonemapKernel(const float4 *hdrIn,
                                uint32_t      *fbOut,
                                int            width,
                                int            height)
  {
    const int x = blockIdx.x * blockDim.x + threadIdx.x;
    const int y = blockIdx.y * blockDim.y + threadIdx.y;
    if (x >= width || y >= height) return;

    const int idx = x + y * width;
    const float4 c = hdrIn[idx];

    float r = c.x / (1.f + c.x);
    float g = c.y / (1.f + c.y);
    float b = c.z / (1.f + c.z);
    r = powf(fmaxf(0.f, r), 1.f / 2.2f);
    g = powf(fmaxf(0.f, g), 1.f / 2.2f);
    b = powf(fmaxf(0.f, b), 1.f / 2.2f);

    fbOut[idx] = pack_rgba8(r, g, b);
  }

  void launchTonemap(const float4 *hdrIn,
                     uint32_t     *fbOut,
                     int           width,
                     int           height,
                     cudaStream_t  stream)
  {
    if (width <= 0 || height <= 0) return;
    const dim3 block(16, 16);
    const dim3 grid((width  + block.x - 1) / block.x,
                    (height + block.y - 1) / block.y);
    tonemapKernel<<<grid, block, 0, stream>>>(hdrIn, fbOut, width, height);
  }

} // namespace mypt

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

  // Optional Reinhard x/(1+x), then gamma. `useReinhard=false` matches CPU
  // framebuffer output more closely: clamp + gamma.
  __global__ void tonemapKernel(const float4 *hdrIn,
                                uint32_t      *fbOut,
                                int            width,
                                int            height,
                                float          gamma,
                                bool           useReinhard)
  {
    const int x = blockIdx.x * blockDim.x + threadIdx.x;
    const int y = blockIdx.y * blockDim.y + threadIdx.y;
    if (x >= width || y >= height) return;

    const int idx = x + y * width;
    const float4 c = hdrIn[idx];

    float r = useReinhard ? c.x / (1.f + c.x) : c.x;
    float g = useReinhard ? c.y / (1.f + c.y) : c.y;
    float b = useReinhard ? c.z / (1.f + c.z) : c.z;
    const float invGamma = gamma > 0.f ? 1.f / gamma : 1.f;
    r = powf(fmaxf(0.f, r), invGamma);
    g = powf(fmaxf(0.f, g), invGamma);
    b = powf(fmaxf(0.f, b), invGamma);

    fbOut[idx] = pack_rgba8(r, g, b);
  }

  void launchTonemap(const float4 *hdrIn,
                     uint32_t     *fbOut,
                     int           width,
                     int           height,
                     float         gamma,
                     bool          useReinhard,
                     cudaStream_t  stream)
  {
    if (width <= 0 || height <= 0) return;
    const dim3 block(16, 16);
    const dim3 grid((width  + block.x - 1) / block.x,
                    (height + block.y - 1) / block.y);
    tonemapKernel<<<grid, block, 0, stream>>>(
      hdrIn, fbOut, width, height, gamma, useReinhard);
  }

} // namespace mypt

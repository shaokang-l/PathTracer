// ======================================================================== //
// postprocess.h - host-side entry points for post-process CUDA kernels.
//
// Defined in postprocess.cu. The mypt executable links these directly;
// they are NOT part of the embedded OptiX PTX module.
// ======================================================================== //

#pragma once

#include <cstdint>
#include <cuda_runtime.h>

namespace pt {

  /*! Tone-map + gamma + RGBA8 pack. Reads `width*height`
      HDR float4 pixels from `hdrIn` and writes packed RGBA8 to `fbOut`.
      Async on `stream`; caller is responsible for stream ordering /
      synchronization. */
  void launchTonemap(const float4 *hdrIn,
                     uint32_t     *fbOut,
                     int           width,
                     int           height,
                     float         gamma,
                     bool          useReinhard,
                     cudaStream_t  stream);

} // namespace pt

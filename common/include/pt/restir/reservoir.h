#pragma once

#include "../math/vec.h"

#include <cstdint>

namespace pt {

  struct RestirLightSample {
    int lightId = -1; // the selected light
    Vec2f uv = Vec2f(0.f);
    float sourcePdf = 0.f; // candidate sampling pdf, including light selection
    float target = 0.f;    // scalar target, usually luminance of unshadowed contribution

    Vec3f position = Vec3f(0.f); // position of the sampled point
    Vec3f normal = Vec3f(0.f);   // normal of the sampled point
    Vec3f emission = Vec3f(0.f); // emission of the sampled point
  };
    
  struct RestirReservoir {
    RestirLightSample y; // the selected candidate
    float wSum = 0.f;    // sum of RIS candidate weights
    float W = 0.f;       // final reservoir weight used during shading
    uint32_t M = 0;      // number of candidates represented by the reservoir
  };

  __both__ inline void clearReservoir(RestirReservoir &reservoir)
  {
    reservoir.y = RestirLightSample();
    reservoir.wSum = 0.f;
    reservoir.W = 0.f;
    reservoir.M = 0;
  }

  __both__ inline float reservoirCandidateWeight(float target, float sourcePdf)
  {
    // numerical stability: avoid division by zero
    if (target <= 0.f || sourcePdf <= 0.f) return 0.f;
    return target / sourcePdf;
  }

  __both__ inline float reservoirCandidateWeight(const RestirLightSample &sample)
  {
    return reservoirCandidateWeight(sample.target, sample.sourcePdf);
  }

  __both__ inline bool updateReservoir(RestirReservoir &reservoir,
                                       const RestirLightSample &sample,
                                       float weight,
                                       float u)
  {
    ++reservoir.M;
    if (weight <= 0.f) return false;

    reservoir.wSum += weight;
    const float replaceProbability = weight / reservoir.wSum;
    if (u < replaceProbability) {
      reservoir.y = sample;
      return true;
    }
    return false;
  }

  __both__ inline bool updateReservoir(RestirReservoir &reservoir,
                                       const RestirLightSample &sample,
                                       float u)
  {
    return updateReservoir(reservoir, sample, reservoirCandidateWeight(sample), u);
  }

  __both__ inline void finalizeReservoir(RestirReservoir &reservoir,
                                         float selectedTarget)
  {
    if (reservoir.M == 0 || reservoir.wSum <= 0.f || selectedTarget <= 0.f) {
      reservoir.W = 0.f;
      return;
    }
    reservoir.W = reservoir.wSum / (float(reservoir.M) * selectedTarget);
  }

  __both__ inline void finalizeReservoir(RestirReservoir &reservoir)
  {
    finalizeReservoir(reservoir, reservoir.y.target);
  }

}
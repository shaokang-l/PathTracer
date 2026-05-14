#include "pt/restir/reservoir.h"

#include <gtest/gtest.h>

#include <cmath>
#include <random>

namespace {

pt::RestirLightSample makeSample(int lightId, float target, float sourcePdf)
{
  pt::RestirLightSample sample;
  sample.lightId = lightId;
  sample.target = target;
  sample.sourcePdf = sourcePdf;
  return sample;
}

} // namespace

TEST(RestirReservoir, CandidateWeightUsesTargetOverSourcePdf)
{
  EXPECT_FLOAT_EQ(pt::reservoirCandidateWeight(6.f, 2.f), 3.f);
  EXPECT_FLOAT_EQ(pt::reservoirCandidateWeight(0.f, 2.f), 0.f);
  EXPECT_FLOAT_EQ(pt::reservoirCandidateWeight(6.f, 0.f), 0.f);
  EXPECT_FLOAT_EQ(pt::reservoirCandidateWeight(-1.f, 2.f), 0.f);
  EXPECT_FLOAT_EQ(pt::reservoirCandidateWeight(6.f, -1.f), 0.f);
}

TEST(RestirReservoir, SingleCandidateFinalizesToInverseSourcePdf)
{
  pt::RestirReservoir reservoir;
  const pt::RestirLightSample sample = makeSample(3, 4.f, 0.25f);

  const bool replaced = pt::updateReservoir(reservoir, sample, 0.99f);
  pt::finalizeReservoir(reservoir);

  EXPECT_TRUE(replaced);
  EXPECT_EQ(reservoir.M, 1u);
  EXPECT_FLOAT_EQ(reservoir.wSum, 16.f);
  EXPECT_EQ(reservoir.y.lightId, 3);
  EXPECT_FLOAT_EQ(reservoir.W, 4.f);
}

TEST(RestirReservoir, ZeroWeightCandidateIsSafe)
{
  pt::RestirReservoir reservoir;
  const pt::RestirLightSample sample = makeSample(7, 0.f, 0.5f);

  const bool replaced = pt::updateReservoir(reservoir, sample, 0.0f);
  pt::finalizeReservoir(reservoir);

  EXPECT_FALSE(replaced);
  EXPECT_EQ(reservoir.M, 1u);
  EXPECT_FLOAT_EQ(reservoir.wSum, 0.f);
  EXPECT_FLOAT_EQ(reservoir.W, 0.f);
  EXPECT_TRUE(std::isfinite(reservoir.W));
  EXPECT_EQ(reservoir.y.lightId, -1);
}

TEST(RestirReservoir, SelectionProbabilityMatchesWeights)
{
  std::mt19937 rng(12345u);
  std::uniform_real_distribution<float> uniform(0.f, 1.f);

  constexpr int trials = 100000;
  int counts[3] = {0, 0, 0};
  const pt::RestirLightSample samples[3] = {
    makeSample(0, 1.f, 1.f),
    makeSample(1, 2.f, 1.f),
    makeSample(2, 7.f, 1.f),
  };

  for (int trial = 0; trial < trials; ++trial) {
    pt::RestirReservoir reservoir;
    for (const pt::RestirLightSample &sample : samples) {
      pt::updateReservoir(reservoir, sample, uniform(rng));
    }
    ASSERT_GE(reservoir.y.lightId, 0);
    ASSERT_LT(reservoir.y.lightId, 3);
    ++counts[reservoir.y.lightId];
  }

  EXPECT_NEAR(float(counts[0]) / float(trials), 0.1f, 0.01f);
  EXPECT_NEAR(float(counts[1]) / float(trials), 0.2f, 0.01f);
  EXPECT_NEAR(float(counts[2]) / float(trials), 0.7f, 0.01f);
}

TEST(RestirReservoir, EqualWeightsSelectUniformly)
{
  std::mt19937 rng(54321u);
  std::uniform_real_distribution<float> uniform(0.f, 1.f);

  constexpr int trials = 80000;
  int counts[4] = {0, 0, 0, 0};

  for (int trial = 0; trial < trials; ++trial) {
    pt::RestirReservoir reservoir;
    for (int lightId = 0; lightId < 4; ++lightId) {
      pt::updateReservoir(reservoir,
                          makeSample(lightId, 1.f, 1.f),
                          uniform(rng));
    }
    ASSERT_GE(reservoir.y.lightId, 0);
    ASSERT_LT(reservoir.y.lightId, 4);
    ++counts[reservoir.y.lightId];
  }

  for (int lightId = 0; lightId < 4; ++lightId) {
    EXPECT_NEAR(float(counts[lightId]) / float(trials), 0.25f, 0.01f);
  }
}

TEST(RestirReservoir, ClearResetsAllState)
{
  pt::RestirReservoir reservoir;
  pt::updateReservoir(reservoir, makeSample(2, 3.f, 0.5f), 0.5f);
  pt::finalizeReservoir(reservoir);

  pt::clearReservoir(reservoir);

  EXPECT_EQ(reservoir.y.lightId, -1);
  EXPECT_FLOAT_EQ(reservoir.wSum, 0.f);
  EXPECT_FLOAT_EQ(reservoir.W, 0.f);
  EXPECT_EQ(reservoir.M, 0u);
}


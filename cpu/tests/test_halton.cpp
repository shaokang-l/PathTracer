#include "sampler/sampler.hpp"
#include <gtest/gtest.h>

// Verify the first few values of the 1D Halton (base-2) sequence
TEST(HaltonSampler, Get1DFirstValues) {
  HaltonSampler sampler(0, RandomStrategy::None);
  // Sample 1: index = 1 → radicalInverse(base2,1) = 0.5
  sampler.startSample();
  EXPECT_NEAR(sampler.get1D(), 0.5f, 1e-6f);
  // Sample 2: index = 2 → radicalInverse(base2,2) = 0.25
  sampler.startSample();
  EXPECT_NEAR(sampler.get1D(), 0.25f, 1e-6f);
  // Sample 3: index = 3 → radicalInverse(base2,3) = 0.75
  sampler.startSample();
  EXPECT_NEAR(sampler.get1D(), 0.75f, 1e-6f);
  // Sample 4: index = 4 → radicalInverse(base2,4) = 0.125
  sampler.startSample();
  EXPECT_NEAR(sampler.get1D(), 0.125f, 1e-6f);
}

// Verify the first few values of the 2D Halton (bases 2,3) sequence
TEST(HaltonSampler, Get2DFirstValues) {
  HaltonSampler sampler(0, RandomStrategy::None);
  // Sample 1: index = 1 → (base2:0.5, base3:1/3)
  sampler.startSample();
  auto v1 = sampler.get2D();
  EXPECT_NEAR(v1.x(), 0.5f, 1e-6f);
  EXPECT_NEAR(v1.y(), 1.0f / 3.0f, 1e-6f);
  // Sample 2: index = 2 → (base2:0.25, base3:2/3)
  sampler.startSample();
  auto v2 = sampler.get2D();
  EXPECT_NEAR(v2.x(), 0.25f, 1e-6f);
  EXPECT_NEAR(v2.y(), 2.0f / 3.0f, 1e-6f);
}

// Ensure that startSample() resets the dimension counter
TEST(HaltonSampler, StartSampleResetsDimension) {
  HaltonSampler sampler(0, RandomStrategy::None);
  // First sample: advance to index=1, dimension→0
  sampler.startSample();
  float a = sampler.get1D(); // dim=0 → 0.5
  auto b = sampler.get2D();  // dim=1&2 → (1/3, ...)
  // Now dimension>=2; next startSample should reset dim to 0
  sampler.startSample();                      // index=2, dimension→0
  EXPECT_NEAR(sampler.get1D(), 0.25f, 1e-6f); // dim=0 again
}

// Check basic statistics over many 1D samples: mean≈0.5, var≈1/12
TEST(HaltonSampler, BasicStatistics) {
  HaltonSampler sampler(0, RandomStrategy::Owen);
  const int N = 1024;
  double sum = 0.0;
  double sumSq = 0.0;
  for (int i = 0; i < N; ++i) {
    sampler.startSample();
    double v = sampler.get1D();
    sum += v;
    sumSq += v * v;
  }
  double mean = sum / N;
  double var = sumSq / N - mean * mean;
  EXPECT_NEAR(mean, 0.5, 1e-2);
  EXPECT_NEAR(var, 1.0 / 12.0, 1e-2);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
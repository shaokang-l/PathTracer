#include "pt/restir/reservoir.h"
#include "pt/restir/target.h"

#include <gtest/gtest.h>

#include <array>
#include <random>

namespace {

struct ToyCandidate {
  int lightId = -1;
  pt::Vec3f contribution = pt::Vec3f(0.f); // unshadowed area-domain contribution
  float sourcePdf = 0.f;
};

struct RunningStats {
  double mean = 0.0;
  double variance = 0.0;
};

const std::array<ToyCandidate, 3> kCandidates = {
  ToyCandidate{0, pt::Vec3f(0.25f), 0.50f},
  ToyCandidate{1, pt::Vec3f(1.00f), 0.30f},
  ToyCandidate{2, pt::Vec3f(4.00f), 0.20f},
};

float referenceIntegral()
{
  float sum = 0.f;
  for (const ToyCandidate &candidate : kCandidates) {
    sum += candidate.contribution.x;
  }
  return sum;
}

const ToyCandidate &sampleCandidate(float u)
{
  float cdf = 0.f;
  for (const ToyCandidate &candidate : kCandidates) {
    cdf += candidate.sourcePdf;
    if (u < cdf) return candidate;
  }
  return kCandidates.back();
}

pt::RestirLightSample makeRestirSample(const ToyCandidate &candidate)
{
  pt::RestirLightSample sample;
  sample.lightId = candidate.lightId;
  sample.sourcePdf = candidate.sourcePdf;
  sample.target = pt::restirTargetFromRgb(candidate.contribution);
  return sample;
}

float contributionForLight(int lightId)
{
  for (const ToyCandidate &candidate : kCandidates) {
    if (candidate.lightId == lightId) return candidate.contribution.x;
  }
  return 0.f;
}

float estimateOneSampleNee(const ToyCandidate &candidate)
{
  if (candidate.sourcePdf <= 0.f) return 0.f;
  return candidate.contribution.x / candidate.sourcePdf;
}

float estimateNoReuseRis(int initialCandidates, std::mt19937 &rng)
{
  std::uniform_real_distribution<float> uniform(0.f, 1.f);
  pt::RestirReservoir reservoir;

  for (int i = 0; i < initialCandidates; ++i) {
    const ToyCandidate &candidate = sampleCandidate(uniform(rng));
    pt::updateReservoir(reservoir, makeRestirSample(candidate), uniform(rng));
  }

  pt::finalizeReservoir(reservoir);
  return contributionForLight(reservoir.y.lightId) * reservoir.W;
}

RunningStats estimateStats(int initialCandidates, int trials, uint32_t seed)
{
  std::mt19937 rng(seed);
  double sum = 0.0;
  double sumSq = 0.0;

  for (int i = 0; i < trials; ++i) {
    const double estimate = estimateNoReuseRis(initialCandidates, rng);
    sum += estimate;
    sumSq += estimate * estimate;
  }

  RunningStats stats;
  stats.mean = sum / double(trials);
  stats.variance = sumSq / double(trials) - stats.mean * stats.mean;
  return stats;
}

} // namespace

TEST(RestirRisEstimator, OneSampleNeeMeanMatchesReferenceIntegral)
{
  std::mt19937 rng(1234u);
  std::uniform_real_distribution<float> uniform(0.f, 1.f);

  constexpr int trials = 100000;
  double sum = 0.0;
  for (int i = 0; i < trials; ++i) {
    sum += estimateOneSampleNee(sampleCandidate(uniform(rng)));
  }

  EXPECT_NEAR(sum / double(trials), referenceIntegral(), 0.05);
}

TEST(RestirRisEstimator, SingleInitialCandidateMatchesNeeForSameCandidate)
{
  for (const ToyCandidate &candidate : kCandidates) {
    pt::RestirReservoir reservoir;
    pt::updateReservoir(reservoir, makeRestirSample(candidate), 0.99f);
    pt::finalizeReservoir(reservoir);

    const float risEstimate =
      contributionForLight(reservoir.y.lightId) * reservoir.W;
    EXPECT_NEAR(risEstimate, estimateOneSampleNee(candidate), 1e-6f);
  }
}

TEST(RestirRisEstimator, NoReuseRisMeanMatchesReferenceIntegral)
{
  constexpr int trials = 100000;
  const RunningStats stats = estimateStats(/*initialCandidates=*/8, trials, 5678u);

  EXPECT_NEAR(stats.mean, referenceIntegral(), 0.05);
}

TEST(RestirRisEstimator, MoreInitialCandidatesReduceVariance)
{
  constexpr int trials = 100000;
  const RunningStats one = estimateStats(/*initialCandidates=*/1, trials, 10u);
  const RunningStats four = estimateStats(/*initialCandidates=*/4, trials, 20u);
  const RunningStats sixteen = estimateStats(/*initialCandidates=*/16, trials, 30u);

  EXPECT_NEAR(one.mean, referenceIntegral(), 0.08);
  EXPECT_NEAR(four.mean, referenceIntegral(), 0.05);
  EXPECT_NEAR(sixteen.mean, referenceIntegral(), 0.03);

  EXPECT_LT(four.variance, one.variance * 0.40);
  EXPECT_LT(sixteen.variance, four.variance * 0.40);
}


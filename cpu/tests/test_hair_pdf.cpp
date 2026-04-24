#include "header.hpp"
#include <gtest/gtest.h>

using namespace gl;

TEST(HairPDF, Normalization) {
  // Set up a fixed outgoing direction and hair parameters
  vec3 hairTangent(0.f, 0.f, 1.f);
  vec3 wo_world(0.f, 1.f, 0.f);
  wo_world = normalize(wo_world);

  // HairMarschner(h, β_m, η, β_n, α_R, α)
  HairMarschner hair(vec3(0.419f, 0.697f, 1.37f), 0.f, 1.55f, 0.7f, 0.5f,
                     to_radian(2.f));

  HairPDF pdf(hair, hairTangent, wo_world);

  // Monte‐Carlo integrate ∫_{S²} pdf(w) dω ≡ 1?
  const int N = 200000;
  double sum = 0.0, negativeCount = 0.0;
  for (int i = 0; i < N; ++i) {
    float u1 = rand_num(), u2 = rand_num();
    vec3 wi = uniformSampleSphere(u1, u2);
    float p = pdf.at(wi);
    if (p < 0) {
      negativeCount += 1.0;
      p = 0.f;
    }
    sum += p;
  }
  double avg = sum / double(N);
  double estimate = avg * (4.0 * M_PI);

  // No negative PDFs
  EXPECT_EQ(negativeCount, 0.0) << "Saw " << negativeCount << " negative pdfs";

  // Estimate should be within ~1% of 1.0
  EXPECT_NEAR(estimate, 1.0, 0.02) << "Normalization off: ∫pdf = " << estimate;
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
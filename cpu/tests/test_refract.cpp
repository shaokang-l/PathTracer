// test_refract.cpp
#include "utils/utility.hpp"
#include <cmath>
#include <gtest/gtest.h>

using namespace gl; // if vec3 lives in namespace gl

TEST(RefractTest, NoRefractionEtaOne) {
  vec3 wo(0.0f, 0.0f, -1.0f);
  vec3 n(0.0f, 0.0f, 1.0f);
  vec3 wi;
  float etaScale;
  bool success = refract(wo, n, 1.0f, etaScale, wi);
  EXPECT_TRUE(success);
  EXPECT_FLOAT_EQ(etaScale, 1.0f);
  EXPECT_NEAR(wi.x(), wo.x(), 1e-6f);
  EXPECT_NEAR(wi.y(), wo.y(), 1e-6f);
  EXPECT_NEAR(wi.z(), wo.z(), 1e-6f);
}

TEST(RefractTest, TotalInternalReflection) {
  // 45° incidence: cosθ = √2/2, eta > 1 ⇒ TIR should occur
  constexpr float angle = M_PI / 4.0f;
  vec3 wo(std::sin(angle), 0.0f, std::cos(angle));
  vec3 n(0.0f, 0.0f, 1.0f);
  vec3 wi;
  float etaScale;
  bool success = refract(wo, n, 2.0f, etaScale, wi);
  EXPECT_FALSE(success);
}

TEST(RefractTest, BendsAwayFromNormal) {
  // 10° incidence: small angle ⇒ refraction should succeed
  constexpr float angle = M_PI / 18.0f; // 10°
  vec3 wo(std::sin(angle), 0.0f, std::cos(angle));
  vec3 n(0.0f, 0.0f, 1.0f);
  vec3 wi;
  float etaScale;
  bool success = refract(wo, n, 1.5f, etaScale, wi);

  ASSERT_TRUE(success);
  EXPECT_FLOAT_EQ(etaScale, 1.5f);

  // Check the refracted vector is still unit length
  float wiLen = std::sqrt(wi.x() * wi.x() + wi.y() * wi.y() + wi.z() * wi.z());
  EXPECT_NEAR(wiLen, 1.0f, 1e-6f);

  // Since η>1 the ray bends *away* from the normal:
  // the z–component of wi should be < that of the incident wo
  EXPECT_LT(wi.z(), wo.z());
}

using namespace gl; // if vec3 lives here

TEST(RefractTest, DirectionMatchesSnellLaw) {
  // 30° incidence in the x–z plane, incident INTO the surface
  constexpr float angle = M_PI / 6.0f;
  vec3 I(std::sin(angle), 0.0f, std::cos(angle)); // I·n = cos(angle)>0
  vec3 wo = I;                                    // outgoing/view dir
  vec3 n(0.0f, 0.0f, 1.0f);                       // world‐space normal

  float eta = 1.5f; // ηᵢ/ηₜ
  float etaScale = 0.0f;
  vec3 wi; // will hold the refracted direction

  // Should succeed and not flip IOR
  ASSERT_TRUE(pbrt::refract(wo, n, eta, etaScale, wi));
  EXPECT_FLOAT_EQ(etaScale, eta);

  // Recompute exactly what your refract() does (no-flip branch)
  float cosThetaI = dot(n, I); // = cos(angle)
  float sin2ThetaI = std::fmax(0.0f, 1 - cosThetaI * cosThetaI);
  float sin2ThetaT = sin2ThetaI / (eta * eta);
  ASSERT_LT(sin2ThetaT, 1.0f);
  float cosThetaT = std::sqrt(1 - sin2ThetaT);

  // PBRT‐style transmitted direction (unit length)
  //    T = -I/η + (cosθᵢ/η - cosθₜ) * n
  vec3 expected = (-I / eta) + (cosThetaI / eta - cosThetaT) * n;
  expected = expected.normalize();

  constexpr float eps = 1e-6f;
  EXPECT_NEAR(wi.x(), expected.x(), eps);
  EXPECT_NEAR(wi.y(), expected.y(), eps);
  EXPECT_NEAR(wi.z(), expected.z(), eps);
}

// No need for your own main — link against gtest_main
// g++ test_refract.cpp -lgtest -lgtest_main -pthread -o test_refract

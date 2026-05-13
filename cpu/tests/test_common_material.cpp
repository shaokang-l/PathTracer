#include "material/materialMath.hpp"
#include "pt/material/fresnel.h"
#include "pt/material/trowbridge_reitz.h"

#include <gtest/gtest.h>

TEST(CommonMaterialMath, FresnelDielectricMatchesCpuFacade) {
  const float cosTheta = 0.37f;
  const float eta = 1.5f;
  EXPECT_NEAR(gl::fresnelDielectric(cosTheta, eta),
              pt::fresnelDielectric(cosTheta, eta),
              1e-6f);
}

TEST(CommonMaterialMath, ConductorFresnelIsFiniteAndBounded) {
  const pt::Vec3f eta(0.20f, 0.92f, 1.10f);
  const pt::Vec3f k(3.91f, 2.45f, 2.14f);
  const pt::Vec3f f = pt::fresnelComplex(0.5f, eta, k);

  EXPECT_GE(f.x, 0.f);
  EXPECT_GE(f.y, 0.f);
  EXPECT_GE(f.z, 0.f);
  EXPECT_LE(f.x, 1.f);
  EXPECT_LE(f.y, 1.f);
  EXPECT_LE(f.z, 1.f);
}

TEST(CommonMaterialMath, TrowbridgeReitzFacadeMatchesCommon) {
  TrowbridgeReitzDistribution cpu(0.3f, 0.5f);
  pt::TrowbridgeReitzDistribution common(0.3f, 0.5f);

  const gl::vec3 cpuWm = gl::vec3(0.2f, -0.3f, 0.93f).normalize();
  const pt::Vec3f commonWm(cpuWm.x(), cpuWm.y(), cpuWm.z());

  EXPECT_NEAR(cpu.D(cpuWm), common.D(commonWm), 1e-5f);
  EXPECT_NEAR(cpu.lambda(cpuWm), common.lambda(commonWm), 1e-5f);
}


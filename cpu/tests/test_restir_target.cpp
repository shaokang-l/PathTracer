#include "pt/restir/target.h"

#include <gtest/gtest.h>

TEST(RestirTarget, LuminanceUsesRec709Weights)
{
  EXPECT_NEAR(pt::restirLuminance(pt::Vec3f(1.f, 0.f, 0.f)), 0.2126f, 1e-6f);
  EXPECT_NEAR(pt::restirLuminance(pt::Vec3f(0.f, 1.f, 0.f)), 0.7152f, 1e-6f);
  EXPECT_NEAR(pt::restirLuminance(pt::Vec3f(0.f, 0.f, 1.f)), 0.0722f, 1e-6f);
  EXPECT_NEAR(pt::restirLuminance(pt::Vec3f(1.f)), 1.f, 1e-6f);
}

TEST(RestirTarget, TargetFromRgbClampsNegativeLuminance)
{
  EXPECT_FLOAT_EQ(pt::restirTargetFromRgb(pt::Vec3f(-1.f)), 0.f);
  EXPECT_FLOAT_EQ(pt::restirTargetFromRgb(pt::Vec3f(2.f)), 2.f);
}

TEST(RestirTarget, GeometryTermHandlesFacingPair)
{
  const pt::Vec3f shadingP(0.f, 0.f, 0.f);
  const pt::Vec3f shadingN(0.f, 1.f, 0.f);
  const pt::Vec3f lightP(0.f, 2.f, 0.f);
  const pt::Vec3f lightN(0.f, -1.f, 0.f);

  const pt::DirectLightGeometry geom =
    pt::directLightGeometry(shadingP, shadingN, lightP, lightN);

  EXPECT_NEAR(geom.wi.x, 0.f, 1e-6f);
  EXPECT_NEAR(geom.wi.y, 1.f, 1e-6f);
  EXPECT_NEAR(geom.wi.z, 0.f, 1e-6f);
  EXPECT_NEAR(geom.distance, 2.f, 1e-6f);
  EXPECT_NEAR(geom.distanceSquared, 4.f, 1e-6f);
  EXPECT_NEAR(geom.NoI, 1.f, 1e-6f);
  EXPECT_NEAR(geom.NoL, 1.f, 1e-6f);
  EXPECT_NEAR(geom.G, 0.25f, 1e-6f);
}

TEST(RestirTarget, GeometryTermRejectsBackFacingShadingPoint)
{
  const float G = pt::geometryTerm(pt::Vec3f(0.f, 0.f, 0.f),
                                   pt::Vec3f(0.f, -1.f, 0.f),
                                   pt::Vec3f(0.f, 2.f, 0.f),
                                   pt::Vec3f(0.f, -1.f, 0.f));

  EXPECT_FLOAT_EQ(G, 0.f);
}

TEST(RestirTarget, GeometryTermRejectsBackFacingLight)
{
  const float G = pt::geometryTerm(pt::Vec3f(0.f, 0.f, 0.f),
                                   pt::Vec3f(0.f, 1.f, 0.f),
                                   pt::Vec3f(0.f, 2.f, 0.f),
                                   pt::Vec3f(0.f, 1.f, 0.f));

  EXPECT_FLOAT_EQ(G, 0.f);
}

TEST(RestirTarget, GeometryTermFallsOffWithDistanceSquared)
{
  const float nearG = pt::geometryTerm(pt::Vec3f(0.f, 0.f, 0.f),
                                       pt::Vec3f(0.f, 1.f, 0.f),
                                       pt::Vec3f(0.f, 2.f, 0.f),
                                       pt::Vec3f(0.f, -1.f, 0.f));
  const float farG = pt::geometryTerm(pt::Vec3f(0.f, 0.f, 0.f),
                                      pt::Vec3f(0.f, 1.f, 0.f),
                                      pt::Vec3f(0.f, 4.f, 0.f),
                                      pt::Vec3f(0.f, -1.f, 0.f));

  EXPECT_NEAR(farG, nearG * 0.25f, 1e-6f);
}

TEST(RestirTarget, DirectLightingAreaIntegrandMultipliesRadianceBsdfAndGeometry)
{
  const pt::Vec3f integrand =
    pt::directLightingAreaIntegrand(pt::Vec3f(10.f, 4.f, 2.f),
                                    pt::Vec3f(0.5f, 0.25f, 0.1f),
                                    0.2f);

  EXPECT_NEAR(integrand.x, 1.f, 1e-6f);
  EXPECT_NEAR(integrand.y, 0.2f, 1e-6f);
  EXPECT_NEAR(integrand.z, 0.04f, 1e-6f);
}

TEST(RestirTarget, DirectLightingTargetUsesLuminanceOfIntegrand)
{
  const pt::Vec3f emittedRadiance(8.f, 8.f, 8.f);
  const pt::Vec3f bsdfValue(0.5f, 0.5f, 0.5f);
  const float target = pt::directLightingTarget(emittedRadiance, bsdfValue, 0.25f);

  EXPECT_NEAR(target, 1.f, 1e-6f);
}

TEST(RestirTarget, CoincidentPointsAreSafe)
{
  const pt::DirectLightGeometry geom =
    pt::directLightGeometry(pt::Vec3f(1.f, 2.f, 3.f),
                            pt::Vec3f(0.f, 1.f, 0.f),
                            pt::Vec3f(1.f, 2.f, 3.f),
                            pt::Vec3f(0.f, -1.f, 0.f));

  EXPECT_FLOAT_EQ(geom.distanceSquared, 0.f);
  EXPECT_FLOAT_EQ(geom.G, 0.f);
}


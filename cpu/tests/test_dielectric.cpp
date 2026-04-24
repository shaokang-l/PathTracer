// MFDielectricPDF_test.cpp
#include "material/dielectric.hpp"
#include "utils/orthoBasis.hpp"
#include <cmath>
#include <gtest/gtest.h>
#include <limits>
#include <random>

using namespace gl;

class MFDielectricTest : public ::testing::Test {
protected:
  HitRecord rec;
  ScatterRecord srec;
  std::mt19937 rng; // For generating uc and u if needed for multiple samples

  MFDielectricTest() : rng(std::random_device{}()) {}

  void SetUp() override {
    // Assuming gl::vec3 constructor takes (float, float, float)
    // and gl::vec3 has methods x(), y(), z() and normalize()
    rec.position = gl::vec3(0.0f, 0.0f, 0.0f);
    rec.normal =
        gl::vec3(0.0f, 0.0f, 1.0f).normalize(); // Ensure normal is normalized
    srec = ScatterRecord();                     // Reset scatter record
  }

  // Helper to create an incident ray coming from a specific world-space
  // direction wo_world_direction wo_world_direction is the direction from the
  // surface to the previous point (e.g., camera/light)
  Ray CreateRayForWoWorld(const gl::vec3 &wo_world_direction) {
    // The ray_in.getDirection() should be -wo_world_direction
    // Assuming Ray constructor takes (origin, direction)
    // and gl::vec3 supports operator* with float and operator+
    return Ray(rec.position + wo_world_direction * 0.1f,
               -wo_world_direction.normalize());
  }

  // Helper to check for bad float values
  void CheckScatterRecordValidity(const ScatterRecord &record,
                                  bool check_pdf_positive = true) {
    ASSERT_FALSE(std::isnan(record.pdf_val)) << "PDF is NaN";
    ASSERT_FALSE(std::isinf(record.pdf_val)) << "PDF is Inf";
    if (check_pdf_positive) {
      ASSERT_GT(record.pdf_val, 0.0f) << "PDF is not strictly positive";
    } else {
      ASSERT_GE(record.pdf_val, 0.0f) << "PDF is negative";
    }

    // Assuming record.attenuation is gl::vec3 with x(), y(), z() methods
    ASSERT_FALSE(std::isnan(record.attenuation.x()))
        << "Attenuation.x() is NaN";
    ASSERT_FALSE(std::isinf(record.attenuation.x()))
        << "Attenuation.x() is Inf";
    ASSERT_GE(record.attenuation.x(), 0.0f) << "Attenuation.x() is negative";
    ASSERT_FALSE(std::isnan(record.attenuation.y()))
        << "Attenuation.y() is NaN";
    ASSERT_FALSE(std::isinf(record.attenuation.y()))
        << "Attenuation.y() is Inf";
    ASSERT_GE(record.attenuation.y(), 0.0f) << "Attenuation.y() is negative";
    ASSERT_FALSE(std::isnan(record.attenuation.z()))
        << "Attenuation.z() is NaN";
    ASSERT_FALSE(std::isinf(record.attenuation.z()))
        << "Attenuation.z() is Inf";
    ASSERT_GE(record.attenuation.z(), 0.0f) << "Attenuation.z() is negative";

    // Assuming record.sampled_ray has getDirection() returning gl::vec3
    ASSERT_FALSE(std::isnan(record.sampled_ray.getDirection().x()))
        << "Sampled ray getDirection().x() is NaN";
    ASSERT_FALSE(std::isinf(record.sampled_ray.getDirection().x()))
        << "Sampled ray getDirection().x() is Inf";
    // Check other components of direction if necessary
    float dir_len_sq = gl::dot(record.sampled_ray.getDirection(),
                               record.sampled_ray.getDirection());
    ASSERT_NEAR(dir_len_sq, 1.0f, 1e-5f)
        << "Sampled ray direction is not normalized";
  }
};

TEST_F(MFDielectricTest, SmoothNormalIncidenceAirToGlass) {
  MFDielectric bxdf(1.5f, 0.0f, 0.0f);
  ASSERT_TRUE(bxdf.effectivelySmooth());

  gl::vec3 wo_world = gl::vec3(0.0f, 0.0f, 1.0f);
  Ray ray_in = CreateRayForWoWorld(wo_world);

  float expected_R = gl::square((1.5f - 1.0f) / (1.5f + 1.0f));
  float expected_T = 1.0f - expected_R;

  srec = ScatterRecord();
  bool scattered_refl = bxdf.scatter(ray_in, rec, srec, 0.01f);
  ASSERT_TRUE(scattered_refl);
  EXPECT_EQ(srec.sampled_type, BxDFFlags::SpecularReflection);
  CheckScatterRecordValidity(srec, srec.pdf_val > 0.0f);
  EXPECT_NEAR(srec.pdf_val, expected_R, 1e-6f);
  EXPECT_NEAR(srec.sampled_ray.getDirection().x(), 0.0f, 1e-6f);
  EXPECT_NEAR(srec.sampled_ray.getDirection().y(), 0.0f, 1e-6f);
  EXPECT_NEAR(srec.sampled_ray.getDirection().z(), 1.0f, 1e-6f);

  srec = ScatterRecord();
  bool scattered_trans = bxdf.scatter(ray_in, rec, srec, 0.99f);
  ASSERT_TRUE(scattered_trans);
  EXPECT_EQ(srec.sampled_type, BxDFFlags::SpecularTransmission);
  CheckScatterRecordValidity(srec, srec.pdf_val > 0.0f);
  EXPECT_NEAR(srec.pdf_val, expected_T, 1e-6f);
  EXPECT_NEAR(srec.sampled_ray.getDirection().x(), 0.0f, 1e-6f);
  EXPECT_NEAR(srec.sampled_ray.getDirection().y(), 0.0f, 1e-6f);
  EXPECT_NEAR(srec.sampled_ray.getDirection().z(), -1.0f, 1e-6f);
}

TEST_F(MFDielectricTest, SmoothNormalIncidenceGlassToAir) {
  MFDielectric bxdf(1.5f, 0.0f, 0.0f);
  ASSERT_TRUE(bxdf.effectivelySmooth());

  gl::vec3 wo_world = gl::vec3(0.0f, 0.0f, -1.0f);
  Ray ray_in = CreateRayForWoWorld(wo_world);

  float eta_ratio = 1.0f / 1.5f;
  float expected_R = gl::square((eta_ratio - 1.0f) / (eta_ratio + 1.0f));
  float expected_T = 1.0f - expected_R;

  srec = ScatterRecord();
  bool scattered_refl = bxdf.scatter(ray_in, rec, srec, 0.01f);
  ASSERT_TRUE(scattered_refl);
  EXPECT_EQ(srec.sampled_type, BxDFFlags::SpecularReflection);
  CheckScatterRecordValidity(srec, srec.pdf_val > 0.0f);
  EXPECT_NEAR(srec.pdf_val, expected_R, 1e-6f);
  EXPECT_NEAR(srec.sampled_ray.getDirection().x(), 0.0f, 1e-6f);
  EXPECT_NEAR(srec.sampled_ray.getDirection().y(), 0.0f, 1e-6f);
  EXPECT_NEAR(srec.sampled_ray.getDirection().z(), -1.0f, 1e-6f);

  srec = ScatterRecord();
  bool scattered_trans = bxdf.scatter(ray_in, rec, srec, 0.99f);
  ASSERT_TRUE(scattered_trans);
  EXPECT_EQ(srec.sampled_type, BxDFFlags::SpecularTransmission);
  CheckScatterRecordValidity(srec, srec.pdf_val > 0.0f);
  EXPECT_NEAR(srec.pdf_val, expected_T, 1e-6f);
  EXPECT_NEAR(srec.sampled_ray.getDirection().x(), 0.0f, 1e-6f);
  EXPECT_NEAR(srec.sampled_ray.getDirection().y(), 0.0f, 1e-6f);
  EXPECT_NEAR(srec.sampled_ray.getDirection().z(), 1.0f, 1e-6f);
}

TEST_F(MFDielectricTest, SmoothTotalInternalReflectionGlassToAir) {
  MFDielectric bxdf(1.5f, 0.0f, 0.0f);
  // User commented this out, so we respect that. If it should be active, the
  // BxDF's effectivelySmooth might need tuning.
  // ASSERT_TRUE(bxdf.mfDistrib.effectivelySmooth());

  float sin_crit_sq = gl::square(1.0f / 1.5f);
  float cos_crit = std::sqrt(1.0f - sin_crit_sq);

  float cos_theta_i_val = cos_crit * 0.8f;
  float sin_theta_i_val = std::sqrt(1.0f - gl::square(cos_theta_i_val));

  gl::vec3 wo_world =
      gl::vec3(sin_theta_i_val, 0.0f, -cos_theta_i_val).normalize();
  Ray ray_in = CreateRayForWoWorld(wo_world);

  srec = ScatterRecord();
  bool scattered = bxdf.scatter(ray_in, rec, srec, 0.5f);
  ASSERT_TRUE(scattered) << "Scatter returned false for expected TIR case.";
  EXPECT_EQ(srec.sampled_type, BxDFFlags::SpecularReflection);
  CheckScatterRecordValidity(srec, srec.pdf_val > 0.0f);
  EXPECT_NEAR(srec.pdf_val, 1.0f, 1e-6f);

  // Corrected expectations for the reflected ray direction
  // If wo_world is (s, 0, -c), reflected wi_world is (-s, 0, -c)
  EXPECT_NEAR(srec.sampled_ray.getDirection().x(), -sin_theta_i_val, 1e-6f);
  EXPECT_NEAR(srec.sampled_ray.getDirection().y(), 0.0f, 1e-6f);
  EXPECT_NEAR(srec.sampled_ray.getDirection().z(), -cos_theta_i_val, 1e-6f);
}

// --- Rough Surface Tests ---

TEST_F(MFDielectricTest, RoughPathNoNaNsOrInfsGrazingIncidence) {
  MFDielectric bxdf(1.5f, 0.3f, 0.3f);
  ASSERT_FALSE(bxdf.effectivelySmooth());

  gl::vec3 wo_world = gl::vec3(0.99f, 0.1f, 0.01f).normalize();
  Ray ray_in = CreateRayForWoWorld(wo_world);

  int num_samples = 100;
  std::uniform_real_distribution<float> dist_uc(0.f, 1.f);
  std::uniform_real_distribution<float> dist_u(0.f, 1.f);

  for (int i = 0; i < num_samples; ++i) {
    srec = ScatterRecord();
    float uc = dist_uc(rng);
    gl::vec2 u_sample = {dist_u(rng), dist_u(rng)};

    bool scattered = bxdf.scatter(ray_in, rec, srec, uc, u_sample);

    if (scattered) {
      SCOPED_TRACE("Sample " + std::to_string(i) +
                   ", uc=" + std::to_string(uc));
      CheckScatterRecordValidity(srec);
      // Check that it's a glossy type if scattered from rough path
      ASSERT_TRUE(srec.sampled_type == BxDFFlags::GlossyReflection ||
                  srec.sampled_type == BxDFFlags::GlossyTransmission)
          << "Sampled type is not glossy for rough surface.";
    }
  }
}

TEST_F(MFDielectricTest, RoughPathNoNaNsOrInfsNearNormalIncidence) {
  MFDielectric bxdf(1.5f, 0.3f, 0.3f);
  ASSERT_FALSE(bxdf.effectivelySmooth());

  gl::vec3 wo_world = gl::vec3(0.01f, 0.01f, 0.99f).normalize();
  Ray ray_in = CreateRayForWoWorld(wo_world);

  int num_samples = 100;
  std::uniform_real_distribution<float> dist_uc(0.f, 1.f);
  std::uniform_real_distribution<float> dist_u(0.f, 1.f);

  for (int i = 0; i < num_samples; ++i) {
    srec = ScatterRecord();
    float uc = dist_uc(rng);
    gl::vec2 u_sample = {dist_u(rng), dist_u(rng)};

    bool scattered = bxdf.scatter(ray_in, rec, srec, uc, u_sample);

    if (scattered) {
      SCOPED_TRACE("Sample " + std::to_string(i) +
                   ", uc=" + std::to_string(uc));
      CheckScatterRecordValidity(srec);
      ASSERT_TRUE(srec.sampled_type == BxDFFlags::GlossyReflection ||
                  srec.sampled_type == BxDFFlags::GlossyTransmission)
          << "Sampled type is not glossy for rough surface.";
    }
  }
}

TEST_F(MFDielectricTest, RoughPathInternalIncidenceNoNaNsOrInfs) {
  MFDielectric bxdf(1.5f, 0.3f, 0.3f);
  // ASSERT_FALSE(bxdf.mfDistrib.effectivelySmooth());

  gl::vec3 wo_world = gl::vec3(0.01f, 0.01f, -0.99f).normalize();
  Ray ray_in = CreateRayForWoWorld(wo_world);

  int num_samples = 100;
  std::uniform_real_distribution<float> dist_uc(0.f, 1.f);
  std::uniform_real_distribution<float> dist_u(0.f, 1.f);

  for (int i = 0; i < num_samples; ++i) {
    srec = ScatterRecord();
    float uc = dist_uc(rng);
    gl::vec2 u_sample = {dist_u(rng), dist_u(rng)};

    bool scattered = bxdf.scatter(ray_in, rec, srec, uc, u_sample);

    if (scattered) {
      SCOPED_TRACE("Sample " + std::to_string(i) +
                   ", uc=" + std::to_string(uc));
      CheckScatterRecordValidity(srec);
      ASSERT_TRUE(srec.sampled_type == BxDFFlags::GlossyReflection ||
                  srec.sampled_type == BxDFFlags::GlossyTransmission)
          << "Sampled type is not glossy for rough surface.";
    }
  }
}

TEST_F(MFDielectricTest, RoughPathVeryLowRoughnessBehavesLikeSmooth) {
  float low_roughness = 1e-5f;
  MFDielectric bxdf_rough_like_smooth(1.5f, low_roughness, low_roughness);

  gl::vec3 wo_world = gl::vec3(0.0f, 0.0f, 1.0f);
  Ray ray_in = CreateRayForWoWorld(wo_world);

  float expected_R_smooth = gl::square((1.5f - 1.0f) / (1.5f + 1.0f));
  float expected_T_smooth = 1.0f - expected_R_smooth;

  srec = ScatterRecord();
  bool scattered_refl =
      bxdf_rough_like_smooth.scatter(ray_in, rec, srec, 0.01f);
  ASSERT_TRUE(scattered_refl);
  if (bxdf_rough_like_smooth.effectivelySmooth()) {
    EXPECT_EQ(srec.sampled_type, BxDFFlags::SpecularReflection);
    CheckScatterRecordValidity(srec, srec.pdf_val > 0.0f);
    EXPECT_NEAR(srec.pdf_val, expected_R_smooth, 1e-5f);
  } else {
    EXPECT_EQ(srec.sampled_type, BxDFFlags::GlossyReflection);
    CheckScatterRecordValidity(srec);
  }
  EXPECT_NEAR(srec.sampled_ray.getDirection().z(), 1.0f, 1e-3f);

  srec = ScatterRecord();
  bool scattered_trans =
      bxdf_rough_like_smooth.scatter(ray_in, rec, srec, 0.99f);
  ASSERT_TRUE(scattered_trans);
  if (bxdf_rough_like_smooth.effectivelySmooth()) {
    EXPECT_EQ(srec.sampled_type, BxDFFlags::SpecularTransmission);
    CheckScatterRecordValidity(srec, srec.pdf_val > 0.0f);
    EXPECT_NEAR(srec.pdf_val, expected_T_smooth, 1e-5f);
  } else {
    EXPECT_EQ(srec.sampled_type, BxDFFlags::GlossyTransmission);
    CheckScatterRecordValidity(srec);
  }
  EXPECT_NEAR(srec.sampled_ray.getDirection().z(), -1.0f, 1e-3f);
};
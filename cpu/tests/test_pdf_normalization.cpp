#include "PDF/clearcoatPDF.hpp"
#include "PDF/mfPDF.hpp"
#include "pdf_test.hpp"
#include "PDF/hairPDF.hpp"
#include "PDF/mfDielectricPDF.hpp"
#include "material/commonMaterials.hpp"
#include <gtest/gtest.h>
TEST(AnyPDFNormalization, ClearcoatPDFTest)
{
    using namespace gl; // Or your specific namespace for vec3 etc.

    // 1. Set up your PDF object to be tested
    vec3 wo_world(0.0f, 1.0f, 0.0f); // Example outgoing direction
    wo_world = normalize(wo_world);  // Ensure it's normalized

    ClearcoatTRD distrib(0.5f);               // Example roughness 0.5
    OrthoBasis basis(vec3(0.0f, 0.0f, 1.0f)); // Example basis, surface normal along Z
    ClearcoatPDF pdf_to_test(distrib, basis, wo_world);

    // 2. Call the generic test function
    // The first argument is a lambda that calls the .at() method of your PDF object.
    PDFNormalizationTestResult result = test_sphere_pdf_normalization(
        [&](const gl::vec3 &sampled_wi_world)
        {
            return pdf_to_test.at(sampled_wi_world);
        },
        200000, // Number of samples
        1.0,    // Expected integral value
        0.02    // Tolerance
    );

    // 3. Use Google Test assertions to check the results
    EXPECT_EQ(result.negative_pdf_count, 0)
        << "Encountered " << result.negative_pdf_count << " negative PDF values.";

    EXPECT_NEAR(result.integral_estimate, 0.5, result.tolerance)
        << "PDF Normalization failed: integral estimate is " << result.integral_estimate
        << " (expected ~1.0). Tested with " << result.num_samples << " samples.";

    // Optional: Print success message if using this outside of a simple pass/fail test
    if (result.passed_negative_check && result.passed_integral_check)
    {
        std::cout << "PDF Normalization Test for ClearcoatPDF PASSED. Estimate: "
                  << result.integral_estimate << std::endl;
    }
}

TEST(AnyPDFNormalization, MicrofacetPDFTest)
{
    using namespace gl; // Or your specific namespace for vec3 etc.

    // 1. Set up your PDF object to be tested
    vec3 wo_world(0.2f);            // Example outgoing direction
    wo_world = normalize(wo_world); // Ensure it's normalized

    TrowbridgeReitzDistribution distrib(0.05f, 0.05f);
    OrthoBasis basis(vec3(0.0f, 0.0f, 1.0f)); // Example basis, surface normal along Z
    MicrofacetPDF pdf_to_test(distrib, basis, wo_world);

    // 2. Call the generic test function
    // The first argument is a lambda that calls the .at() method of your PDF object.

    auto result = test_microfacet_area(pdf_to_test, 1000000);
    EXPECT_EQ(result.negative_pdf_count, 0);
    EXPECT_NEAR(result.integral_estimate, 2.0 * M_PI, result.tolerance);

    EXPECT_EQ(result.negative_pdf_count, 0)
        << "Encountered " << result.negative_pdf_count << " negative PDF values.";

    // Optional: Print success message if using this outside of a simple pass/fail test
    if (result.passed_negative_check && result.passed_integral_check)
    {
        std::cout << "PDF Normalization Test for ClearcoatPDF PASSED. Estimate: "
                  << result.integral_estimate << std::endl;
    }
}

TEST(AnyPDFNormalization, HairPDFTest)
{
    using namespace gl; // Or your specific namespace for vec3 etc.

    // 1. Set up your PDF object to be tested
    vec3 wo_world(0.3f, 0.5f, 0.4f);          // Example outgoing direction
    wo_world = normalize(wo_world);           // Ensure it's normalized
    OrthoBasis basis(vec3(0.0f, 0.0f, 1.0f)); // Example basis, surface normal along Z

    auto hair_mat = std::dynamic_pointer_cast<HairMarschner>(MARSCH_HAIR);
    HairPDF hair_pdf(*hair_mat, basis, wo_world);

    // 2. Call the generic test function
    // The first argument is a lambda that calls the .at() method of your PDF object.
    PDFNormalizationTestResult result = test_sphere_pdf_normalization(
        [&](const gl::vec3 &sampled_wi_world)
        {
            return hair_pdf.at(sampled_wi_world);
        },
        1000000, // Number of samples
        1.0,     // Expected integral value
        0.02     // Tolerance
    );

    // 3. Use Google Test assertions to check the results
    EXPECT_EQ(result.negative_pdf_count, 0)
        << "Encountered " << result.negative_pdf_count << " negative PDF values.";

    EXPECT_NEAR(result.integral_estimate, 1.0, result.tolerance)
        << "PDF Normalization failed: integral estimate is " << result.integral_estimate
        << " (expected ~1.0). Tested with " << result.num_samples << " samples.";

    // Optional: Print success message if using this outside of a simple pass/fail test
    if (result.passed_negative_check && result.passed_integral_check)
    {
        std::cout << "PDF Normalization Test for ClearcoatPDF PASSED. Estimate: "
                  << result.integral_estimate << std::endl;
    }
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
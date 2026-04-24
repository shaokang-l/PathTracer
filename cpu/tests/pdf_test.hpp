#pragma once
#include <vector>
#include <numeric>
#include <cmath>
#include <algorithm> // std::max
#include <iostream>
#include "utils/utility.hpp"

struct PDFNormalizationTestResult
{
    double integral_estimate;
    int negative_pdf_count;
    int num_samples;
    bool passed_negative_check;
    bool passed_integral_check;
    double tolerance;
};

/**
 * @brief Tests that ∫_{sphere} p(w) dΩ = 1 by uniform-sphere sampling.
 *
 * PDFEvaluator must return p(w) for any wi.  We sample
 *   wi ∼ q(w)=1/(4π) on S²,
 * form the weight p(w)/q(w), average, and get the integral.
 */
template <typename PDFEvaluator>
PDFNormalizationTestResult test_sphere_pdf_normalization(
    PDFEvaluator evaluate_pdf_at,
    int num_samples = 200000,
    double expected_integral = 1.0,
    double tolerance = 0.1)
{
    double sum_weights = 0.0;
    int negative_count = 0;

    // uniform-sphere PDF
    const double q_uniform = 1.0 / (4.0 * M_PI);

    for (int i = 0; i < num_samples; ++i)
    {
        float u1 = gl::rand_num();
        float u2 = gl::rand_num();
        // sample wi uniformly on the full sphere
        gl::vec3 wi = gl::uniformSampleSphere(u1, u2);

        float pdf_val = evaluate_pdf_at(wi);
        if (pdf_val < 0.0f)
        {
            ++negative_count;
            pdf_val = 0.0f;
        }

        // accumulate p(w)/q(w)
        sum_weights += static_cast<double>(pdf_val) / q_uniform;
    }

    // average gives the integral over the sphere
    double integral_estimate = sum_weights / static_cast<double>(num_samples);

    PDFNormalizationTestResult result;
    result.integral_estimate = integral_estimate;
    result.negative_pdf_count = negative_count;
    result.num_samples = num_samples;
    result.tolerance = tolerance;
    result.passed_negative_check = (negative_count == 0);
    result.passed_integral_check = (std::abs(integral_estimate - expected_integral) <= tolerance);
    return result;
}

template <typename PDF>
PDFNormalizationTestResult test_microfacet_area(
    PDF &pdf,
    int num_samples = 200000,
    double expected_area = 2.0 * M_PI,
    double tolerance = 0.1)
{
    double sum_inv = 0.0;
    int negs = 0;

    for (int i = 0; i < num_samples; ++i)
    {
        // wi is drawn from p(w)
        gl::vec3 wi = pdf.get();
        double pval = pdf.at(wi);

        if (pval <= 0.0)
        {
            ++negs;
            continue; // skip invalid or below-hemisphere samples
        }
        // accumulate 1/p(w)
        sum_inv += 1.0 / pval;
    }

    // average of 1/p(w) converges to ∫ dΩ = hemisphere area = 2π
    double area_estimate = sum_inv / double(num_samples);

    return {
        area_estimate,
        negs,
        num_samples,
        (negs == 0),
        (std::abs(area_estimate - expected_area) <= tolerance),
        tolerance};
}
#pragma once
#include "utils/utility.hpp"
namespace gl
{

  static float fresnelDielectric(float cosTheta_i, float eta)
  {
    cosTheta_i = std::clamp(cosTheta_i, -1.f, 1.f);

    if (cosTheta_i < 0)
    {
      eta = 1 / eta;
      cosTheta_i = -cosTheta_i;
    }

    float sin2Theta_i = 1 - square(cosTheta_i);
    float sin2Theta_t = sin2Theta_i / square(eta);
    // the total internal reflection case
    if (sin2Theta_t >= 1)
      return 1.f;
    float cosTheta_t = safeSqrt(1 - sin2Theta_t);

    float r_parl =
        (eta * cosTheta_i - cosTheta_t) / (eta * cosTheta_i + cosTheta_t);
    float r_perp =
        (cosTheta_i - eta * cosTheta_t) / (cosTheta_i + eta * cosTheta_t);
    return (square(r_parl) + square(r_perp)) / 2;
  }

  static float fresnelComplex(float cosTheta_i, std::complex<float> eta)
  {

    using Complex = std::complex<float>;
    cosTheta_i = std::clamp(cosTheta_i, 0.f, 1.f);
    float sin2Theta_i = 1 - square(cosTheta_i);
    Complex sin2Theta_t = sin2Theta_i / square(eta);
    Complex cosTheta_t = std::sqrt(Complex(1.0f, 0.0f) - sin2Theta_t);

    Complex r_parl =
        (eta * cosTheta_i - cosTheta_t) / (eta * cosTheta_i + cosTheta_t);
    Complex r_perp =
        (cosTheta_i - eta * cosTheta_t) / (cosTheta_i + eta * cosTheta_t);

    return (std::norm(r_parl) + std::norm(r_perp)) / 2.f;
  };

  static vec3 fresnelComplex(float cosTheta_i, vec3 eta, vec3 k)
  {
    vec3 result;
    for (int i = 0; i < 3; ++i)
      result[i] = fresnelComplex(cosTheta_i, std::complex<float>(eta[i], k[i]));
    return result;
  }

  static inline vec3 fresnelSchlick(float abs_cos_theta, vec3 F0)
  {
    return F0 + (vec3(1.0) - F0) * pow(1.0 - abs_cos_theta, 5.0);
  };

  static inline float hackedSchlick(float abs_cos_theta, float F90)
  {
    return 1.f + (F90 - 1.f) * pow(1.f - abs_cos_theta, 5.f);
  };

  static inline float computeFd90(const gl::vec3 &wo, const gl::vec3 &wi, const float roughness)
  {
    vec3 wh = (wo + wi).normalize();
    float wh_wi = dot(wh, wi);
    return 0.5f + 2.f * roughness * pow(fabs(wh_wi), 2.f);
  }

  static inline float computeFss90(const gl::vec3 &wo, const gl::vec3 &wi, const float roughness)
  {
    vec3 wh = (wo + wi).normalize();
    float wh_wi = dot(wh, wi);
    return roughness * pow(fabs(wh_wi), 2.f);
  }

  static inline float etaToR0(float eta)
  {
    float upper = eta - 1;
    float lower = eta + 1;
    return gl::square(upper / lower);
  }

  static inline float geometrySchlickGGX(float NdotV, float roughness)
  {
    float a = roughness;
    float k = (a * a) / 2.0;

    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
  };

  static inline float geometrySmith(gl::vec3 N, gl::vec3 V, gl::vec3 L, float roughness)
  {
    float NdotV = std::max(dot(N, V), 0.0f);
    float NdotL = std::max(dot(N, L), 0.0f);
    float ggx2 = geometrySchlickGGX(NdotV, roughness);
    float ggx1 = geometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
  };

}; // namespace gl

class TrowbridgeReitzDistribution
{

private:
  float alpha_x, alpha_y;

public:
  TrowbridgeReitzDistribution(float alpha_x, float alpha_y)
      : alpha_x(alpha_x), alpha_y(alpha_y) {}

  static TrowbridgeReitzDistribution fromRoughnessAnisotropic(float roughness, float anisotropic)
  {
    // Ensure anisotropic is clamped to prevent sqrt of negative if not handled by safeSqrt
    anisotropic = std::max(0.0f, std::min(anisotropic, 1.0f / 0.9f - 0.00001f)); // Example clamping
    float aspect = gl::safeSqrt(1.0f - 0.9f * anisotropic);

    // Prevent division by zero if aspect can be zero
    if (aspect == 0.0f)
      aspect = 0.0001f; // Or handle error appropriately

    float calculated_alpha_x = std::max(0.0001f, gl::square(roughness) / aspect);
    float calculated_alpha_y = std::max(0.0001f, gl::square(roughness) * aspect);
    return TrowbridgeReitzDistribution(calculated_alpha_x, calculated_alpha_y);
  }

  float D(gl::vec3 wm) const
  {
    using namespace gl;
    float _tan2Theta = pbrt::tan2Theta(wm);
    if (isInf(_tan2Theta))
      return 0;
    float _cos4Theta = square(pbrt::cos2Theta(wm));
    float e = _tan2Theta * (square(pbrt::cosPhi(wm) / alpha_x) +
                            square(pbrt::sinPhi(wm) / alpha_y));
    return 1 / (M_PI * alpha_x * alpha_y * _cos4Theta * square(1 + e));
  }

  bool effectivelySmooth() const { return std::max(alpha_x, alpha_y) < 1e-3f; }

  float G1(gl::vec3 w) const { return 1 / (1 + lambda(w)); }

  float lambda(gl::vec3 w) const
  {
    using namespace gl;
    float _tan2Theta = pbrt::tan2Theta(w);
    if (isInf(_tan2Theta))
      return 0;
    float alpha2 =
        square(pbrt::cosPhi(w) * alpha_x) + square(pbrt::sinPhi(w) * alpha_y);
    return (std::sqrt(1 + alpha2 * _tan2Theta) - 1) / 2;
  }

  float G(gl::vec3 wo, gl::vec3 wi) const
  {
    return 1 / (1 + lambda(wo) + lambda(wi));
  }

  float D(gl::vec3 w, gl::vec3 wm) const
  {
    using namespace gl;
    return G1(w) / pbrt::absCosTheta(w) * D(wm) * fabs(dot(w, wm));
  }

  // how likely you are to sample a bump-normal of orientation ωₘ when your rays
  // come from ω.
  float PDF(gl::vec3 w, gl::vec3 wm) const { return D(w, wm); }

  // given a direction w, sample a bump's normal wm. u is a uniform random
  // number in [0,1)
  gl::vec3 sample_wm(gl::vec3 w, gl::vec2 u) const
  {
    using namespace gl;
    vec3 wh = vec3(alpha_x * w.x(), alpha_y * w.y(), w.z()).normalize();
    if (wh.z() < 0)
      wh = -wh;

    vec3 T1 = (wh.z() < 0.99999f) ? (cross(vec3(0, 0, 1), wh)).normalize()
                                  : vec3(1, 0, 0);
    vec3 T2 = cross(wh, T1);

    vec2 p = sampleUniformDiskPolar(u);

    float h = std::sqrt(1 - square(p.x()));
    p.y() = lerp(h, p.y(), (1 + wh.z()) / 2);

    float pz = std::sqrt(std::max<float>(0, 1 - square(vec2(p).length())));

    vec3 nh = p.x() * T1 + p.y() * T2 + pz * wh;
    return vec3(alpha_x * nh.x(), alpha_y * nh.y(),
                std::max<float>(1e-6f, nh.z()))
        .normalize();
  }

  static float roughnessToAlphaPBRT(float roughness)
  {
    return std::sqrt(roughness);
  }

  void regularize()
  {
    if (alpha_x < 0.3f)
      alpha_x = std::clamp(2 * alpha_x, 0.1f, 0.3f);
    if (alpha_y < 0.3f)
      alpha_y = std::clamp(2 * alpha_y, 0.1f, 0.3f);
  };
};

class ClearcoatTRD
{
private:
  float alpha;

public:
  ClearcoatTRD(float alpha) : alpha(alpha) {};

  static ClearcoatTRD fromClearcoatGloss(float clearcoat_gloss)
  {
    float alpha = (1.f - clearcoat_gloss) * 0.1f + clearcoat_gloss * 0.001f;
    return ClearcoatTRD(alpha);
  }

  float D(gl::vec3 h) const
  {
    float upper = alpha * alpha - 1;
    if (alpha <= 0.f)
      throw std::runtime_error("alpha is too small");
    float lower = M_PI * log(gl::square(alpha));
    float term = 1 + (gl::square(alpha) - 1) * gl::square(h.z());
    return upper / (lower * term);
  }

  gl::vec3 F(float abs_h_dot_wi) const
  {
    float R0 = gl::etaToR0(1.5f);
    return gl::fresnelSchlick(abs_h_dot_wi, R0);
  }

  float G(gl::vec3 wo, gl::vec3 wi) const
  {
    return 1.f / (1.f + lambda(wo) + lambda(wi));
  }

  float lambda(gl::vec3 w) const
  {
    using namespace gl;
    float _tan2Theta = pbrt::tan2Theta(w);
    if (isInf(_tan2Theta))
      return 0;
    float alpha2 =
        square(pbrt::cosPhi(w) * 0.25) + square(pbrt::sinPhi(w) * 0.25);
    return (std::sqrt(1 + alpha2 * _tan2Theta) - 1) / 2;
  }

  gl::vec3 sample_h(gl::vec2 u) const
  {
    using namespace gl;

    float upper = 1 - pow(square(alpha), 1 - u.x());
    float lower = 1 - square(alpha);
    float cos_h_elevation = safeSqrt(upper / lower);
    float sin_h_elevation = safeSqrt(1 - square(cos_h_elevation));
    float h_azimuth = 2 * M_PI * u.y();

    vec3 h_l;
    h_l.x() = sin_h_elevation * cos(h_azimuth);
    h_l.y() = sin_h_elevation * sin(h_azimuth);
    h_l.z() = cos_h_elevation;

    return h_l;
  };
};
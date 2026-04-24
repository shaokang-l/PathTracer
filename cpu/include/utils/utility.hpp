#pragma once
#include "./constants.hpp"
#include "./matrix.hpp"
#include "./plane.hpp"
#include "./transformations.hpp"
#include <chrono>
#include <complex>
#include <float.h>
#include <random>
#include <type_traits>

template <typename E>
constexpr auto to_underlying(E e) noexcept
{
  return static_cast<std::underlying_type_t<E>>(e);
}

// This file defines shader utilities and some math functions
namespace gl
{
  template <typename T, int N>
  static T maxComponent(const vec<N, T> &v)
  {
    return *std::max_element(v.data.begin(), v.data.end());
  }

  template <typename T, int N>
  static T absDot(const vec<N, T> &v1, const vec<N, T> &v2)
  {
    return std::abs(dot(v1, v2));
  }

  static inline float attenuate(float distance)
  {
    return std::min(
        1.f, 1.f / (0.25f + 0.1f * distance + 0.01f * distance * distance));
  }

  static inline vec3 reflect(const vec3 &v, const vec3 &n)
  {
    auto _v = v.normalize();
    auto _n = n.normalize();
    auto dt = dot(_v, _n);
    return (_v - 2 * dt * _n).normalize();
  }

  static inline vec3 refract(const vec3 &v, const vec3 &n, float ni_over_nt,
                             bool &is_refract)
  {

    auto _v = v.normalize();
    auto _n = n.normalize();
    auto dt = dot(_v, _n);
    auto discriminant = 1.0 - ni_over_nt * ni_over_nt * (1 - dt * dt);
    if (discriminant > 0)
    {
      is_refract = true;
      return ni_over_nt * (_v - n * dt) - _n * sqrt(discriminant);
    }
    else
    {
      is_refract = false;
      return reflect(_v, _n);
    }
  }

  static inline bool refract(const vec3 &wo, const vec3 &n, float eta,
                             float &etaScale, vec3 &wi)
  {
    // call your existing function
    bool didRefract;
    vec3 tdir = refract(wo, n, eta, didRefract);
    if (!didRefract)
      return false;
    wi = tdir;
    etaScale = eta; // pass back the same ηᵢ/ηₜ ratio you used
    return true;
  }

  static inline vec3 refract(const vec3 &dir, const vec3 &n, float ni_over_nt)
  {
    auto cos_theta = fmin(dot(-dir, n), 1.0);
    vec3 r_out_perp = ni_over_nt * (dir + cos_theta * n);
    vec3 r_out_parallel = -sqrt(fabs(1.0 - dot(r_out_perp, r_out_perp))) * n;
    return r_out_perp + r_out_parallel;
  }

  static inline float sign(float x)
  {
    if (x < 0.f)
      return -1.f;
    if (x > 0.f)
      return 1.f;
    return 0.f;
  }

  static float safeASin(float x) { return std::asin(std::clamp(x, -1.0f, 1.0f)); }
  static float safeACos(float x) { return std::acos(std::clamp(x, -1.0f, 1.0f)); }
  static float safeSqrt(float x) { return std::sqrt(std::max(0.f, x)); }

  template <typename T, int N>
  static inline vec<N, T> safeSqrt(vec<N, T> x)
  {
    vec<N, T> res;
    for (int i = 0; i < N; i++)
    {
      res[i] = std::sqrt(std::max(0.f, x[i]));
    }
    return res;
  }

  inline int solveQuadratic(float A, float B, float C, float roots[2])
  {
    float disc = B * B - 4 * A * C;
    if (disc < 0)
      return 0;
    float sq = sqrt(disc);
    float q = B < 0 ? -0.5f * (B - sq) : -0.5f * (B + sq);
    roots[0] = q / A;
    roots[1] = C / q;
    if (roots[0] > roots[1])
      std::swap(roots[0], roots[1]);
    return roots[0] == roots[1] ? 1 : 2;
  }

  inline float BitsToFloat(uint32_t ui) { return std::bit_cast<float>(ui); }
  inline uint32_t floatToBits(float f) { return std::bit_cast<uint32_t>(f); }
  // get base-2 exponent
  inline int exponent(float v) { return (floatToBits(v) >> 23) - 127; }

  inline float nextFloatDown(float x)
  {
    return std::nextafter(x, -std::numeric_limits<float>::infinity());
  };

  inline float nextFloatUp(float x)
  {
    return std::nextafter(x, std::numeric_limits<float>::infinity());
  };

  template <typename T, int N>
  std::vector<T> to_vector(const std::array<T, N> &data)
  {
    return std::vector<T>(data.begin(), data.end());
  }

  template <typename T>
  std::vector<T> move_to_vector(std::vector<T> &&data)
  {
    return std::vector<T>(std::make_move_iterator(std::move(data.begin())),
                          std::make_move_iterator(std::move(data.end())));
  }

  // PBRT impl
  inline int sampleDiscrete(const std::vector<float> &weights, float u,
                            float *pmf, float *u_remapped)
  {
    if (weights.size() == 0)
    {
      if (pmf)
        *pmf = 0;
      return -1;
    }

    float sumWeights = 0.f;
    for (const auto &w : weights)
    {
      sumWeights += w;
    }

    float up = u * sumWeights;
    if (up == sumWeights)
      up = std::nextafter(up, -std::numeric_limits<float>::infinity());

    // find offsets
    int offset = 0;
    float sum = 0.f;
    while (sum + weights[offset] <= up)
    {
      sum += weights[offset];
      ++offset;
    }

    if (pmf)
      *pmf = weights[offset] / sumWeights;
    if (u_remapped)
      *u_remapped = std::min((up - sum) / weights[offset], (float)0x1.fffffep-1);

    return offset;
  };

  // per channel exp
  inline vec3 exp(const vec3 &v)
  {
    return vec3(std::exp(v.x()), std::exp(v.y()), std::exp(v.z()));
  }

  template <typename C>
  constexpr float evalPolynomial(float t, C c) { return c; }

  template <typename C, typename... Args>
  constexpr float evalPolynomial(float t, C c, Args... cRemaining)
  {
    return std::fma(t, evalPolynomial(t, cRemaining...), c);
  }

  // square and cube functions
  template <typename T>
  static inline T square(T x) { return x * x; }
  template <typename T>
  static inline T cube(T x) { return x * x * x; }

  static inline float I0f(float x, float tol = 1e-6f, int maxIter = 500)
  {
    float term = 1.0f; // term_0
    float sum = term;  // running sum
    float x2 = x * x;

    for (int k = 1; k < maxIter; ++k)
    {
      // term_k = term_{k-1} * (x^2) / (4 k^2)
      term *= x2 / (4.0f * k * k);
      sum += term;
      if (term < tol * sum)
        break;
      // guard against extreme overflow
      if (sum > FLT_MAX * 0.5f)
        break;
    }
    return sum;
  }

  static inline float logI0f(float x)
  {
    if (x > 12.0f)
    {
      // float-precision pi
      const float pi = 3.14159265358979323846f;
      // asymptotic: I0(x) ~ exp(x) / sqrt(2*pi*x)
      // so log I0 ~ x - 0.5*log(2*pi*x)
      return x - 0.5f * std::logf(2.0f * pi * x);
    }
    // for smaller x, just take log of the series result
    return std::logf(I0f(x));
  }

  // fast exp
  static inline float fastExp(float x)
  {

    float xp = x * 1.442695041f;
    float fxp = floor(xp);
    float f = xp - fxp;
    int i = (int)fxp;

    float twoToF =
        evalPolynomial(f, 1.f, 0.695556856f, 0.226173572f, 0.0781455737f);
    int exp = exponent(twoToF) + i;
    if (exp < -126)
      return 0;
    if (exp > 127)
      throw std::runtime_error("exponent out of range");

    uint32_t bits = floatToBits(twoToF);
    bits &= 0b10000000011111111111111111111111u;
    bits |= (exp + 127) << 23;
    return BitsToFloat(bits);
  };

  inline vec3 fastExp(const vec3 &v)
  {
    return vec3(fastExp(v.x()), fastExp(v.y()), fastExp(v.z()));
  }

  static inline float logistic(float x, float s)
  {
    x = std::abs(x);
    return std::exp(-x / s) / (s * square(1 + std::exp(-x / s)));
  }

  static inline float logisticCDF(float x, float s)
  {
    return 1 / (1 + std::exp(-x / s));
  }

  static inline float trimmedLogistic(float x, float s, float a, float b)
  {
    return logistic(x, s) / (logisticCDF(b, s) - logisticCDF(a, s));
  }

  static inline float lerp(float x, float y, float t) { return x + t * (y - x); }

  static inline float sampleLogistic(float u, float s)
  {
    return -s * std::log(1 / u - 1);
  }

  static inline float invertLogisticSample(float x, float s)
  {
    return 1 / (1 + std::exp(-x / s));
  }

  static inline float sampleTrimmedLogistic(float u, float s, float a, float b)
  {
    auto P = [&](float x)
    { return invertLogisticSample(x, s); };
    u = lerp(P(a), P(b), u);
    float x = sampleLogistic(u, s);
    return std::clamp(x, a, b);
  }

  template <int N>
  static inline vec<N, float> lerp(vec<N, float> x, vec<N, float> y, float t)
  {
    vec<N, float> res;

    for (int i = 0; i < N; i++)
    {
      res[i] = lerp(x[i], y[i], t);
    }

    return res;
  }

  template <int N>
  static inline vec<N, float> sign(vec<N, float> v)
  {
    vec<N, float> res;
    for (int i = 0; i < N; i++)
    {
      res[i] = sign(v[i]);
    }
    return res;
  }

  template <int N>
  static inline vec<N, float> abs(vec<N, float> v)
  {
    vec<N, float> res;
    for (int i = 0; i < N; i++)
    {
      res[i] = fabs(v[i]);
    }

    return res;
  }

  template <int N>
  static inline vec<N, float> min(vec<N, float> lhs, vec<N, float> rhs)
  {
    vec<N, float> res;
    for (int i = 0; i < N; i++)
    {
      res[i] = std::min(lhs[i], rhs[i]);
    }
    return res;
  }

  template <int N>
  static inline vec<N, float> max(vec<N, float> lhs, vec<N, float> rhs)
  {
    vec<N, float> res;
    for (int i = 0; i < N; i++)
    {
      res[i] = std::max(lhs[i], rhs[i]);
    }
    return res;
  }

  static inline float smoothstep(float edge0, float edge1, float x)
  {
    x = std::clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
    return x * x * (3 - 2 * x);
  };

  static inline float clamp(float x, float min, float max)
  {
    return std::clamp(x, min, max);
  }

  static inline vec2 clamp(vec2 v, float min, float max)
  {
    return vec2(std::clamp(v.x(), min, max), std::clamp(v.y(), min, max));
  }

  static inline vec3 clamp(vec3 v, float min, float max)
  {
    return vec3(std::clamp(v.x(), min, max), std::clamp(v.y(), min, max),
                std::clamp(v.z(), min, max));
  }

  static inline float smoothstep_alt(float edge0, float edge1, float x)
  {
    x = std::clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
    return x * x * x * (x * (x * 6 - 15) + 10);
  };

  static inline float fract(float x) { return x - floor(x); }

  static const int Primes[] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41,
                               43, 47, 53, 59, 61, 67, 71, 73, 79, 83, 89, 97};
  static const int PrimeCount = sizeof(Primes) / sizeof(Primes[0]);

  static constexpr float OneMinusEpsilon = 0x1.fffffep-1;

  static float radicalInverse(int baseIndex, uint64_t a)
  {
    int base = Primes[baseIndex % PrimeCount];
    float invBase = (float)1 / (float)base, invBaseM = 1;
    uint64_t reversedDigits = 0;
    while (a)
    {
      uint64_t next = a / base;
      uint64_t digit = a - next * base;
      reversedDigits = reversedDigits * base + digit;
      invBaseM *= invBase;
      a = next;
    }
    return std::min(reversedDigits * invBaseM, OneMinusEpsilon);
  };

  static uint64_t inverseRadicalInverse(uint64_t inverse, int base, int nDigits)
  {
    uint64_t index = 0;
    for (int i = 0; i < nDigits; ++i)
    {
      uint64_t digit = inverse % base;
      inverse /= base;
      index = index * base + digit;
    }
    return index;
  }

  inline int permutationElement(uint32_t i, uint32_t l, uint32_t p)
  {
    uint32_t w = l - 1;
    w |= w >> 1;
    w |= w >> 2;
    w |= w >> 4;
    w |= w >> 8;
    w |= w >> 16;
    do
    {
      i ^= p;
      i *= 0xe170893d;
      i ^= p >> 16;
      i ^= (i & w) >> 4;
      i ^= p >> 8;
      i *= 0x0929eb3f;
      i ^= p >> 23;
      i ^= (i & w) >> 1;
      i *= 1 | p >> 27;
      i *= 0x6935fa69;
      i ^= (i & w) >> 11;
      i *= 0x74dcb303;
      i ^= (i & w) >> 2;
      i *= 0x9e501cc3;
      i ^= (i & w) >> 2;
      i *= 0xc860a3df;
      i &= w;
      i ^= i >> 5;
    } while (i >= l);
    return (i + p) % l;
  }

  static inline float VanDerCorput(uint bits)
  {
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10; //
  }

  static inline gl::vec2 Hammersley(uint i, uint N)
  {
    return gl::vec2(float(i) / float(N), VanDerCorput(i));
  }

  static float step(float edge, float x) { return x < edge ? 0.0f : 1.0f; }

  // convolve kernel
  static float filtered_step(float edge, float x, float w)
  {
    return std::clamp((x - edge + w / 2.0f) / w, 0.0f, 1.0f);
  }

  static float tri_sig(float x)
  {
    float h = fract(x * 0.5f) - 0.5f;
    return 1.0f - 2.0f * fabs(h);
  }

  template <int N>
  static vec<N, float> tri_sig(vec<N, float> v)
  {
    vec<N, float> res;
    for (int i = 0; i < N; i++)
    {
      res[i] = tri_sig(v[i]);
    }
    return res;
  }

  static float sign(gl::vec2 p1, gl::vec2 p2, gl::vec2 p3)
  {
    return (p1.x() - p3.x()) * (p2.y() - p3.y()) -
           (p2.x() - p3.x()) * (p1.y() - p3.y());
  }

  // check if a point is inside a rectangle
  // note that p1 p2 p3 p4 must be in clockwise order or counter clockwise order
  static bool is_inside_rect(gl::vec2 sample, gl::vec2 p1, gl::vec2 p2,
                             gl::vec2 p3, gl::vec2 p4)
  {
    auto d1 = sign(sample, p1, p2);
    auto d2 = sign(sample, p2, p3);
    auto d3 = sign(sample, p3, p4);
    auto d4 = sign(sample, p4, p1);

    auto all_neg = (d1 <= 0) && (d2 <= 0) && (d3 <= 0) && (d4 <= 0);
    auto all_pos = (d1 >= 0) && (d2 >= 0) && (d3 >= 0) && (d4 >= 0);

    return all_neg || all_pos;
  }

  static bool is_convex(gl::vec2 p1, gl::vec2 p2, gl::vec2 p3, gl::vec2 p4)
  {
    auto d1 = sign(p4, p2, p1);
    auto d2 = sign(p1, p3, p2);
    auto d3 = sign(p2, p4, p3);
    auto d4 = sign(p3, p1, p4);

    auto all_neg = (d1 <= 0) && (d2 <= 0) && (d3 <= 0) && (d4 <= 0);
    auto all_pos = (d1 >= 0) && (d2 >= 0) && (d3 >= 0) && (d4 >= 0);
    return all_neg || all_pos;
  }

  static std::random_device
      rd; // Will be used to obtain a seed for the random number engine
  static std::mt19937
      gen(rd()); // Standard mersenne_twister_engine seeded with rd()
  static std::uniform_real_distribution<> dist(0, 1);

  // random number from 0 to 1
  static float rand_num() { return dist(gen); }
  // random number from 0 to end_point
  static float rand_num(float end_point) { return dist(gen) * end_point; }

  // random number from start_point to end_point
  static inline float rand_num(float start_point, float end_point)
  {
    return start_point + (end_point - start_point) * dist(gen);
  }

  static inline int C_rand_int(int begin, int end)
  {
    return rand() % (end - begin) + begin;
  }

  static inline float C_rand() { return rand() / (RAND_MAX + 1.0); }

  static inline float C_rand(float min, float max)
  {
    // Returns a random real in [min,max).
    return min + (max - min) * C_rand();
  }

  static inline gl::vec3 C_rand_vec3()
  {
    return gl::vec3(C_rand(), C_rand(), C_rand());
  }

  static inline gl::vec3 C_rand_vec3(float min, float max)
  {
    return gl::vec3(C_rand(min, max), C_rand(min, max), C_rand(min, max));
  }

  // reject sampling
  // a uniform distribution, all points are within the circle
  static inline vec2 sampleUniformDisk(float r = 1.f)
  {
    auto p = vec2(C_rand(-1.f, 1.f), C_rand(-1.f, 1.f));

    while (p.length() >= 1.f)
    {
      p = vec2(C_rand(-1.f, 1.f), C_rand(-1.f, 1.f));
    }
    return p;
  }

  static inline vec2 sampleUniformDiskPolar(vec2 u)
  {
    float r = sqrt(u.x());
    float theta = 2 * M_PI * u.y();
    return vec2(r * cos(theta), r * sin(theta));
  };

  // a cos(theta)^3 distribution, all points are within the sphere
  static inline vec3 sphere_random_vec(float r = 1.f)
  {
    auto p = vec3(C_rand(-1.f, 1.f), C_rand(-1.f, 1.f), C_rand(-1.f, 1.f));
    return p * C_rand(-r, r);
  }

  // a cos(theta) distribution, all points are on the surface of the sphere
  static inline vec3 on_sphere_random_vec(float r = 1.f)
  {
    auto p = vec3(C_rand(-1.f, 1.f), C_rand(-1.f, 1.f), C_rand(-1.f, 1.f));
    p.normalized();
    return p * r;
  }

  // alternative distribution, hemisphere cos(theta) distribution
  static inline vec3 on_hemisphere_random_vec(const vec3 &normal, float r = 1.f)
  {
    auto p = vec3(C_rand(-1.f, 1.f), C_rand(-1.f, 1.f), C_rand(-1.f, 1.f));
    p.normalized();
    if (dot(p, normal) < 0)
    {
      p = -p;
    }
    return p;
  }

  // Helper function: Convert spherical coordinates (theta, phi) to a 3D vector
  // in local space, where the z-axis is assumed to be "up."
  inline gl::vec3 sphericalDirection(float theta, float phi)
  {
    float sinTheta = sin(theta);
    return gl::vec3(sinTheta * cos(phi), sinTheta * sin(phi), cos(theta));
  }

  inline gl::vec3 sphericalDirection(float sin_theta, float cos_theta,
                                     float phi)
  {
    return gl::vec3(sin_theta * cos(phi), sin_theta * sin(phi), cos_theta);
  }

  // biliner interpolation
  static inline float bilinear(float w1, float w2, float w3, float w4, float q1,
                               float q2, float q3, float q4)
  {
    return (w1 * q1 + w2 * q2 + w3 * q3 + w4 * q4) / (w1 + w2 + w3 + w4);
  }

  // biliner interpolation
  template <typename T>
  static inline T bilinear(float w1, float w2, float w3, float w4, T q1, T q2,
                           T q3, T q4)
  {
    return (w1 * q1 + w2 * q2 + w3 * q3 + w4 * q4) / (w1 + w2 + w3 + w4);
  }

  // biliner interpolation
  template <typename T>
  static inline T bilinear(gl::vec2 uv, T p1, T p2, T p3, T p4)
  {
    T p = p4 + (p3 - p4) * uv.u();
    T q = p1 + (p2 - p1) * uv.u();
    return p + (q - p) * uv.v();
  }

  // approximated bilinear interpolation, projected micropolygon may not be
  // rectangular
  static inline float get_depth_bilinear(gl::vec2 sample_coord, gl::vec3 p1,
                                         gl::vec3 p2, gl::vec3 p3, gl::vec3 p4)
  {
    float w1 = (p3.x() - sample_coord.x()) * (p3.y() - sample_coord.y()) /
               ((p3.x() - p1.x()) * (p3.y() - p1.y()));
    float w2 = (sample_coord.x() - p4.x()) * (p3.y() - sample_coord.y()) /
               ((p3.x() - p1.x()) * (p3.y() - p1.y()));
    float w3 = (sample_coord.x() - p1.x()) * (sample_coord.y() - p1.y()) /
               ((p3.x() - p1.x()) * (p3.y() - p1.y()));
    float w4 = (p2.x() - sample_coord.x()) * (sample_coord.y() - p1.y()) /
               ((p3.x() - p1.x()) * (p3.y() - p1.y()));
    float q1 = p1.z();
    float q2 = p2.z();
    float q3 = p3.z();
    float q4 = p4.z();
    return bilinear(w1, w2, w3, w4, q1, q2, q3, q4);
  };

  // https://stackoverflow.com/questions/808441/inverse-bilinear-interpolation
  // The good thing is the order of the points is preserved after projection
  // If the points is not passed in clockwise or counter clockwise order, the
  // result will be wrong This returns the interpolated params u,v Use u,v we
  // can interpolate depth, normal, color, etc.
  static gl::vec2 inverseBilinear(gl::vec2 sample, gl::vec2 p1, gl::vec2 p2,
                                  gl::vec2 p3, gl::vec2 p4)
  {
    using namespace gl;
    vec2 res(0.0);

    if (!is_convex(p1, p2, p3, p4))
      throw std::runtime_error("Non convex polygon!");

    vec2 e = p2 - p1;
    vec2 f = p4 - p1;
    vec2 g = p1 - p2 + p3 - p4;
    vec2 h = sample - p1;

    float k0 = h.x() * e.y() - h.y() * e.x();
    float k1 = e.x() * f.y() - e.y() * f.x() + h.x() * g.y() - h.y() * g.x();
    float k2 = g.x() * f.y() - g.y() * f.x();

    // special case: parallel edges
    if (fabs(k2) < 1e-4)
      return res = vec2((h.x() * k1 + f.x() * k0) / (e.x() * k1 - g.x() * k0),
                        -k0 / k1);

    // quadric equation

    float delta = k1 * k1 - 4 * k0 * k2;

    if (delta < 0.f)
    {
      // no solution
      std::cout << "no solution" << std::endl;
      return res = vec2(-1.f, -1.f);
    }

    delta = sqrtf(delta);
    float v = (-k1 - delta) / (2 * k2);
    float u = (h.x() - f.x() * v) / (e.x() + g.x() * v);

    if (u < 0.0 || u > 1.0 || v < 0.0 || v > 1.0)
    {
      v = (-k1 + delta) / (2 * k2);
      u = (h.x() - f.x() * v) / (e.x() + g.x() * v);
    }

    // degrade to nearest
    if (u < 0.0 || u > 1.0 || v < 0.0 || v > 1.0)
    {
      return res = vec2(0.f, 0.f);
    }

    return res = vec2(u, v);
  }

  static inline float isInf(float v) { return std::isinf(v); }

  // these are functions follows pbrt conventions
  namespace pbrt
  {
    // this one assumes wo points from the surface to the light
    static inline vec3 reflect(const vec3 &wo, const vec3 &n)
    {
      return -wo + 2 * dot(wo, n) * n;
    }

    static inline bool refract(vec3 wi, vec3 n, float eta, float &etap, vec3 &wt)
    {
      float cosTheta_i = dot(n, wi);

      if (cosTheta_i < 0)
      {
        eta = 1 / eta;
        cosTheta_i = -cosTheta_i;
        n = -n;
      }

      float sin2Theta_i = std::max<float>(0, 1 - square(cosTheta_i));
      float sin2Theta_t = sin2Theta_i / square(eta);

      if (sin2Theta_t >= 1)
        return false;

      float cosTheta_t = safeSqrt(1 - sin2Theta_t);

      wt = -wi / eta + (cosTheta_i / eta - cosTheta_t) * vec3(n);
      wt.normalized();
      etap = eta;

      return true;
    }

    static inline float cosTheta(vec3 w) { return w.z(); }
    static inline float cos2Theta(vec3 w) { return square(w.z()); }
    static inline float absCosTheta(vec3 w) { return std::abs(w.z()); }

    static inline float sin2Theta(vec3 w)
    {
      return std::max<float>(0, 1 - cos2Theta(w));
    }
    static inline float sinTheta(vec3 w) { return std::sqrt(sin2Theta(w)); }

    static inline float tanTheta(vec3 w) { return sinTheta(w) / cosTheta(w); }
    static inline float tan2Theta(vec3 w) { return sin2Theta(w) / cos2Theta(w); }

    static inline float cosPhi(vec3 w)
    {
      float _sinTheta = sinTheta(w);
      return (_sinTheta == 0) ? 1 : std::clamp(w.x() / _sinTheta, -1.f, 1.f);
    }
    static inline float sinPhi(vec3 w)
    {
      float _sinTheta = sinTheta(w);
      return (_sinTheta == 0) ? 0 : std::clamp(w.y() / _sinTheta, -1.f, 1.f);
    }

    static inline vec3 faceForward(vec3 n, vec3 v)
    {
      return (dot(n, v) < 0.f) ? -n : n;
    }

    static inline bool sameHemisphere(const vec3 &a, const vec3 &b)
    {
      return a.z() * b.z() > 0;
    }
  }; // namespace pbrt
}; // namespace gl

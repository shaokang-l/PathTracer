#pragma once
#include "utils/matrix.hpp"
#include "utils/orthoBasis.hpp"
#include "utils/random.hpp"
#include "utils/utility.hpp"

// Global counter for the number of times a PDF's sampling direction is rejected
static int rejects = 0;
class PDF
{
public:
  virtual float at(const gl::vec3 &direction) const = 0;
  virtual gl::vec3 get(float uc = gl::rand_num(),
                       gl::vec2 u = gl::vec2(gl::rand_num(),
                                             gl::rand_num())) const = 0;
  virtual ~PDF() = default;
};

class CosinePDF : public PDF
{
  OrthoBasis onb;

public:
  CosinePDF(const gl::vec3 &normal) : onb(normal) {}
  float at(const gl::vec3 &wi_world) const override
  {
    float cosine = gl::dot(wi_world.normalize(), onb.w());
    return std::max(cosine, 0.f) / M_PI;
  }

  gl::vec3 get(float uc = gl::rand_num(),
               gl::vec2 u = gl::vec2(gl::rand_num(),
                                     gl::rand_num())) const override
  {
    return onb.at(gl::cosineSampleHemiSphere(u.u(), u.v()));
  }
};

class UniformPDF : public PDF
{
public:
  UniformPDF() = default;

  float at(const gl::vec3 &wi_world) const override
  {
    return 1.0f / (4.0f * M_PI);
  }

  gl::vec3 get(float uc = gl::rand_num(),
               gl::vec2 u = gl::vec2(gl::rand_num(),
                                     gl::rand_num())) const override
  {
    return gl::uniformSampleSphere(u.u(), u.v());
  }
};

class PhongLobePDF : public PDF
{
public:
  // Constructor takes the perfect reflection direction and the shininess
  // exponent.
  PhongLobePDF(const gl::vec3 &R, float shininess)
      : R(normalize(R)), shininess(shininess), onb(R) {}

  // Return the PDF value for a given outgoing direction.
  float at(const gl::vec3 &direction) const override
  {
    float cosine = std::max(dot(normalize(direction), R), 0.0f);
    return (shininess + 1.0f) / (2.0f * M_PI) * pow(cosine, shininess);
  }

  // Sample a new direction according to the Phong-lobe distribution.
  gl::vec3 get(float uc = gl::rand_num(),
               gl::vec2 u = gl::vec2(gl::rand_num(),
                                     gl::rand_num())) const override
  {
    float u1 = u.x();
    float u2 = u.y();
    // Invert the cumulative distribution for cosine-power:
    float theta = acos(pow(u1, 1.0f / (shininess + 1.0f)));
    float phi = 2.0f * M_PI * u2;
    // Convert spherical coordinates to a local direction.
    gl::vec3 localDir = gl::sphericalDirection(theta, phi);
    return normalize(onb.at(localDir));
  }

private:
  gl::vec3 R;
  float shininess;
  OrthoBasis onb;
};

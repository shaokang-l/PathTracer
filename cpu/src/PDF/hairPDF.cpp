#include "PDF/hairPDF.hpp"
#include "material/hairMarsch.hpp"

HairPDF::HairPDF(const HairMarschner &hair_mat, const OrthoBasis &basis,
                 const gl::vec3 &wo_world)
    : hair_mat(hair_mat), basis(basis), wo_local(basis.toLocal(wo_world)) {};

// expecting a wi_world
gl::vec3 HairPDF::get(float uc, gl::vec2 u) const
{

  using namespace gl;
  float sinTheta_o = wo_local.x();
  float cosTheta_o = safeSqrt(1 - sinTheta_o * sinTheta_o);
  float phi_o = std::atan2(wo_local.z(), wo_local.y());
  float gamma_o = safeASin(sinTheta_o);

  std::array<float, HairMarschner::pMax + 1> apPDF = hair_mat.ApPDF(cosTheta_o);

  int p = sampleDiscrete(std::vector<float>(apPDF.begin(), apPDF.end()), uc,
                         nullptr, nullptr);

  float sinThetap_o, cosThetap_o;
  if (p == 0)
  {
    sinThetap_o = sinTheta_o * hair_mat.cos2kAlpha[1] -
                  cosTheta_o * hair_mat.sin2kAlpha[1];
    cosThetap_o = cosTheta_o * hair_mat.cos2kAlpha[1] +
                  sinTheta_o * hair_mat.sin2kAlpha[1];
  }
  else if (p == 1)
  {
    sinThetap_o = sinTheta_o * hair_mat.cos2kAlpha[0] +
                  cosTheta_o * hair_mat.sin2kAlpha[0];
    cosThetap_o = cosTheta_o * hair_mat.cos2kAlpha[0] -
                  sinTheta_o * hair_mat.sin2kAlpha[0];
  }
  else if (p == 2)
  {
    sinThetap_o = sinTheta_o * hair_mat.cos2kAlpha[2] +
                  cosTheta_o * hair_mat.sin2kAlpha[2];
    cosThetap_o = cosTheta_o * hair_mat.cos2kAlpha[2] -
                  sinTheta_o * hair_mat.sin2kAlpha[2];
  }
  else
  {
    sinThetap_o = sinTheta_o;
    cosThetap_o = cosTheta_o;
  }

  cosThetap_o = std::abs(cosThetap_o);

  // sample Mp to compute theta_i
  float cosTheta =
      1 +
      hair_mat.v[p] * std::log(std::max(u[0], (float)1e-5) +
                               (1.f - u[0]) * fastExp(-2.f / hair_mat.v[p]));
  float sinTheta = safeSqrt(1 - square(cosTheta));
  float cosPhi = std::cos(2 * M_PI * u[1]);

  float sinTheta_i = -cosTheta * sinThetap_o + sinTheta * cosPhi * cosThetap_o;
  float cosTheta_i = safeSqrt(1 - square(sinTheta_i));

  float etap = safeSqrt(square(hair_mat.eta) - square(sinTheta_o)) / cosTheta_o;
  float sinGamma_t = hair_mat.h / etap;
  float cosGamma_t = safeSqrt(1 - square(sinGamma_t));
  float gamma_t = safeASin(sinGamma_t);

  float dphi;
  if (p < hair_mat.pMax)
    dphi = hair_mat.Phi(p, gamma_o, gamma_t) +
           sampleTrimmedLogistic(uc, hair_mat.s, -M_PI, M_PI);
  else
    dphi = 2 * M_PI * uc;

  float phi_i = dphi + phi_o;
  vec3 wi_local(sinTheta_i, cosTheta_i * std::cos(phi_i),
                cosTheta_i * std::sin(phi_i));

  return basis.toWorld(wi_local);
}

float HairPDF::at(const gl::vec3 &wi_world) const
{
  using namespace gl;
  vec3 wi_local = basis.toLocal(wi_world.normalize());

  float sinTheta_i = wi_local.x();
  float cosTheta_i = safeSqrt(1 - sinTheta_i * sinTheta_i);
  float phi_i = std::atan2(wi_local.z(), wi_local.y());

  float sinTheta_o = wo_local.x();
  float cosTheta_o = safeSqrt(1 - sinTheta_o * sinTheta_o);
  float phi_o = std::atan2(wo_local.z(), wo_local.y());
  float gamma_o = safeASin(sinTheta_o);

  float etap = safeSqrt(square(hair_mat.eta) - square(sinTheta_o)) / cosTheta_o;
  float sinGamma_t = hair_mat.h / etap;
  float cosGamma_t = safeSqrt(1 - square(sinGamma_t));
  float gamma_t = safeASin(sinGamma_t);

  float dphi = phi_i - phi_o;
  if (dphi < -M_PI)
    dphi += 2 * M_PI;
  else if (dphi > M_PI)
    dphi -= 2 * M_PI;

  std::array<float, HairMarschner::pMax + 1> apPDF = hair_mat.ApPDF(cosTheta_o);

  float pdf = 0.f;
  for (int p = 0; p < hair_mat.pMax; ++p)
  {
    float sinThetap_o, cosThetap_o;
    if (p == 0)
    {
      sinThetap_o = sinTheta_o * hair_mat.cos2kAlpha[1] -
                    cosTheta_o * hair_mat.sin2kAlpha[1];
      cosThetap_o = cosTheta_o * hair_mat.cos2kAlpha[1] +
                    sinTheta_o * hair_mat.sin2kAlpha[1];
    }
    else if (p == 1)
    {
      sinThetap_o = sinTheta_o * hair_mat.cos2kAlpha[0] +
                    cosTheta_o * hair_mat.sin2kAlpha[0];
      cosThetap_o = cosTheta_o * hair_mat.cos2kAlpha[0] -
                    sinTheta_o * hair_mat.sin2kAlpha[0];
    }
    else if (p == 2)
    {
      sinThetap_o = sinTheta_o * hair_mat.cos2kAlpha[2] +
                    cosTheta_o * hair_mat.sin2kAlpha[2];
      cosThetap_o = cosTheta_o * hair_mat.cos2kAlpha[2] -
                    sinTheta_o * hair_mat.sin2kAlpha[2];
    }
    else
    {
      sinThetap_o = sinTheta_o;
      cosThetap_o = cosTheta_o;
    }

    cosThetap_o = std::abs(cosThetap_o);

    float Mp_val = hair_mat.Mp(cosTheta_i, cosThetap_o, sinTheta_i, sinThetap_o,
                               hair_mat.v[p]);

    float Np_val = (p < hair_mat.pMax)
                       ? hair_mat.Np(dphi, p, hair_mat.s, gamma_o, gamma_t)
                       : 1.f / (2.f * M_PI);
    pdf += apPDF[p] * Mp_val * Np_val;
  }

  return pdf;
}
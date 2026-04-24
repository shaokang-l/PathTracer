#include "material/hairMarsch.hpp"
#include "PDF/hairPDF.hpp"

bool HairMarschner::scatter(const Ray &ray_in, HitRecord &rec,
                            ScatterRecord &srec, float uc,
                            const gl::vec2 &u, // 2D microfacet sample
                            TransportMode mode,
                            BxDFReflTransFlags flags) const
{
  gl::vec3 wo_world = -ray_in.getDirection().normalize();

  // if hair_tangent is not set, use normal
  if (rec.hair_tangent.near_zero())
  {
    rec.hair_tangent = rec.normal;
  }

  OrthoBasis basis(rec.hair_tangent);
  auto pdf_ptr = std::make_shared<HairPDF>(*this, basis, wo_world);
  auto wi_world = pdf_ptr->get(uc, u).normalize();

  srec.attenuation = f(wo_world, wi_world, rec);
  srec.pdf_ptr = pdf_ptr;
  srec.pdf_val = pdf_ptr->at(wi_world);
  srec.sampled_ray = Ray(rec.position, wi_world);
  srec.sampled_type = BxDFFlags::GlossyReflection;
  return true;
};

gl::vec3 HairMarschner::evalMarschner(const gl::vec3 &wi,
                                      const gl::vec3 &wo) const
{
  // compute wo hair related term
  using namespace gl;
  float sinTheta_o = wo.x();
  float cosTheta_o = safeSqrt(1 - sinTheta_o * sinTheta_o);
  float phi_o = std::atan2(wo.z(), wo.y());
  float gamma_o = safeASin(sinTheta_o);

  float sinTheta_i = wi.x();
  float cosTheta_i = safeSqrt(1 - sinTheta_i * sinTheta_i);
  float phi_i = std::atan2(wi.z(), wi.y());

  // compute cos theta_t
  float sinTheta_t = sinTheta_o / eta;
  float cosTheta_t = safeSqrt(1 - sinTheta_t * sinTheta_t);

  // gamma_t
  float etap = safeSqrt((square(eta) - square(sinTheta_o)));
  float sinGamma_t = this->h / etap;
  float cosGamma_t = safeSqrt(1 - sinGamma_t * sinGamma_t);
  float gamma_t = safeASin(sinGamma_t);

  // Transmittance T
  vec3 T(0.f);
  T.x() = gl::fastExp(-sigma_a.x() * (2 * cosGamma_t / cosTheta_t));
  T.y() = gl::fastExp(-sigma_a.y() * (2 * cosGamma_t / cosTheta_t));
  T.z() = gl::fastExp(-sigma_a.z() * (2 * cosGamma_t / cosTheta_t));

  // calculate bsdf
  float phi = phi_i - phi_o;
  std::array<vec3, pMax + 1> ap = Ap(cosTheta_o, eta, h, T);
  vec3 bsdf(0.f);
  for (int p = 0; p < pMax; ++p)
  {

    // scale adjusting
    float sinThetap_o, cosThetap_o;
    if (p == 0)
    {
      sinThetap_o = sinTheta_o * cos2kAlpha[1] - cosTheta_o * sin2kAlpha[1];
      cosThetap_o = cosTheta_o * cos2kAlpha[1] + sinTheta_o * sin2kAlpha[1];
    }
    else if (p == 1)
    {
      sinThetap_o = sinTheta_o * cos2kAlpha[0] + cosTheta_o * sin2kAlpha[0];
      cosThetap_o = cosTheta_o * cos2kAlpha[0] - sinTheta_o * sin2kAlpha[0];
    }
    else if (p == 2)
    {
      sinThetap_o = sinTheta_o * cos2kAlpha[2] + cosTheta_o * sin2kAlpha[2];
      cosThetap_o = cosTheta_o * cos2kAlpha[2] - sinTheta_o * sin2kAlpha[2];
    }
    else
    {
      sinThetap_o = sinTheta_o;
      cosThetap_o = cosTheta_o;
    }

    cosThetap_o = std::abs(cosThetap_o);

    float mp = Mp(cosTheta_i, cosTheta_o, sinTheta_i, sinTheta_o, v[p]);
    float np = Np(phi, p, s, gamma_o, gamma_t);
    bsdf += ap[p] * mp * np;
  }

  bsdf += Mp(cosTheta_i, cosTheta_o, sinTheta_i, sinTheta_o, v[pMax]) *
          ap[pMax] / (2 * M_PI);

  if (std::abs(wi.z()) > 0.f)
    bsdf /= std::abs(wi.z());

  return bsdf;
}

float HairMarschner::scatter_pdf(const gl::vec3 &wo_world, const gl::vec3 &wi_world,
                                 const HitRecord &rec,
                                 TransportMode mode,
                                 BxDFReflTransFlags flags) const
{
  using namespace gl;
  if (!(flags & BxDFReflTransFlags::Reflection))
    return 0.f;

  auto pdf = std::make_shared<HairPDF>(*this, OrthoBasis(rec.normal), wo_world);
  return pdf->at(wi_world);
}

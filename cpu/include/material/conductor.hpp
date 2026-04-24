#pragma once
#include "material/material.hpp"
#include "PDF/mfPDF.hpp"
class Conductor : public Material
{
public:
  Conductor(const gl::vec3 &eta, const gl::vec3 &k, float alpha_x,
            float alpha_y)
      : eta(eta), k(k), mfDistribution(alpha_x, alpha_y) {}

  bool
  scatter(const Ray &ray_in, HitRecord &rec, ScatterRecord &srec,
          float uc = gl::rand_num(), // coin-flip sample
          const gl::vec2 &u = {gl::rand_num(),
                               gl::rand_num()}, // 2D microfacet sample
          TransportMode mode = TransportMode::Radiance,
          BxDFReflTransFlags flags = BxDFReflTransFlags::All) const override
  {

    using namespace gl;
    if (!(flags & BxDFReflTransFlags::Reflection))
      return false;

    vec3 wo_world = -ray_in.getDirection().normalize();

    if (mfDistribution.effectivelySmooth())
    {

      gl::vec3 wi = gl::pbrt::reflect(wo_world, rec.normal).normalize();
      if (dot(rec.normal, wi) <= 0)
        return false;

      srec.sampled_ray = Ray(rec.position, wi);
      // Fresnel reflectance at this angle:
      float absCosThetaI = fabs(dot(rec.normal, wi));
      vec3 f = fresnelComplex(absCosThetaI, eta, k);
      srec.attenuation = f;
      srec.sampled_type = BxDFFlags::SpecularReflection;
      srec.pdf_ptr = nullptr; // delta
      srec.pdf_val = 0.0f;
      return true;
    }

    // --- rough (microfacet) case ---
    OrthoBasis basis(rec.normal);
    auto pdf_ptr = std::make_shared<MicrofacetPDF>(mfDistribution, basis, wo_world);

    // 2) sample an incoming direction `wi`
    vec3 wi_world = pdf_ptr->get(uc, u).normalize();
    if (wi_world.xyz().near_zero()) // our PDF signals "no sample"
      return false;
    // 3) reject if it's below the geometric normal
    if (dot(rec.normal, wi_world) <= 0)
      return false;

    srec.sampled_ray = Ray(rec.position, wi_world);
    srec.sampled_type = BxDFFlags::GlossyReflection;
    srec.attenuation = f(wo_world, wi_world, rec, mode);
    srec.pdf_ptr = pdf_ptr;
    // 4) compute the PDF value
    srec.pdf_val = pdf_ptr->at(wi_world);
    return true;
  }

  float scatter_pdf(
      const ScatterRecord &srec, const Ray &wi_world,
      TransportMode mode = TransportMode::Radiance,
      BxDFReflTransFlags flags = BxDFReflTransFlags::All) const override
  {
    if (!(flags & BxDFReflTransFlags::Reflection))
      return 0.0f;

    if (mfDistribution.effectivelySmooth())
    {
      return 0.0f;
    }

    if (srec.pdf_ptr == nullptr)
      return 0.f;
    return srec.pdf_ptr->at(wi_world.getDirection().normalize());
  }

  float scatter_pdf(const gl::vec3 &wo_world, const gl::vec3 &wi_world,
                    const HitRecord &rec,
                    TransportMode mode = TransportMode::Radiance,
                    BxDFReflTransFlags flags = BxDFReflTransFlags::All) const override
  {
    using namespace gl;
    if (!(flags & BxDFReflTransFlags::Reflection))
      return 0.0f;

    if (mfDistribution.effectivelySmooth())
    {
      return 0.0f;
    }

    OrthoBasis basis(rec.normal);
    vec3 wo_l = basis.toLocal(wo_world.normalize());
    vec3 wi_l = basis.toLocal(wi_world.normalize());

    auto pdf_ptr = std::make_shared<MicrofacetPDF>(mfDistribution, basis, wo_world);

    if (dot(rec.normal, wi_world) <= 0)
      return 0.0f;

    return pdf_ptr->at(wi_world);
  }

  gl::vec3 f(const gl::vec3 &wo_world, const gl::vec3 &wi_world,
             const HitRecord &rec,
             TransportMode mode = TransportMode::Radiance) const override
  {

    using namespace gl;
    OrthoBasis basis(rec.normal);
    vec3 wo_l = basis.toLocal(wo_world.normalize());
    vec3 wi_l = basis.toLocal(wi_world.normalize());
    // 2) below‐surface => zero
    if (wi_l.z() <= 0 || wo_l.z() <= 0)
    {
      return vec3(0.f);
    }
    // 3) handle delta case
    if (mfDistribution.effectivelySmooth())
    {
      return vec3(0.f);
    }
    // 4) microfacet half-vector
    vec3 m_l = normalize(wo_l + wi_l);
    // 5) D, G, F
    float D = mfDistribution.D(m_l);
    float G = mfDistribution.G(wo_l, wi_l);
    vec3 F = fresnelComplex(fabs(dot(wo_l, m_l)), eta, k);
    // 6) f value
    vec3 f =
        D * F * G / (4.f * pbrt::absCosTheta(wo_l) * pbrt::absCosTheta(wi_l));

    return f;
  };

private:
  TrowbridgeReitzDistribution mfDistribution;
  // ior and absorption
  gl::vec3 eta, k;
};
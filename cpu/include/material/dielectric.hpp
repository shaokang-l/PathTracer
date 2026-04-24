#pragma once
#include "material/material.hpp"
#include "PDF/mfDielectricPDF.hpp"

class MFDielectric : public Material
{
private:
  float eta;
  TrowbridgeReitzDistribution mfDistrib;

public:
  MFDielectric(float eta, TrowbridgeReitzDistribution mfDistrib)
      : eta(eta), mfDistrib(mfDistrib) {};

  MFDielectric(float eta, float alpha_x, float alpha_y)
      : eta(eta), mfDistrib(alpha_x, alpha_y) {};

  bool effectivelySmooth() const { return mfDistrib.effectivelySmooth(); }
  bool
  scatter(const Ray &ray_in, HitRecord &rec, ScatterRecord &srec,
          float uc = gl::rand_num(), // coin-flip sample
          const gl::vec2 &u = {gl::rand_num(),
                               gl::rand_num()}, // 2D microfacet sample
          TransportMode mode = TransportMode::Radiance,
          BxDFReflTransFlags flags = BxDFReflTransFlags::All) const override
  {

    if (eta == 1 || mfDistrib.effectivelySmooth())
    {
      using namespace gl;

      vec3 wo_world = -ray_in.getDirection().normalize();
      float cosThetaO_signed = gl::dot(rec.normal, wo_world);
      float R = gl::fresnelDielectric(cosThetaO_signed, eta), T = 1 - R;
      float pr = R, pt = T;
      if (!(flags & BxDFReflTransFlags::Reflection))
        pr = 0;
      if (!(flags & BxDFReflTransFlags::Transmission))
        pt = 0;
      if (pr == 0 && pt == 0)
        return false;

      if (uc < pr / (pr + pt))
      {
        vec3 wi_world = pbrt::reflect(wo_world, rec.normal).normalize();
        srec.sampled_ray = Ray(rec.position, wi_world);
        srec.sampled_type = BxDFFlags::SpecularReflection;
        srec.pdf_ptr = nullptr;
        srec.attenuation = 1.0f;
        srec.pdf_val = R;
        return true;
      }
      else
      {
        vec3 wi_world;
        float etap;
        // TIR
        bool valid = pbrt::refract(wo_world, rec.normal, eta, etap, wi_world);
        if (!valid)
          return false;

        srec.sampled_ray = Ray(rec.position, wi_world);
        srec.sampled_type = BxDFFlags::SpecularTransmission;
        srec.pdf_ptr = nullptr;
        srec.pdf_val = T;
        if (mode == TransportMode::Radiance)
          srec.attenuation = 1.0f / square(etap);
        else
          srec.attenuation = 1.0f;
        return true;
      }
    }

    // create PDF object
    using namespace gl;
    vec3 wo_world = -ray_in.getDirection().normalize();
    OrthoBasis onb(rec.normal);
    auto pdf = std::make_shared<MFDielectricPDF>(mfDistrib, onb, wo_world, eta,
                                                 flags, mode);
    // sample a world-space direction
    vec3 wi_world = pdf->get(uc, u);
    if (wi_world.xyz().near_zero()) // our PDF signals "no sample"
      return false;

    // fill scatter record
    srec.sampled_ray = Ray(rec.position, wi_world);
    // determine specular/unfolded type from hemisphere test
    bool isRefl = dot(onb.toLocal(wi_world), onb.toLocal(wo_world)) > 0;
    srec.sampled_type =
        isRefl ? BxDFFlags::GlossyReflection : BxDFFlags::GlossyTransmission;

    // attenuation = the BRDF value
    srec.attenuation = f(wo_world, wi_world, rec, mode);
    // store PDF
    srec.pdf_ptr = pdf;
    srec.pdf_val = pdf->at(wi_world);
    return true;
  };

  float scatter_pdf(
      const gl::vec3 &wo_world, const gl::vec3 &wi_world,
      const HitRecord &rec,
      TransportMode mode = TransportMode::Radiance,
      BxDFReflTransFlags flags = BxDFReflTransFlags::All) const override
  {
    using namespace gl;
    if (eta == 1 || mfDistrib.effectivelySmooth())
      return 0.0f;

    OrthoBasis basis(rec.normal);
    vec3 wo_l = basis.toLocal(wo_world.normalize());
    vec3 wi_l = basis.toLocal(wi_world.normalize());

    auto pdf = std::make_shared<MFDielectricPDF>(mfDistrib, basis, wo_world, eta,
                                                 flags, mode);
    return pdf->at(wi_world);
  }

  // required for non-delta
  gl::vec3 f(const gl::vec3 &wo_world, const gl::vec3 &wi_world,
             const HitRecord &rec,
             TransportMode mode = TransportMode::Radiance) const override
  {
    using namespace gl;
    if (eta == 1 || mfDistrib.effectivelySmooth())
      return vec3(0.f);

    OrthoBasis basis(rec.normal);
    vec3 wo_l = basis.toLocal(wo_world.normalize());
    vec3 wi_l = basis.toLocal(wi_world.normalize());

    // compute half vector
    float cosTheta_o = pbrt::cosTheta(wo_l), cosTheta_i = pbrt::cosTheta(wi_l);
    bool reflect = cosTheta_i * cosTheta_o > 0;
    float etap = 1;
    if (!reflect)
      etap = cosTheta_o > 0 ? eta : (1 / eta);
    vec3 wm = wi_l * etap + wo_l;
    if (cosTheta_i == 0 || cosTheta_o == 0 || wm.length() == 0)
      return vec3(0.f);

    wm = pbrt::faceForward(normalize(wm), vec3(0, 0, 1));
    // discard backfacing microfacets
    if (dot(wm, wi_l) * cosTheta_i < 0 || dot(wm, wo_l) * cosTheta_o < 0)
      return {};

    float F = fresnelDielectric(dot(wo_l, wm), eta);
    if (reflect)
    {
      return vec3(mfDistrib.D(wm) * mfDistrib.G(wo_l, wi_l) * F /
                  std::abs(4 * cosTheta_i * cosTheta_o));
    }
    else
    {
      float denom = square(dot(wi_l, wm) + dot(wo_l, wm) / etap) * cosTheta_i *
                    cosTheta_o;

      float ft = mfDistrib.D(wm) * (1 - F) * mfDistrib.G(wo_l, wi_l) *
                 std::abs(dot(wi_l, wm) * dot(wo_l, wm) / denom);

      if (mode == TransportMode::Radiance)
        ft /= square(etap);

      return vec3(ft);
    }
  };
};
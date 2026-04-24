#pragma once
#include "material.hpp"
#include "utils/utility.hpp"
#include "utils/constants.hpp"
#include "texture/texture.hpp"
#include "PDF/clearcoatPDF.hpp"
class DisneyClearcoat : public Material
{
private:
    ClearcoatTRD distribution;

public:
    DisneyClearcoat(float clearcoat_gloss) : distribution(ClearcoatTRD::fromClearcoatGloss(clearcoat_gloss)) {};

    bool scatter(const Ray &ray_in, HitRecord &rec, ScatterRecord &srec,
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

        OrthoBasis basis(rec.normal);
        auto pdf_ptr = std::make_shared<ClearcoatPDF>(distribution, basis, wo_world);

        vec3 wi_world = pdf_ptr->get(uc, u).normalize();

        if (dot(rec.normal, wi_world) <= 0)
            return false;

        srec.sampled_ray = Ray(rec.position, wi_world.normalize());
        srec.sampled_type = BxDFFlags::GlossyReflection;
        srec.attenuation = f(wo_world, wi_world, rec, mode);
        srec.pdf_ptr = pdf_ptr;
        srec.pdf_val = pdf_ptr->at(wi_world);
        return true;
    }

    gl::vec3 f(const gl::vec3 &wo_world, const gl::vec3 &wi_world,
               const HitRecord &rec,
               TransportMode mode = TransportMode::Radiance) const override
    {
        using namespace gl;
        OrthoBasis onb(rec.normal);
        vec3 wo = onb.toLocal(wo_world);
        vec3 wi = onb.toLocal(wi_world);
        vec3 wh = (wo + wi).normalize();

        if (wi.z() <= 0 || wo.z() <= 0)
            return vec3(0.f);

        float cos_theta_wi = pbrt::cosTheta(wi);
        float cos_theta_wo = pbrt::cosTheta(wo);
        float abs_dot_wh_wi = fabs(dot(wh, wi));
        vec3 Fc = distribution.F(abs_dot_wh_wi);
        vec3 Dc = distribution.D(wh);
        vec3 Gc = distribution.G(wo, wi);
        vec3 f = Fc * Dc * Gc / fabs(4.f * cos_theta_wi * cos_theta_wo);
        return f;
    };

    float scatter_pdf(const gl::vec3 &wo_world, const gl::vec3 &wi_world, const HitRecord &rec,
                      TransportMode mode = TransportMode::Radiance,
                      BxDFReflTransFlags flags = BxDFReflTransFlags::All) const override
    {
        using namespace gl;
        if (!(flags & BxDFReflTransFlags::Reflection))
            return 0.f;

        OrthoBasis basis(rec.normal);
        auto pdf = std::make_shared<ClearcoatPDF>(distribution, basis, wo_world);
        return pdf->at(wi_world);
    };
};
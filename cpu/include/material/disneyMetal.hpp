#pragma once
#include "material.hpp"
#include "utils/utility.hpp"
#include "utils/constants.hpp"
#include "texture/texture.hpp"
#include "PDF/mfPDF.hpp"
#include <variant>

class DisneyMetal : public Material
{
private:
    std::shared_ptr<Texture2D> albedo;
    TrowbridgeReitzDistribution distribution;

public:
    using ColorVariant = std::variant<gl::vec3, std::shared_ptr<Texture2D>>;

    DisneyMetal(const ColorVariant &a, float roughness, float anisotropic)
        : distribution(TrowbridgeReitzDistribution::fromRoughnessAnisotropic(roughness, anisotropic))
    {
        albedo = gl::texture::to_texture2d(a);
    }

    bool effectivelySmooth() const { return distribution.effectivelySmooth(); }
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

        vec3 baseColor = albedo->getTexelColor(rec.texCoords);
        vec3 wo_world = -ray_in.getDirection().normalize();
        if (distribution.effectivelySmooth())
        {

            gl::vec3 wi_world = gl::pbrt::reflect(wo_world, rec.normal).normalize();
            if (dot(rec.normal, wi_world) <= 0)
                return false;

            srec.sampled_ray = Ray(rec.position, wi_world);
            float absCosThetaI = fabs(dot(rec.normal, wi_world));
            vec3 f = fresnelSchlick(absCosThetaI, baseColor);
            srec.attenuation = f;
            srec.sampled_type = BxDFFlags::SpecularReflection;
            srec.pdf_ptr = nullptr;
            srec.pdf_val = 0.0f;
            return true;
        }
        // --- rough (microfacet) case ---
        OrthoBasis basis(rec.normal);
        auto pdf_ptr = std::make_shared<MicrofacetPDF>(distribution, basis, wo_world);

        // 2) sample an incoming direction `wi`
        vec3 wi_world = pdf_ptr->get(uc, u).normalize();
        if (wi_world.xyz().near_zero()) // our PDF signals "no sample"
            return false;
        // 3) reject if it's below the geometric normal
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

        if (wi.z() <= 0 || wo.z() <= 0 || distribution.effectivelySmooth())
            return vec3(0.f);

        float cos_theta_wi = pbrt::cosTheta(wi);
        float cos_theta_wo = pbrt::cosTheta(wo);
        float dot_wh_wi = fabs(dot(wh, wi));
        vec3 baseColor = albedo->getTexelColor(rec.texCoords);
        vec3 Fm = fresnelSchlick(dot_wh_wi, baseColor);
        vec3 Dm = distribution.D(wh);
        vec3 Gm = distribution.G(wo, wi);
        vec3 f = Fm * Dm * Gm / fabs(4.f * cos_theta_wi * cos_theta_wo);
        return f;
    };

    gl::vec3 f_variant(const gl::vec3 &wo_world, const gl::vec3 &wi_world,
                       const HitRecord &rec,
                       TransportMode mode = TransportMode::Radiance, float specular_tint = 0.5f, float eta = 1.0f, float specular = 0.5f, float metallic = 0.5f) const
    {
        using namespace gl;
        OrthoBasis onb(rec.normal);
        vec3 wo = onb.toLocal(wo_world);
        vec3 wi = onb.toLocal(wi_world);
        vec3 wh = (wo + wi).normalize();

        if (wi.z() <= 0 || wo.z() <= 0 || distribution.effectivelySmooth())
            return vec3(0.f);

        float cos_theta_wi = pbrt::cosTheta(wi);
        float cos_theta_wo = pbrt::cosTheta(wo);
        float dot_wh_wi = fabs(dot(wh, wi));
        vec3 baseColor = albedo->getTexelColor(rec.texCoords);

        vec3 Ks = (1 - specular_tint) + specular_tint * baseColor;
        vec3 R0 = etaToR0(eta);
        vec3 C0 = specular * R0 * (1 - metallic) * Ks + metallic * baseColor;
        vec3 Fm = fresnelSchlick(dot_wh_wi, C0);
        vec3 Dm = distribution.D(wh);
        vec3 Gm = distribution.G(wo, wi);
        vec3 f = Fm * Dm * Gm / fabs(4.f * cos_theta_wi * cos_theta_wo);
        return f;
    };

    float scatter_pdf(const gl::vec3 &wo_world, const gl::vec3 &wi_world, const HitRecord &rec,
                      TransportMode mode = TransportMode::Radiance,
                      BxDFReflTransFlags flags = BxDFReflTransFlags::All) const override
    {
        using namespace gl;
        if (!(flags & BxDFReflTransFlags::Reflection))
            return 0.f;
        if (distribution.effectivelySmooth())
            return 0.f;

        OrthoBasis basis(rec.normal);
        auto pdf = std::make_shared<MicrofacetPDF>(distribution, basis, wo_world);
        return pdf->at(wi_world);
    };
};
#pragma once
#include "material.hpp"
#include "utils/utility.hpp"
#include "utils/constants.hpp"
#include "texture/texture.hpp"
#include <variant>

class DisneyDiffuse : public Material
{
private:
    std::shared_ptr<Texture2D> albedo;
    float roughness;
    float subsurface;

public:
    DisneyDiffuse(const ColorVariant &a, float r, float s)
        : roughness(r), subsurface(s)
    {
        albedo = gl::texture::to_texture2d(a);
    }

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
        std::shared_ptr<CosinePDF> pdf = std::make_shared<CosinePDF>(rec.normal);
        vec3 wi_world = pdf->get(uc, u);
        srec.sampled_ray = Ray(rec.position, wi_world.normalize());
        srec.pdf_ptr = pdf;
        srec.pdf_val = pdf->at(wi_world);
        srec.sampled_type = BxDFFlags::DiffuseReflection;
        srec.attenuation = f(wo_world, wi_world, rec, mode);
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
        float abs_cos_theta_wi = fabs(pbrt::cosTheta(wi));
        float abs_cos_theta_wo = fabs(pbrt::cosTheta(wo));
        vec3 baseColor = albedo->getTexelColor(rec.texCoords);
        // compute diffuse term
        float Fd90 = computeFd90(wo, wi, roughness);
        vec3 baseDiff = baseColor * (1.f / M_PI);
        baseDiff *= hackedSchlick(abs_cos_theta_wi, Fd90) * hackedSchlick(abs_cos_theta_wo, Fd90);

        // compute ss term
        float fss90 = computeFss90(wo, wi, roughness);
        vec3 baseSubsurface = 1.25 * baseColor * (1.f / M_PI);
        baseSubsurface *= (hackedSchlick(abs_cos_theta_wi, fss90) * hackedSchlick(abs_cos_theta_wo, fss90) * (1.f / (abs_cos_theta_wi + abs_cos_theta_wo) - 0.5f) + 0.5f);
        return (1.f - this->subsurface) * baseDiff + baseSubsurface * this->subsurface;
    };

    // required for non-delta materials
    float
    scatter_pdf(const ScatterRecord &srec, const Ray &wi_world,
                TransportMode mode = TransportMode::Radiance,
                BxDFReflTransFlags flags = BxDFReflTransFlags::All) const override
    {
        if (srec.pdf_ptr == nullptr)
            return 0.f;
        return srec.pdf_ptr->at(wi_world.getDirection().normalize());
    }

    float scatter_pdf(const gl::vec3 &wo_world, const gl::vec3 &wi_world, const HitRecord &rec,
                      TransportMode mode = TransportMode::Radiance,
                      BxDFReflTransFlags flags = BxDFReflTransFlags::All) const override
    {
        using namespace gl;
        if (!(flags & BxDFReflTransFlags::Reflection))
            return 0.f;

        std::shared_ptr<CosinePDF> pdf = std::make_shared<CosinePDF>(rec.normal);
        return pdf->at(wi_world);
    };
};
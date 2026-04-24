#pragma once
#include "material.hpp"
#include "utils/utility.hpp"
#include "utils/constants.hpp"
#include "texture/texture.hpp"
#include "base/colors.hpp"
class DisneySheen : public Material
{
private:
    std::shared_ptr<Texture2D> color;
    float sheen_tint;

public:
    DisneySheen(const ColorVariant &color_var, float sheen_tint = 0.0f)
        : sheen_tint(sheen_tint)
    {
        color = gl::texture::to_texture2d(color_var);
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
        vec3 wh = (wo + wi).normalize();
        float abs_cos_theta_wi = fabs(pbrt::cosTheta(wi));
        float abs_cos_theta_wo = fabs(pbrt::cosTheta(wo));
        float abs_dot_wh_wi = fabs(dot(wh, wi));
        vec3 baseColor = color->getTexelColor(rec.texCoords);
        float luminance = rgbToLuminance(baseColor);
        vec3 Ctint = luminance > 0.0f ? baseColor / luminance : vec3(1.0f);
        vec3 Csheen = gl::lerp(vec3(1.0f), Ctint, sheen_tint);
        vec3 f = Csheen * pow((1 - abs_dot_wh_wi), 5.f);
        return f;
    };

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
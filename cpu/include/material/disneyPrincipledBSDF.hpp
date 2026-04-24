#pragma once
#include "disneyGlass.hpp"
#include "disneyMetal.hpp"
#include "disneySheen.hpp"
#include "disneyClearcoat.hpp"
#include "disneyDiffuse.hpp"

class DisneyPrincipledBSDF : public Material
{
private:
    std::shared_ptr<DisneyGlass> glass_mat;
    std::shared_ptr<DisneyMetal> metal_mat;
    std::shared_ptr<DisneySheen> sheen_mat;
    std::shared_ptr<DisneyClearcoat> clearcoat_mat;
    std::shared_ptr<DisneyDiffuse> diffuse_mat;

    float specular_transmission;
    float metallic;
    float clearcoat;

    float diffuse_weight;
    float metal_weight;
    float glass_weight;
    float clearcoat_weight;
    // sheen weight doesn't participate in importance sampling
    float sheen_weight;

    // needed for metal lobe, since it's a variant of the metal lobe
    float specular_tint;
    float eta;
    float specular;

public:
    DisneyPrincipledBSDF(
        const gl::vec3 &baseColor,
        float specular_transmission,
        float metallic,
        float subsurface,
        float specular,
        float roughness,
        float specular_tint,
        float anisotropic,
        float sheen,
        float sheen_tint,
        float clearcoat,
        float clearcoat_gloss,
        float eta)
        : specular_transmission(specular_transmission), metallic(metallic),
          clearcoat(clearcoat), specular_tint(specular_tint), eta(eta), specular(specular)
    {
        glass_mat = std::make_shared<DisneyGlass>(baseColor, roughness, anisotropic, eta);
        metal_mat = std::make_shared<DisneyMetal>(baseColor, roughness, anisotropic);
        sheen_mat = std::make_shared<DisneySheen>(baseColor, sheen_tint);
        clearcoat_mat = std::make_shared<DisneyClearcoat>(clearcoat_gloss);
        diffuse_mat = std::make_shared<DisneyDiffuse>(baseColor, roughness, subsurface);

        // weights for the mixture
        diffuse_weight = (1 - metallic) * (1 - specular_transmission);
        metal_weight = (1 - specular_transmission * (1 - metallic));
        glass_weight = (1 - metallic) * specular_transmission;
        clearcoat_weight = 0.25 * clearcoat;
        sheen_weight = (1 - metallic) * sheen;
    }

    bool scatter(const Ray &ray_in, HitRecord &rec, ScatterRecord &srec,
                 float uc = gl::rand_num(), // coin-flip sample
                 const gl::vec2 &u = {gl::rand_num(),
                                      gl::rand_num()}, // 2D microfacet sample
                 TransportMode mode = TransportMode::Radiance,
                 BxDFReflTransFlags flags = BxDFReflTransFlags::All) const override;

    gl::vec3 f(const gl::vec3 &wo_world, const gl::vec3 &wi_world,
               const HitRecord &rec,
               TransportMode mode = TransportMode::Radiance) const override
    {
        using namespace gl;

        if (rec.is_inside)
            return glass_mat ? glass_mat->f(wo_world, wi_world, rec, mode) : gl::vec3(0.f);

        vec3 result(0.f);
        if (diffuse_mat)
            result += diffuse_mat->f(wo_world, wi_world, rec, mode) * diffuse_weight;
        if (metal_mat)
            result += metal_mat->f_variant(wo_world, wi_world, rec, mode) * metal_weight;
        if (glass_mat)
            result += glass_mat->f(wo_world, wi_world, rec, mode) * glass_weight;
        if (clearcoat_mat)
            result += clearcoat_mat->f(wo_world, wi_world, rec, mode) * clearcoat_weight;
        if (sheen_mat)
            result += sheen_mat->f(wo_world, wi_world, rec, mode) * sheen_weight;
        return result;
    }

    float scatter_pdf(const gl::vec3 &wo_world, const gl::vec3 &wi_world,
                      const HitRecord &rec,
                      TransportMode mode = TransportMode::Radiance,
                      BxDFReflTransFlags flags = BxDFReflTransFlags::All) const override;
};

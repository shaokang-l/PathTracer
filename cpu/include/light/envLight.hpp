#pragma once
#include "light.hpp"

class EnvironmentLight : public Light
{
public:
    EnvironmentLight(const std::string &hdri_filepath, float intensity_scale = 1.0f)
        // Call base Light constructor. Position is irrelevant for env light.
        // Base color_texture can be a dummy white, as HDRITexture provides color.
        : Light(gl::vec3(0.f), std::make_shared<HDRITexture>(hdri_filepath, false /*flip_y typically false for env maps*/), intensity_scale)
    {
        this->type = LightType::ENVIRONMENT_LIGHT; // Make sure this type is in your enum
    }

    // --- Implement/Override Light virtual methods ---

    // Samples a direction FROM the environment map.
    // For consistency with Light::get_sample returning a point, we return a point
    // "at infinity" along the sampled direction. The normal is -direction.
    gl::vec3 get_sample(float u1, float u2) const override
    {
        gl::vec3 sampled_direction;
        float pdf_solid_angle_unused; // This specific signature doesn't return PDF directly
        if (this->texture)
        {

            // cast to HDRITexture
            auto hdr_texture = std::dynamic_pointer_cast<HDRITexture>(this->texture);
            // This method in HDRITexture should be const
            hdr_texture->sample_direction(u1, u2, sampled_direction, pdf_solid_angle_unused);
            return sampled_direction * 1e5f; // Point "at infinity"
        }
        else
        {
            throw std::runtime_error("Environment light texture is not an HDRITexture");
        }
    }

    // This method is less intuitive for env lights if it's meant to return a point.
    // Let's assume uniform_sample also returns a point at infinity.
    gl::vec3 uniform_sample() const override
    {
        gl::vec3 normal_at_sample_unused; // Not really a normal for a point sample
        float pdf_area_unused;
        return get_sample(gl::rand_num(), gl::rand_num()); // Reuses get_sample logic
    }

    // Normal at a point on the infinitely distant sphere representing the environment.
    // If p_on_light is a point at infinity (e.g., direction * large_dist),
    // the normal points inwards, so -direction.
    gl::vec3 get_normal_at(const gl::vec3 &p_on_light_at_infinity) const override
    {
        return -p_on_light_at_infinity.normalize();
    }

    float get_area() const override
    {
        return INFINITY; // Or a very large number if INFINITY isn't defined/usable
    }

    // PDF of sampling the direction 'dir_to_env' from 'origin' *towards* this environment light.
    // This is P(ω) for sampling the environment light.
    float pdf_value(const gl::vec3 &origin_shading_point, const gl::vec3 &dir_to_env) const override
    {
        if (this->texture)
        {
            // cast to HDRITexture
            auto hdr_texture = std::dynamic_pointer_cast<HDRITexture>(this->texture);
            // This should be the PDF of the sampling strategy used in HDRITexture::sample_direction
            return hdr_texture->pdf_direction(dir_to_env.normalize());
        }
        else
        {
            throw std::runtime_error("Environment light texture is not an HDRITexture");
        }
    }

    // --- Method to get radiance from a specific direction ---
    gl::vec3 L_emit(const HitRecord &hit_record, const gl::vec3 &direction_from_env) const
    {
        if (this->texture)
        {
            // cast to HDRITexture
            auto hdr_texture = std::dynamic_pointer_cast<HDRITexture>(this->texture);
            return hdr_texture->getTexelColor(direction_from_env) * this->intensity;
        }
        else
        {
            throw std::runtime_error("Environment light texture is not an HDRITexture");
        }
    }
};

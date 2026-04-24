#pragma once
#include "material/material.hpp"
class MaterialTemplate : public Material
{
public:
    bool scatter(const Ray &ray_in, HitRecord &rec, ScatterRecord &srec,
                 float uc = gl::rand_num(), // coin-flip sample
                 const gl::vec2 &u = {gl::rand_num(),
                                      gl::rand_num()}, // 2D microfacet sample
                 TransportMode mode = TransportMode::Radiance,
                 BxDFReflTransFlags flags = BxDFReflTransFlags::All) const override
    {
        return false;
    }

    gl::vec3 f(const gl::vec3 &wo_world, const gl::vec3 &wi_world,
               const HitRecord &rec,
               TransportMode mode = TransportMode::Radiance) const override
    {
        return gl::vec3(0.0f);
    };

    float scatter_pdf(const gl::vec3 &wo_world, const gl::vec3 &wi_world,
                      const HitRecord &rec,
                      TransportMode mode = TransportMode::Radiance,
                      BxDFReflTransFlags flags = BxDFReflTransFlags::All) const override
    {
        return 0.f;
    }

    //===============================================================
    // Below are usually not needed to be overridden
    //===============================================================
    float scatter_pdf(const ScatterRecord &srec, const Ray &wi_world,
                      TransportMode mode = TransportMode::Radiance,
                      BxDFReflTransFlags flags = BxDFReflTransFlags::All) const override
    {
        if (srec.pdf_ptr == nullptr)
            return 0.f;
        return srec.pdf_ptr->at(wi_world.getDirection().normalize());
    }

    gl::vec3 emit(const Ray &ray_in, HitRecord &rec) const
    {
        return gl::vec3(0.0f);
    }

    bool is_emitter() const { return false; }
};
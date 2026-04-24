#pragma once
#include "medium/medium.hpp"

class HomogeneousMajorantIterator : public RayMajorantIterator
{

private:
    RayMajorantSegment segment;
    bool called;

public:
    HomogeneousMajorantIterator(float t_min, float t_max, gl::vec3 sigma_maj)
    {
        segment.t_min = t_min;
        segment.t_max = t_max;
        segment.sigma_maj = sigma_maj;
        called = false;
    }

    HomogeneousMajorantIterator() : called(true) {};

    std::optional<RayMajorantSegment> next() override
    {
        if (called)
            return std::nullopt;
        called = true;
        return segment;
    }
};

class HomogeneousMedium : public Medium
{
private:
    gl::vec3 sigma_a;
    gl::vec3 sigma_s;
    gl::vec3 Le;
    std::shared_ptr<PhaseFunction> phase_function;

public:
    HomogeneousMedium(gl::vec3 sigma_a, gl::vec3 sigma_s, float sigma_scale, gl::vec3 Le, float Le_scale, float g)
    {
        this->sigma_a = sigma_a * sigma_scale;
        this->sigma_s = sigma_s * sigma_scale;
        this->Le = Le * Le_scale;
        phase_function = std::make_shared<HGPhaseFunction>(g);
    }

    std::shared_ptr<RayMajorantIterator> get_ray_majorant_iterator(const Ray &ray, float tMax) override
    {
        gl::vec3 sigma_maj = sigma_a + sigma_s;
        return std::make_shared<HomogeneousMajorantIterator>(0, tMax, sigma_maj);
    }

    bool is_emitter() const override
    {
        return gl::maxComponent(Le) > 0.f;
    }

    MediumProperties sample_properties_at(const gl::vec3 &point) const override
    {
        MediumProperties properties;
        properties.sigma_a = sigma_a;
        properties.sigma_s = sigma_s;
        properties.phase_function = phase_function;
        properties.Le = Le;
        return properties;
    }
};

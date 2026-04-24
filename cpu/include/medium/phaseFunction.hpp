#pragma once
#include "base/ray.hpp"
#include "utils/matrix.hpp"
#include "medium/phaseMath.hpp"

struct HitRecord;
struct PhaseRecord
{
    float p;
    gl::vec3 wi;
    float pdf;
};

class PhaseFunction
{
public:
    // think of it as f()
    virtual float p(const gl::vec3 &wo_world, const gl::vec3 &wi_world) const = 0;
    // think of it as scatter
    virtual bool sample_p(const gl::vec3 &wo_world, PhaseRecord &prec, const gl::vec2 &u) const = 0;
    virtual float pdf(const gl::vec3 &wo_world, const gl::vec3 &wi_world) const = 0;
};

class HGPhaseFunction : public PhaseFunction
{
private:
    float g;

public:
    HGPhaseFunction(float g) : g(g) {}

    float p(const gl::vec3 &wo_world, const gl::vec3 &wi_world) const override
    {
        return gl::HenyeyGreenstein(gl::dot(wo_world.normalize(), wi_world.normalize()), g);
    };

    bool sample_p(const gl::vec3 &wo_world, PhaseRecord &prec, const gl::vec2 &u) const override
    {
        prec.wi = gl::sampleHenyeyGreenstein(wo_world, g, u, &prec.pdf);
        prec.p = prec.pdf;
        return true;
    }

    float pdf(const gl::vec3 &wo_world, const gl::vec3 &wi_world) const override
    {
        return p(wo_world, wi_world);
    }
};

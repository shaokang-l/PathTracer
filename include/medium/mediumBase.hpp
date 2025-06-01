#pragma once
#include "utils/utility.hpp"
#include "phaseFunction.hpp"
// MediumProperties is a simple structure that
// wraps up the values that describe scattering and emission
// at a point inside a medium.
class Medium;

// medium interaction record
struct MediumRecord
{
    gl::vec3 p;
    gl::vec3 wo;
    std::shared_ptr<Medium> medium;
    std::shared_ptr<PhaseFunction> phase_function;
    MediumRecord(gl::vec3 p, gl::vec3 wo, std::shared_ptr<Medium> medium, std::shared_ptr<PhaseFunction> phase_function)
        : p(p), wo(wo), medium(medium), phase_function(phase_function) {}
};

struct MediumProperties
{
    gl::vec3 sigma_a;
    gl::vec3 sigma_s;
    std::shared_ptr<PhaseFunction> phase_function;
    gl::vec3 Le;
    gl::vec3 sigma_t() const { return sigma_a + sigma_s; }
};

// RayMajorantSegment is a structure that describes a
// segment of a ray that is inside a medium.
struct RayMajorantSegment
{
    float t_min;
    float t_max;
    gl::vec3 sigma_maj;
};

// Implementations of it should return majorant
// segments from the front to the back of the ray with no overlap in
// between segments, though it may skip over ranges of
// corresponding to regions of space where there is no scattering.
// After it has returned all segments along the ray,
// an unset optional value should be returned.
class RayMajorantIterator
{
public:
    virtual std::optional<RayMajorantSegment> next() = 0;
};

struct MediumInterface
{
    MediumInterface(std::shared_ptr<Medium> inside, std::shared_ptr<Medium> outside)
        : inside(inside), outside(outside) {};
    MediumInterface(std::shared_ptr<Medium> medium)
        : inside(medium), outside(medium) {};
    std::shared_ptr<Medium> inside;
    std::shared_ptr<Medium> outside;
    bool is_transition()
    {
        return inside != outside;
    }
};

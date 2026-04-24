#pragma once
#include "material/materialMath.hpp"
#include "PDF/pdf.hpp"
#include "utils/orthoBasis.hpp"
extern int rejects;
class MicrofacetPDF : public PDF
{
private:
  const TrowbridgeReitzDistribution &distrib;
  OrthoBasis onb;
  gl::vec3 wo_local;

public:
  MicrofacetPDF(const TrowbridgeReitzDistribution &d, const OrthoBasis &basis,
                const gl::vec3 &wo_world)
      : distrib(d), onb(basis), wo_local(onb.toLocal(wo_world.normalize())) {}

  // sample an outgoing (*wi*) direction in world space
  gl::vec3 get(float uc = gl::rand_num(),
               gl::vec2 u = gl::vec2(gl::rand_num(),
                                     gl::rand_num())) const override
  {
    // sample half‐vector in local
    gl::vec3 m_local = distrib.sample_wm(wo_local, u);
    gl::vec3 wi_local = gl::pbrt::reflect(wo_local, m_local);
    // if it went below the surface, just flip

    if (!gl::pbrt::sameHemisphere(wo_local, wi_local))
#ifdef BIASED_SAMPLING
      wi_local.z() = -wo_local.z();
#elif defined DISCARD_SAMPLING
      return gl::vec3(0.f);
#else
      do
      {
        rejects++;
        using namespace gl;
        vec2 u2 = vec2(rand_num(), rand_num());
        vec3 m = distrib.sample_wm(wo_local, u2);
        wi_local = gl::pbrt::reflect(wo_local, m);
      } while (wi_local.z() <= 0);
#endif
    // back to world
    return onb.toWorld(wi_local);
  }

  // PDF for a given outgoing direction
  float at(const gl::vec3 &wi_world) const override
  {
    gl::vec3 wi = onb.toLocal(wi_world.normalize());
    if (wi.z() <= 0)
      return 0.f;
    // half‐vector
    gl::vec3 m = (wo_local + wi).normalize();
    auto denominator = 4 * std::abs(dot(wo_local, m));
    if (denominator < 1e-6)
      return 0.f;
    // m = gl::pbrt::faceForward(normalize(m), gl::vec3(0.f, 0.f, 1.f));
    // microfacet PDF = D(m) / (4 * |dot(wi,m)|)
    return distrib.PDF(wo_local, m) / denominator;
  }
};

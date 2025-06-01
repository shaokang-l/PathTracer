#pragma once
#include "base/primitive.hpp"
#include "bezier.hpp"
#include "utils/utility.hpp"

enum class CurveType
{
  Flat,
  Cylinder,
  Ribbon
};
// note that only Cyliner is implemented for now
class Curve : public Hittable
{
public:
  std::array<gl::vec3, 4> cps;
  float width0, width1;
  CurveType type;
  std::shared_ptr<Material> material;
  std::shared_ptr<MediumInterface> medium_interface;

  Curve(const std::array<gl::vec3, 4> &cps, float width0, float width1,
        CurveType type = CurveType::Flat,
        std::shared_ptr<Material> material = nullptr,
        std::shared_ptr<MediumInterface> medium_interface = nullptr)
      : cps(cps), width0(width0), width1(width1), type(type), material(material), medium_interface(medium_interface)
  {
    this->objtype = ObjType::CURVE_OBJ;
  };

  // ignore t0 t1 for now, interface for motion blur
  AABB getAABB(float t0, float t1) override
  {
    gl::vec3 pmin = cps[0], pmax = cps[0];
    for (int i = 1; i < 4; ++i)
    {
      pmin = gl::min(pmin, cps[i]);
      pmax = gl::max(pmax, cps[i]);
    }
    // 2) Inflate by the maximum radius
    float r = std::max(width0, width1);
    pmin -= gl::vec3(r);
    pmax += gl::vec3(r);
    return AABB(pmin, pmax);
  };

  bool intersect(const Ray &ray, HitRecord &hit_record, float tmin = 0.0,
                 float tmax = 10000.f) const override
  {
    gl::hit_count++;
    if (this->intersection_mode != IntersectionMode::DEFAULT)
      throw std::runtime_error("CUSTOM intersection not supported for Curve");

    // --- subdivide into N linear segments and test each as a cylinder ---
    const int N = 8;
    float bestT = tmax;
    bool found = false;
    gl::vec3 bestP, bestN, bestTangent;

    // Precompute points and tangents on the Bézier
    std::vector<gl::vec3> P(N + 1), dP(N + 1);
    for (int i = 0; i <= N; ++i)
    {
      float u = float(i) / N;
      P[i] = gl::evalBezier(cps, u);
      dP[i] = gl::evalBezierDerivative(cps, u);
    }

    // For each small segment [P[i], P[i+1]] treat as cylinder
    for (int i = 0; i < N; ++i)
    {
      const gl::vec3 &pa = P[i];
      const gl::vec3 &pb = P[i + 1];
      float u0 = float(i) / N;
      float u1 = float(i + 1) / N;
      float r0 = width0 + (width1 - width0) * u0;
      float r1 = width0 + (width1 - width0) * u1;
      float radius = 0.5f * (r0 + r1);

      // cylinder axis and length
      gl::vec3 axis = gl::normalize(pb - pa);
      float segLen = (pb - pa).length();

      // project ray direction and origin onto plane ⟂ axis
      gl::vec3 d_perp =
          ray.getDirection() - gl::dot(ray.getDirection(), axis) * axis;
      gl::vec3 delta = ray.getOrigin() - pa;
      gl::vec3 o_perp = delta - gl::dot(delta, axis) * axis;

      // solve A t^2 + B t + C = 0 for intersection with infinite cylinder
      float A = gl::dot(d_perp, d_perp);
      float B = 2.0f * gl::dot(d_perp, o_perp);
      float C = gl::dot(o_perp, o_perp) - radius * radius;

      float roots[2];
      int nRoots = gl::solveQuadratic(A, B, C, roots);
      for (int r = 0; r < nRoots; ++r)
      {
        float t = roots[r];
        if (t < tmin || t > bestT)
          continue;
        // compute the hit point
        gl::vec3 hp = ray.at(t);
        // check within the caps of this segment
        float proj = gl::dot(hp - pa, axis);
        if (proj < 0.0f || proj > segLen)
          continue;

        // record a better hit
        bestT = t;
        bestP = hp;
        // interpolate tangent from the two Bézier-derivative samples
        float uu = proj / segLen;
        bestTangent = gl::normalize(dP[i] + (dP[i + 1] - dP[i]) * uu);
        // normal is vector from axis to hp, perpendicular to tangent
        gl::vec3 v = hp - (pa + axis * proj);
        bestN = gl::normalize(v - gl::dot(v, bestTangent) * bestTangent);
        found = true;
      }
    }

    if (!found)
      return false;

    // fill the hit record
    hit_record.t = bestT;
    hit_record.position = bestP;
    hit_record.normal = bestN;
    hit_record.hair_tangent = bestTangent;
    hit_record.material = this->material;
    hit_record.medium_interface = this->medium_interface;
    hit_record.set_normal(ray, bestN);
    hit_record.surface_area = 2 * M_PI * (width0 + width1);
    return true;
  }

  // for now, we don't treat them as emitters
  float pdf_value(const gl::vec3 &origin, const gl::vec3 &dir) const override
  {
    throw std::runtime_error(
        "pdf_value not implemented for Curve, use pdf_value for cylinder");
    return 0.0f;
  };

  // for now, we don't treat them as emitters
  gl::vec3 get_sample(const gl::vec3 &origin) const override
  {
    throw std::runtime_error(
        "get_sample not implemented for Curve, use get_sample for cylinder");
    return gl::vec3(1.f, 0.f, 0.f);
  };

  std::shared_ptr<MediumInterface> get_medium_interface() const override
  {
    return this->medium_interface;
  }
};
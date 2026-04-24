#pragma once
#include "base/lightList.hpp"
#include "base/objectList.hpp"
#include "config.hpp"
#include "primitives/bvh.hpp"
#include "PDF/hittablePDF.hpp"
#include "PDF/mixedPDF.hpp"
#include "sampler/sampler.hpp"
// Power heuristic for MIS
inline float powerHeuristic(float pdfA, float pdfB)
{
  float a2 = pdfA * pdfA;
  float b2 = pdfB * pdfB;
  return a2 / (a2 + b2);
}

inline gl::vec3 getRayColor(const Ray &ray, const ObjectList &prims,
                            gl::vec3 bg_color, const LightList &lights,
                            uint max_depth = 40,
                            std::shared_ptr<BVHNode> bvh = nullptr)
{
  using namespace gl;
  return gl::vec3(0.f);
}

#pragma once
#include "material/materialMath.hpp"
#include "PDF/pdf.hpp"
#include "utils/orthoBasis.hpp"

class HairMarschner;

class HairPDF : public PDF
{
public:
  HairPDF(const HairMarschner &hair_mat, const OrthoBasis &basis,
          const gl::vec3 &wo_world);
  ~HairPDF() = default;

  gl::vec3 get(float uc = gl::rand_num(),
               gl::vec2 u = gl::vec2(gl::rand_num(),
                                     gl::rand_num())) const override;

  float at(const gl::vec3 &wi_world) const override;

private:
  const HairMarschner &hair_mat;
  const OrthoBasis basis;
  const gl::vec3 wo_local;
};
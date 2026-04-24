#pragma once
#include "./pdf.hpp"

class MixedPDF : public PDF {
public:
  std::vector<std::shared_ptr<PDF>> pdfs;
  std::vector<float> weights;

  MixedPDF(std::vector<std::shared_ptr<PDF>> pdfs) : pdfs(pdfs) {
    weights.resize(pdfs.size());
    // uniform weight
    for (auto i = 0; i < pdfs.size(); i++)
      weights[i] = 1.f / pdfs.size();
  }

  MixedPDF(std::shared_ptr<PDF> p1, std::shared_ptr<PDF> p2) {
    pdfs.push_back(p1);
    pdfs.push_back(p2);
    weights.push_back(0.5f);
    weights.push_back(0.5f);
  }

  MixedPDF(std::vector<std::shared_ptr<PDF>> pdfs, std::vector<float> weights) {
    this->pdfs = pdfs;
    this->weights = weights;
  }

  float at(const gl::vec3 &direction) const override {
    float sum = 0.f;
    for (auto i = 0; i < pdfs.size(); i++)
      sum += weights[i] * pdfs[i]->at(direction);
    return sum;
  }

  gl::vec3 get(float uc = gl::rand_num(),
               gl::vec2 u = gl::vec2(gl::rand_num(),
                                     gl::rand_num())) const override {
    float r = gl::rand_num();
    int index = (int)(std::floor((r * pdfs.size()))) % pdfs.size();
    return pdfs[index]->get(uc, u);
  }
};
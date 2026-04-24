#pragma once
#include "utils/matrix.hpp"

class Medium;

class Ray
{
public:
  Ray(const gl::vec3 &origin, const gl::vec3 &direction, float intensity = 1.0f, const std::shared_ptr<Medium> medium = nullptr)
      : origin(origin), direction(direction), intensity(intensity), current_medium(medium) {};
  ~Ray() = default;
  Ray() = default;

  void setOrigin(const gl::vec3 &origin) { this->origin = origin; };

  void setDirection(const gl::vec3 &direction) { this->direction = direction; };

  gl::vec3 getOrigin() const { return this->origin; };

  gl::vec3 getDirection() const { return this->direction; };

  gl::vec3 at(float t) const { return this->origin + t * this->direction; };

  float intensity = 1.0f;

  std::shared_ptr<Medium> current_medium;
  gl::vec3 origin;
  gl::vec3 direction;
};

#pragma once
#include "./matrix.hpp"
class Plane {
public:
  Plane(gl::vec3 p1, gl::vec3 p2, gl::vec3 p3) {
    gl::vec3 v1 = p2 - p1;
    gl::vec3 v2 = p3 - p1;
    normal = cross(v1, v2);
    normal.normalized();
    d = dot(-normal, p1);
  }

  Plane(gl::vec3 normal, gl::vec3 p) {
    this->normal = normal;
    this->normal.normalize();
    d = dot(-normal, p);
  }

  Plane() {
    normal = gl::vec3(0, 0, 1);
    d = 0;
  }

  float get_x(float y, float z) {
    return -(normal.y() * y + normal.z() * z + d) / normal.x();
  }

  float get_y(float x, float z) {
    return -(normal.x() * x + normal.z() * z + d) / normal.y();
  }

  float get_z(float x, float y) {
    return -(normal.x() * x + normal.y() * y + d) / normal.z();
  }

  gl::vec3 get_normal() { return normal; }

  float get_d() { return d; }

private:
  gl::vec3 normal;
  float d;
};

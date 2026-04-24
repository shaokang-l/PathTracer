#pragma once
#include "base/object3D.hpp"
#include "base/ray.hpp"
#include "external/scene_io.hpp"
#include "utils/matrix.hpp"
#include "utils/quat.hpp"
#include "utils/transformations.hpp"
#include "utils/utility.hpp"

// abstract camera class
class Camera : public Object3D {
public:
  gl::mat4 getViewMat() const;
};

class PerspectiveCamera : public Camera {
public:
  float fov;
  float aspect;
  float focalDist;
  float aperture;
  PerspectiveCamera(float fov, float aspect, float focalDist, float aperture,
                    gl::vec3 up, gl::vec3 front, gl::vec3 position);
  PerspectiveCamera(const CameraIO *cameraIO, float aspect = 1.0f,
                    float aperture = 2.0f);
  ~PerspectiveCamera() = default;
  Ray generateRay(float x, float y) const;
  Ray generateRay(gl::vec2) const;

private:
  gl::vec3 bottom_left_pos;
  gl::vec3 plane_horizontal;
  gl::vec3 plane_vertical;
  gl::vec3 plane_h_vec;
  gl::vec3 plane_v_vec;
};

#include "base/camera.hpp"

gl::mat4 Camera::getViewMat() const {
  return gl::getViewMat(position, position + getFront(), getUp());
}

PerspectiveCamera::PerspectiveCamera(float fov, float aspect, float focalDist,
                                     float aperture, gl::vec3 up,
                                     gl::vec3 front, gl::vec3 position)
    : fov(fov), aspect(aspect), focalDist(focalDist), aperture(aperture) {
  using namespace gl;
  // assume fov is given in radian
  // here assume m equals 1
  const float m = 1.0f;
  auto theta = fov / 2.f;
  auto h = tan(theta);
  auto plane_height = m * h * 2.0f;
  auto plane_width = plane_height * aspect;

  this->defaultUp = up;
  this->defaultFront = front;
  this->position = position;

  auto lookat = -front.normalize();
  this->plane_h_vec = cross(up, lookat).normalize();
  // std::cout<<plane_h_vec<<std::endl;
  this->plane_v_vec = cross(lookat, plane_h_vec).normalize();
  // std::cout<<plane_v_vec<<std::endl;

  this->plane_horizontal = plane_h_vec * plane_width * focalDist;
  this->plane_vertical = plane_v_vec * plane_height * focalDist;
  bottom_left_pos =
      position - this->plane_horizontal / 2.0f - this->plane_vertical / 2.0f;
  bottom_left_pos -= lookat * focalDist;
}

PerspectiveCamera::PerspectiveCamera(const CameraIO *_io, float aspect,
                                     float aperture)
    : PerspectiveCamera(_io->verticalFOV, aspect, _io->focalDistance, aperture,
                        _io->orthoUp, _io->viewDirection, _io->position) {}

Ray PerspectiveCamera::generateRay(float u, float v) const {
  using namespace gl;
  float radius = focalDist / aperture * 0.5f;

  auto random_vec_on_lens =
      vec2(rand_num(-radius, radius), rand_num(-radius, radius));
  auto random_vec_on_plane = plane_h_vec * random_vec_on_lens.u() +
                             plane_v_vec * random_vec_on_lens.v();
  vec3 point_s = this->position + random_vec_on_plane;

  auto dir =
      bottom_left_pos + u * plane_horizontal + v * plane_vertical - point_s;

  return Ray(point_s, dir);
}

Ray PerspectiveCamera::generateRay(gl::vec2 uv) const {
  using namespace gl;
  auto u = uv.u();
  auto v = uv.v();
  return generateRay(u, v);
}

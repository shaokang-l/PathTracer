#pragma once

#include "scalar.h"

namespace pt {

  struct Vec2f {
    float x;
    float y;

    __both__ Vec2f() : x(0.f), y(0.f) {}
    __both__ explicit Vec2f(float v) : x(v), y(v) {}
    __both__ Vec2f(float x, float y) : x(x), y(y) {}
  };

  struct Vec3f {
    float x;
    float y;
    float z;

    __both__ Vec3f() : x(0.f), y(0.f), z(0.f) {}
    __both__ explicit Vec3f(float v) : x(v), y(v), z(v) {}
    __both__ Vec3f(float x, float y, float z) : x(x), y(y), z(z) {}

    __both__ float &operator[](int i) { return (&x)[i]; }
    __both__ const float &operator[](int i) const { return (&x)[i]; }
  };

  __both__ inline Vec2f operator+(Vec2f a, Vec2f b)
  {
    return Vec2f(a.x + b.x, a.y + b.y);
  }

  __both__ inline Vec2f operator-(Vec2f a, Vec2f b)
  {
    return Vec2f(a.x - b.x, a.y - b.y);
  }

  __both__ inline Vec2f operator*(Vec2f a, float s)
  {
    return Vec2f(a.x * s, a.y * s);
  }

  __both__ inline Vec2f operator*(float s, Vec2f a)
  {
    return a * s;
  }

  __both__ inline Vec3f operator+(Vec3f a, Vec3f b)
  {
    return Vec3f(a.x + b.x, a.y + b.y, a.z + b.z);
  }

  __both__ inline Vec3f operator-(Vec3f a, Vec3f b)
  {
    return Vec3f(a.x - b.x, a.y - b.y, a.z - b.z);
  }

  __both__ inline Vec3f operator-(Vec3f a)
  {
    return Vec3f(-a.x, -a.y, -a.z);
  }

  __both__ inline Vec3f operator*(Vec3f a, Vec3f b)
  {
    return Vec3f(a.x * b.x, a.y * b.y, a.z * b.z);
  }

  __both__ inline Vec3f operator*(Vec3f a, float s)
  {
    return Vec3f(a.x * s, a.y * s, a.z * s);
  }

  __both__ inline Vec3f operator*(float s, Vec3f a)
  {
    return a * s;
  }

  __both__ inline Vec3f operator/(Vec3f a, Vec3f b)
  {
    return Vec3f(a.x / b.x, a.y / b.y, a.z / b.z);
  }

  __both__ inline Vec3f operator/(Vec3f a, float s)
  {
    return Vec3f(a.x / s, a.y / s, a.z / s);
  }

  __both__ inline Vec3f operator/(float s, Vec3f a)
  {
    return Vec3f(s / a.x, s / a.y, s / a.z);
  }

  __both__ inline float dot(Vec3f a, Vec3f b)
  {
    return a.x * b.x + a.y * b.y + a.z * b.z;
  }

  __both__ inline Vec3f cross(Vec3f a, Vec3f b)
  {
    return Vec3f(a.y * b.z - a.z * b.y,
                 a.z * b.x - a.x * b.z,
                 a.x * b.y - a.y * b.x);
  }

  __both__ inline float length(Vec2f v)
  {
    return sqrtf(v.x * v.x + v.y * v.y);
  }

  __both__ inline float length(Vec3f v)
  {
    return sqrtf(dot(v, v));
  }

  __both__ inline Vec3f normalize(Vec3f v)
  {
    const float len = length(v);
    return len > 0.f ? v / len : Vec3f(0.f);
  }

  __both__ inline Vec3f faceForward(Vec3f v, Vec3f n)
  {
    return dot(v, n) < 0.f ? -v : v;
  }

  __both__ inline Vec2f sampleUniformDiskPolar(Vec2f u)
  {
    const float r = sqrtf(u.x);
    const float theta = 2.f * Pi * u.y;
    return Vec2f(r * cosf(theta), r * sinf(theta));
  }

} // namespace pt


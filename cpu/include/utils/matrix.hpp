#pragma once
#include <array>
#include <cassert>
#include <cmath>
#include <initializer_list>
#include <iostream>
#include <type_traits>
#include <vector>

#ifdef _WIN32
using uint = unsigned int;
#include <immintrin.h>
#else
#include "external/sse2neon.h"
#endif

// @description: an implmentation of vec and mat class
// Conform to the style of glm
// Use #define GL_SIMD to use SIMD version
// @author: Shaokang
namespace gl {
template <int N, class T> class vec {
  static_assert(N >= 1 && N <= 4, "N must be between 1 and 4");

public:
  union {
    __m128 vector;
    std::array<T, N> data;
  };

  T &r() { return data[0]; }

  T &g() {
    assert(data.size() >= 2);
    return data[1];
  }

  T &b() {
    assert(data.size() >= 3);
    return data[2];
  }

  T &a() {
    assert(data.size() == 4);
    return data[3];
  }

  T &x() { return data[0]; }

  T &y() {
    assert(data.size() >= 2);
    return data[1];
  }

  T &z() {
    assert(data.size() >= 3);
    return data[2];
  }

  T &w() {
    assert(data.size() == 4);
    return data[3];
  }

  T &u() { return data[0]; }

  T &v() {
    assert(data.size() >= 2);
    return data[1];
  }

  const T &x() const { return data[0]; }

  const T &y() const {
    assert(data.size() >= 2);
    return data[1];
  }

  const T &z() const {
    assert(data.size() >= 3);
    return data[2];
  }

  const T &w() const {
    assert(data.size() == 4);
    return data[3];
  }

  const T &r() const { return data[0]; }
  const T &g() const {
    assert(data.size() >= 2);
    return data[1];
  }
  const T &b() const {
    assert(data.size() >= 3);
    return data[2];
  }
  const T &a() const {
    assert(data.size() == 4);
    return data[3];
  }

  const T &u() const { return data[0]; }

  const T &v() const {
    assert(data.size() >= 2);
    return data[1];
  }

  // below accessor are pass by value
  vec<2, T> xy() {
    assert(data.size() >= 2);
    return vec<2, T>{data[0], data[1]};
  }

  vec<2, T> rg() {
    assert(data.size() >= 2);
    return vec<2, T>{data[0], data[1]};
  }

  vec<3, T> xyz() {
    assert(data.size() >= 3);
    return vec<3, T>{data[0], data[1], data[2]};
  }

  vec<3, T> rgb() {
    assert(data.size() >= 3);
    return vec<3, T>{data[0], data[1], data[2]};
  }

  T operator[](size_t i) const { return this->data[i]; }
  T &operator[](size_t i) { return this->data[i]; }

  // all 0s by default
  vec() {
    for (int i = 0; i < N; i++) {
      data[i] = 0.f;
    }
  }

  vec(T v) {
    for (int i = 0; i < N; i++) {
      data[i] = v;
    }
  }

  vec(T v1, T v2) {
    data[0] = v1;
    data[1] = v2;
  }

  vec(T v1, T v2, T v3) {
    data[0] = v1;
    data[1] = v2;
    data[2] = v3;
  }

  vec(T v1, T v2, T v3, T v4) {
    data[0] = v1;
    data[1] = v2;
    data[2] = v3;
    data[3] = v4;
  }

  vec(const vec<N, T> &other) {
    for (int i = 0; i < N; i++) {
      data[i] = other[i];
    }
  }

  vec(const vec<N - 1, T> &other, T v) {
    for (int i = 0; i < (N - 1); i++) {
      data[i] = other[i];
    }

    data[N - 1] = v;
  }

  vec(std::initializer_list<T> list) {
    int i = 0;
    for (const auto &v : list) {
      data[i] = v;
      i++;
    }
  }

  // adapt to type float*
  vec<N, T>(const T v[N]) {
    for (int i = 0; i < N; i++) {
      data[i] = v[i];
    }
  }

  bool near_zero(const float s = 1e-6) const {
    if (N == 1)
      return std::fabs(data[0]) < s;
    else if (N == 2)
      return std::fabs(data[0]) < s && std::fabs(data[1]) < s;
    else if (N == 3)
      return std::fabs(data[0]) < s && std::fabs(data[1]) < s &&
             std::fabs(data[2]) < s;
    else
      return std::fabs(data[0]) < s && std::fabs(data[1]) < s &&
             std::fabs(data[2]) < s && std::fabs(data[3]) < s;
  }

  static float average(const vec<N, T> &v) {
    float sum = 0.f;
    for (int i = 0; i < N; i++) {
      sum += v[i];
    }
    return sum / N;
  };

  float average() const {
    float sum = 0.f;
    for (int i = 0; i < N; i++) {
      sum += data[i];
    }
    return sum / N;
  };

#ifdef GL_SIMD
  vec<N, T> operator/(const float divisor) const {
    vec<N, T> result;
    result.vector = _mm_div_ps(this->vector, _mm_set1_ps(divisor));
    return result;
  }

  // divide equal
  vec<N, T> &operator/=(float divisor) {
    this->vector = _mm_div_ps(this->vector, _mm_set1_ps(divisor));
    return *this;
  }

  // minus equal, component wise
  vec<N, T> &operator-=(const vec<N, T> &other) {
    this->vector = _mm_sub_ps(this->vector, other.vector);
    return *this;
  }

  // multiply equal, component wise
  vec<N, T> &operator*=(const vec<N, T> &other) {
    this->vector = _mm_mul_ps(this->vector, other.vector);
    return *this;
  }

  // multiply equal
  vec<N, T> &operator*=(const float multiplier) {
    this->vector = _mm_mul_ps(this->vector, _mm_set1_ps(multiplier));
    return *this;
  }

  // length of a vector
  float length() const { return sqrtf(dot(*this, *this)); }

  // normalize in place
  vec<N, T> &normalized() {
    float length = this->length();
    this->vector = _mm_div_ps(this->vector, _mm_set1_ps(length));

    return *this;
  };

  vec<N, T> normalize() const {
    vec<N, T> result;
    float length = this->length();
    result.vector = _mm_div_ps(this->vector, _mm_set1_ps(length));
    return result;
  };

  // negate
  vec<N, T> operator-() const {
    vec<N, T> result;
    result.vector = _mm_sub_ps(_mm_set1_ps(0.f), this->vector);
    return result;
  }

  // add equal
  vec<N, T> &operator+=(vec<N, T> other) {
    this->vector = _mm_add_ps(this->vector, other.vector);
    return *this;
  }
#else
  vec<N, T> operator/(const float divisor) const {
    vec<N, T> result;
    for (int i = 0; i < N; i++) {
      result[i] = this->data[i] / divisor;
    }
    return result;
  }

  // divide equal
  vec<N, T> &operator/=(float divisor) {
    for (int i = 0; i < N; i++) {
      this->data[i] /= divisor;
    }
    return *this;
  }

  // minus equal, component wise
  vec<N, T> &operator-=(const vec<N, T> &other) {
    for (int i = 0; i < N; i++) {
      this->data[i] -= other.data[i];
    }
    return *this;
  }

  // multiply equal, component wise
  vec<N, T> &operator*=(const vec<N, T> &other) {
    for (int i = 0; i < N; i++) {
      this->data[i] *= other.data[i];
    }
    return *this;
  }

  // multiply equal
  vec<N, T> &operator*=(const float multiplier) {
    for (int i = 0; i < N; i++) {
      this->data[i] *= multiplier;
    }
    return *this;
  }

  // length of a vector
  float length() const { return sqrtf(dot(*this, *this)); }

  // normalize in place
  vec<N, T> &normalized() {
    float length = this->length();
    for (int i = 0; i < N; i++) {
      this->data[i] /= length;
    }
    return *this;
  };

  vec<N, T> normalize() const {
    vec<N, T> result;
    float length = this->length();
    for (int i = 0; i < N; i++) {
      result[i] = this->data[i] / length;
    }
    return result;
  };

  // negate
  vec<N, T> operator-() const {
    vec<N, T> result;
    for (int i = 0; i < N; i++) {
      result[i] = -this->data[i];
    }
    return result;
  }

  // add equal
  vec<N, T> &operator+=(vec<N, T> other) {
    for (int i = 0; i < N; i++) {
      this->data[i] += other.data[i];
    }
    return *this;
  }
#endif
};

template <int N, class T>
bool operator==(const vec<N, T> &lhs, const vec<N, T> &rhs) {
  for (int i = 0; i < N; i++) {
    if (lhs[i] != rhs[i]) {
      return false;
    }
  }
  return true;
}

template <int N, class T>
bool operator!=(const vec<N, T> &lhs, const vec<N, T> &rhs) {
  for (int i = 0; i < N; i++) {
    if (lhs[i] != rhs[i]) {
      return true;
    }
  }
  return false;
}

#ifdef GL_SIMD
template <int N, class T>
vec<N, T> operator+(const vec<N, T> &lhs, const vec<N, T> &rhs) {
  vec<N, T> result;
  result.vector = _mm_add_ps(lhs.vector, rhs.vector);
  return result;
}

template <int N, class T> vec<N, T> operator+(const vec<N, T> &lhs, float add) {
  vec<N, T> result;
  result.vector = _mm_add_ps(lhs.vector, _mm_set1_ps(add));
  return result;
}

template <int N, class T> vec<N, T> operator+(float add, const vec<N, T> &rhs) {
  vec<N, T> result;
  result.vector = _mm_add_ps(_mm_set1_ps(add), rhs.vector);
  return result;
}

template <int N, class T>
vec<N, T> operator-(const vec<N, T> &lhs, const vec<N, T> &rhs) {
  vec<N, T> result;
  result.vector = _mm_sub_ps(lhs.vector, rhs.vector);
  return result;
}

template <int N, class T> vec<N, T> operator-(const vec<N, T> &lhs, float sub) {
  vec<N, T> result;
  result.vector = _mm_sub_ps(lhs.vector, _mm_set1_ps(sub));
  return result;
}

template <int N, class T>
vec<N, T> operator*(const vec<N, T> &lhs, const float multiplier) {
  vec<N, T> result;
  result.vector = _mm_mul_ps(lhs.vector, _mm_set1_ps(multiplier));
  return result;
}

// component wise multiplication
template <int N, class T>
vec<N, T> operator*(const vec<N, T> &lhs, const vec<N, T> &rhs) {
  vec<N, T> result;
  result.vector = _mm_mul_ps(lhs.vector, rhs.vector);
  return result;
}

template <int N, class T>
vec<N, T> operator*(const float multiplier, const vec<N, T> &rhs) {
  vec<N, T> result;
  result.vector = _mm_mul_ps(_mm_set1_ps(multiplier), rhs.vector);
  return result;
}

template <int N, class T>
vec<N, T> operator/(const vec<N, T> &lhs, const vec<N, T> &rhs) {
  for (int i = 0; i < N; i++)
    assert(rhs[i] != 0 && "Component-wise division by zero");
  vec<N, T> result;
  result.vector = _mm_div_ps(lhs.vector, rhs.vector);
  return result;
}

template <int N, class T> vec<N, T> normalize(const vec<N, T> &a) {
  float length = a.length();
  vec<N, T> result;
  result.vector = _mm_div_ps(a.vector, _mm_set1_ps(length));
  return result;
};

template <int N, class T>
static float dot(const vec<N, T> &a, const vec<N, T> &b) {
  auto res = _mm_dp_ps(a.vector, b.vector, 0b01111111);
  return _mm_cvtss_f32(res);
};

#else
template <int N, class T>
vec<N, T> operator+(const vec<N, T> &lhs, const vec<N, T> &rhs) {
  vec<N, T> result;
  for (int i = 0; i < N; i++) {
    result[i] = lhs[i] + rhs[i];
  }
  return result;
}

template <int N, class T> vec<N, T> operator+(const vec<N, T> &lhs, float add) {
  vec<N, T> result;
  for (int i = 0; i < N; i++) {
    result[i] = lhs[i] + add;
  }
  return result;
}

template <int N, class T> vec<N, T> operator+(float add, const vec<N, T> &rhs) {
  vec<N, T> result;
  for (int i = 0; i < N; i++) {
    result[i] = add + rhs[i];
  }
  return result;
}

template <int N, class T>
vec<N, T> operator-(const vec<N, T> &lhs, const vec<N, T> &rhs) {
  vec<N, T> result;
  for (int i = 0; i < N; i++) {
    result[i] = lhs[i] - rhs[i];
  }
  return result;
}

template <int N, class T> vec<N, T> operator-(const vec<N, T> &lhs, float sub) {
  vec<N, T> result;
  for (int i = 0; i < N; i++) {
    result[i] = lhs[i] - sub;
  }
  return result;
}

template <int N, class T>
vec<N, T> operator*(const vec<N, T> &lhs, const float multiplier) {
  vec<N, T> result;
  for (int i = 0; i < N; i++) {
    result[i] = lhs[i] * multiplier;
  }
  return result;
}

// color multiplication
template <int N, class T>
vec<N, T> operator*(const vec<N, T> &lhs, const vec<N, T> &rhs) {
  vec<N, T> result;
  for (int i = 0; i < N; i++) {
    result[i] = lhs[i] * rhs[i];
  }
  return result;
}

template <int N, class T>
vec<N, T> operator*(const float multiplier, const vec<N, T> &rhs) {
  vec<N, T> result;
  for (int i = 0; i < N; i++) {
    result[i] = multiplier * rhs[i];
  }
  return result;
}

template <int N, class T>
vec<N, T> operator/(const vec<N, T> &lhs, const vec<N, T> &rhs) {
  vec<N, T> result;
  for (int i = 0; i < N; i++) {
    assert(rhs[i] != 0 && "Component-wise division by zero");
    result[i] = lhs[i] / rhs[i];
  }
  return result;
}

template <int N, class T> vec<N, T> normalize(const vec<N, T> &a) {
  float length = a.length();
  vec<N, T> result;
  for (int i = 0; i < N; i++) {
    result[i] = a[i] / length;
  }
  return result;
};

template <int N, class T>
static float dot(const vec<N, T> &a, const vec<N, T> &b) {
  float result = 0.0;
  for (int i = 0; i < N; i++) {
    result += a[i] * b[i];
  }
  return result;
};
#endif

template <int N, class T>
std::ostream &operator<<(std::ostream &os, vec<N, T> v) {
  for (int i = 0; i < N; i++) {
    os << v[i] << " ";
  }
  return os;
};

template <int N, class T>
static vec<3, float> cross(const vec<N, T> &a, const vec<N, T> &b) {
  // cross product
  static_assert(N == 3, "cross product is only defined for 3D vectors");
  vec<3, float> res(0.0);
  res[0] = a[1] * b[2] - a[2] * b[1];
  res[1] = a[2] * b[0] - a[0] * b[2];
  res[2] = a[0] * b[1] - a[1] * b[0];
  return res;
};

using vec2 = vec<2, float>;
using vec3 = vec<3, float>;
using vec4 = vec<4, float>;

template <int M, int N, class T> class mat {
  static_assert(N >= 1 && N <= 4, "N must be between 1 and 4");
  static_assert(M >= 1 && M <= 4, "M must be between 1 and 4");

public:
  // default constructor, all 0s
  mat() {
    for (int i = 0; i < M; i++) {
      for (int j = 0; j < N; j++) {
        data[i][j] = 0.f;
      }
    }
  }

  // constructor with a single value, fills the diagonal with that value
  mat(float value) {
    assert(M == N);

    for (int i = 0; i < M; i++) {
      for (int j = 0; j < N; j++) {
        data[i][j] = (i == j) ? value : 0.f;
      }
    }
  }

  mat(std::initializer_list<vec<N, T>> list) {
    assert(list.size() == M);

    int i = 0;
    for (const auto &v : list) {
      data[i] = v;
      i++;
    }
  }

  mat(std::initializer_list<std::initializer_list<T>> list) {
    assert(list.size() == M);

    for (const auto &l : list)
      assert(l.size() == N);

    int i = 0;
    for (const auto &v : list) {
      data[i] = v;
      i++;
    }
  }

  // assign the value by row major
  mat(std::initializer_list<T> list) {
    assert(list.size() == M * N);
    int i = 0;
    for (const auto &v : list) {
      data[i / N][i % N] = v;
      i++;
    }
  }

  vec<N, T> &operator[](int i) {
    assert(i >= 0 && i < M);
    return data[i];
  }

  // matrix multiplication, assuming square matrices
  mat<M, M, T> operator*(mat<M, M, T> other) const {
    mat<M, M, T> result;
    for (int i = 0; i < M; i++) {
      for (int j = 0; j < M; j++) {
        result[i][j] = 0;
        for (int k = 0; k < M; k++) {
          result[i][j] += data[i][k] * other[k][j];
        }
      }
    }
    return result;
  }

  // matrix multiplication with vector
  vec<N, T> operator*(vec<N, T> other) const {
    vec<N, T> result;
    for (int i = 0; i < N; i++) {
      result[i] = 0;
      for (int j = 0; j < N; j++) {
        result[i] += data[i][j] * other[j];
      }
    }
    return result;
  }

  // matrix addition
  mat<M, N, T> operator+(const mat<M, N, T> other) const {
    mat<M, N, T> result;
    for (int i = 0; i < M; i++) {
      for (int j = 0; j < N; j++) {
        result[i][j] = data[i][j] + other[i][j];
      }
    }
    return result;
  }

  // matrix subtraction
  mat<M, N, T> operator-(const mat<M, N, T> other) const {
    mat<M, N, T> result;
    for (int i = 0; i < M; i++) {
      for (int j = 0; j < N; j++) {
        result[i][j] = data[i][j] - other[i][j];
      }
    }
    return result;
  }

  // matrix scalar multiplication
  mat<M, N, T> operator*(const T other) const {
    mat<M, N, T> result;
    for (int i = 0; i < M; i++) {
      for (int j = 0; j < N; j++) {
        result[i][j] = data[i][j] * other;
      }
    }
    return result;
  }

  // matrix scalar division
  mat<M, N, T> operator/(const T other) const {
    mat<M, N, T> result;
    for (int i = 0; i < M; i++) {
      for (int j = 0; j < N; j++) {
        result[i][j] = data[i][j] / other;
      }
    }
    return result;
  }

  // matrix transpose, not in place
  // returns the transposed matrix
  mat<N, M, T> transpose() const {
    mat<N, M, T> result;
    for (int i = 0; i < M; i++) {
      for (int j = 0; j < N; j++) {
        result[j][i] = data[i][j];
      }
    }
    return result;
  }

  // matrix transpose, in place
  // transposes the matrix
  mat<N, M, T> &transposed() {
    *this = transpose();
    return *this;
  }

  // matrix determinant
  T determinant() const {
    static_assert(M == N, "determinant is only defined for square matrices");
    T result = 0;

    // determinant of the matrix
    if (M == 1)
      return data[0][0];
    if (M == 2)
      return data[0][0] * data[1][1] - data[0][1] * data[1][0];
    if (M == 3)
      return data[0][0] * data[1][1] * data[2][2] +
             data[0][1] * data[1][2] * data[2][0] +
             data[0][2] * data[1][0] * data[2][1] -
             data[0][2] * data[1][1] * data[2][0] -
             data[0][1] * data[1][0] * data[2][2] -
             data[0][0] * data[1][2] * data[2][1];
    if (M == 4)
      return data[0][0] * data[1][1] * data[2][2] * data[3][3] +
             data[0][0] * data[1][2] * data[2][3] * data[3][1] +
             data[0][0] * data[1][3] * data[2][1] * data[3][2] +
             data[0][1] * data[1][0] * data[2][3] * data[3][2] +
             data[0][1] * data[1][2] * data[2][0] * data[3][3] +
             data[0][1] * data[1][3] * data[2][2] * data[3][0] +
             data[0][2] * data[1][0] * data[2][1] * data[3][3] +
             data[0][2] * data[1][1] * data[2][3] * data[3][0] +
             data[0][2] * data[1][3] * data[2][0] * data[3][1] +
             data[0][3] * data[1][0] * data[2][2] * data[3][1] +
             data[0][3] * data[1][1] * data[2][0] * data[3][2] +
             data[0][3] * data[1][2] * data[2][1] * data[3][0] -
             data[0][3] * data[1][1] * data[2][2] * data[3][0] -
             data[0][3] * data[1][2] * data[2][0] * data[3][1] -
             data[0][3] * data[1][0] * data[2][1] * data[3][2] -
             data[0][2] * data[1][1] * data[2][3] * data[3][0] -
             data[0][2] * data[1][3] * data[2][0] * data[3][1] -
             data[0][2] * data[1][0] * data[2][1] * data[3][3] -
             data[0][1] * data[1][0] * data[2][2] * data[3][3] -
             data[0][1] * data[1][2] * data[2][3] * data[3][0] -
             data[0][1] * data[1][3] * data[2][0] * data[3][2] -
             data[0][0] * data[1][1] * data[2][3] * data[3][2] -
             data[0][0] * data[1][2] * data[2][1] * data[3][3] -
             data[0][0] * data[1][3] * data[2][2] * data[3][1];

    return result;
  }

  // matrix inverse
  mat<M, N, T> inverse() const {
    static_assert(M == N, "inverse is only defined for square matrices");
    mat<M, N, T> result;
    T det = determinant();

    if (det == 0) {
      std::cout << "matrix is not invertible" << std::endl;
      return mat<M, N, T>(1.0f);
      // throw std::runtime_error("matrix is not invertible");
    }

    // inverse of the matrix
    if (M == 1)
      result[0][0] = 1 / data[0][0];

    else if (M == 2) {
      result[0][0] = data[1][1] / det;
      result[0][1] = -data[0][1] / det;
      result[1][0] = -data[1][0] / det;
      result[1][1] = data[0][0] / det;
    }

    else if (M == 3) {
      result[0][0] = (data[1][1] * data[2][2] - data[1][2] * data[2][1]) / det;
      result[0][1] = (data[0][2] * data[2][1] - data[0][1] * data[2][2]) / det;
      result[0][2] = (data[0][1] * data[1][2] - data[0][2] * data[1][1]) / det;
      result[1][0] = (data[1][2] * data[2][0] - data[1][0] * data[2][2]) / det;
      result[1][1] = (data[0][0] * data[2][2] - data[0][2] * data[2][0]) / det;
      result[1][2] = (data[0][2] * data[1][0] - data[0][0] * data[1][2]) / det;
      result[2][0] = (data[1][0] * data[2][1] - data[1][1] * data[2][0]) / det;
      result[2][1] = (data[0][1] * data[2][0] - data[0][0] * data[2][1]) / det;
      result[2][2] = (data[0][0] * data[1][1] - data[0][1] * data[1][0]) / det;
    }

    else if (M == 4) {
      result[0][0] = (data[1][1] * data[2][2] * data[3][3] +
                      data[1][2] * data[2][3] * data[3][1] +
                      data[1][3] * data[2][1] * data[3][2] -
                      data[1][3] * data[2][2] * data[3][1] -
                      data[1][2] * data[2][1] * data[3][3] -
                      data[1][1] * data[2][3] * data[3][2]) /
                     det;
      result[0][1] = (data[0][3] * data[2][2] * data[3][1] +
                      data[0][2] * data[2][1] * data[3][3] +
                      data[0][1] * data[2][3] * data[3][2] -
                      data[0][1] * data[2][2] * data[3][3] -
                      data[0][2] * data[2][3] * data[3][1] -
                      data[0][3] * data[2][1] * data[3][2]) /
                     det;
      result[0][2] = (data[0][1] * data[1][2] * data[3][3] +
                      data[0][2] * data[1][3] * data[3][1] +
                      data[0][3] * data[1][1] * data[3][2] -
                      data[0][3] * data[1][2] * data[3][1] -
                      data[0][2] * data[1][1] * data[3][3] -
                      data[0][1] * data[1][3] * data[3][2]) /
                     det;
      result[0][3] = (data[0][3] * data[1][2] * data[2][1] +
                      data[0][2] * data[1][1] * data[2][3] +
                      data[0][1] * data[1][3] * data[2][2] -
                      data[0][1] * data[1][2] * data[2][3] -
                      data[0][2] * data[1][3] * data[2][1] -
                      data[0][3] * data[1][1] * data[2][2]) /
                     det;
      result[1][0] = (data[1][3] * data[2][2] * data[3][0] +
                      data[1][2] * data[2][0] * data[3][3] +
                      data[1][0] * data[2][3] * data[3][2] -
                      data[1][0] * data[2][2] * data[3][3] -
                      data[1][2] * data[2][3] * data[3][0] -
                      data[1][3] * data[2][0] * data[3][2]) /
                     det;
      result[1][1] = (data[0][0] * data[2][2] * data[3][3] +
                      data[0][2] * data[2][3] * data[3][0] +
                      data[0][3] * data[2][0] * data[3][2] -
                      data[0][3] * data[2][2] * data[3][0] -
                      data[0][2] * data[2][0] * data[3][3] -
                      data[0][0] * data[2][3] * data[3][2]) /
                     det;
      result[1][2] = (data[0][3] * data[1][2] * data[3][0] +
                      data[0][2] * data[1][0] * data[3][3] +
                      data[0][0] * data[1][3] * data[3][2] -
                      data[0][0] * data[1][2] * data[3][3] -
                      data[0][2] * data[1][3] * data[3][0] -
                      data[0][3] * data[1][0] * data[3][2]) /
                     det;
      result[1][3] = (data[0][0] * data[1][2] * data[2][3] +
                      data[0][2] * data[1][3] * data[2][0] +
                      data[0][3] * data[1][0] * data[2][2] -
                      data[0][3] * data[1][2] * data[2][0] -
                      data[0][2] * data[1][0] * data[2][3] -
                      data[0][0] * data[1][3] * data[2][2]) /
                     det;
      result[2][0] = (data[1][0] * data[2][1] * data[3][3] +
                      data[1][1] * data[2][3] * data[3][0] +
                      data[1][3] * data[2][0] * data[3][1] -
                      data[1][3] * data[2][1] * data[3][0] -
                      data[1][1] * data[2][0] * data[3][3] -
                      data[1][0] * data[2][3] * data[3][1]) /
                     det;
      result[2][1] = (data[0][3] * data[2][1] * data[3][0] +
                      data[0][1] * data[2][0] * data[3][3] +
                      data[0][0] * data[2][3] * data[3][1] -
                      data[0][0] * data[2][1] * data[3][3] -
                      data[0][1] * data[2][3] * data[3][0] -
                      data[0][3] * data[2][0] * data[3][1]) /
                     det;
      result[2][2] = (data[0][0] * data[1][1] * data[3][3] +
                      data[0][1] * data[1][3] * data[3][0] +
                      data[0][3] * data[1][0] * data[3][1] -
                      data[0][3] * data[1][1] * data[3][0] -
                      data[0][1] * data[1][0] * data[3][3] -
                      data[0][0] * data[1][3] * data[3][1]) /
                     det;
      result[2][3] = (data[0][3] * data[1][1] * data[2][0] +
                      data[0][1] * data[1][0] * data[2][3] +
                      data[0][0] * data[1][3] * data[2][1] -
                      data[0][0] * data[1][1] * data[2][3] -
                      data[0][1] * data[1][3] * data[2][0] -
                      data[0][3] * data[1][0] * data[2][1]) /
                     det;
      result[3][0] = (data[1][1] * data[2][2] * data[3][0] +
                      data[1][2] * data[2][0] * data[3][1] +
                      data[1][0] * data[2][1] * data[3][2] -
                      data[1][0] * data[2][2] * data[3][1] -
                      data[1][2] * data[2][1] * data[3][0] -
                      data[1][1] * data[2][0] * data[3][2]) /
                     det;
      result[3][1] = (data[0][0] * data[2][2] * data[3][1] +
                      data[0][2] * data[2][1] * data[3][0] +
                      data[0][1] * data[2][0] * data[3][2] -
                      data[0][1] * data[2][2] * data[3][0] -
                      data[0][2] * data[2][0] * data[3][1] -
                      data[0][0] * data[2][1] * data[3][2]) /
                     det;
      result[3][2] = (data[0][1] * data[1][2] * data[3][0] +
                      data[0][2] * data[1][0] * data[3][1] +
                      data[0][0] * data[1][1] * data[3][2] -
                      data[0][0] * data[1][2] * data[3][1] -
                      data[0][2] * data[1][1] * data[3][0] -
                      data[0][1] * data[1][0] * data[3][2]) /
                     det;
      result[3][3] = (data[0][0] * data[1][1] * data[2][2] +
                      data[0][1] * data[1][2] * data[2][0] +
                      data[0][2] * data[1][0] * data[2][1] -
                      data[0][2] * data[1][1] * data[2][0] -
                      data[0][1] * data[1][0] * data[2][2] -
                      data[0][0] * data[1][2] * data[2][1]) /
                     det;
    }

    // inverse the matrix
    return result;
  };

  // inplace inverse
  mat<M, N, T> &inversed() {
    *this = inverse();
    return *this;
  };

private:
  // row major definition
  std::array<vec<N, T>, M> data;
};

template <int M, int N, class T>
std::ostream &operator<<(std::ostream &os, mat<M, N, T> m) {
  for (int i = 0; i < M; i++) {
    for (int j = 0; j < N; j++) {
      os << m[i][j] << " ";
    }
    os << std::endl;
  }
  return os;
};

using mat2 = mat<2, 2, float>;
using mat3 = mat<3, 3, float>;
using mat4 = mat<4, 4, float>;
}; // namespace gl

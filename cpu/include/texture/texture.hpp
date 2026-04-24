#pragma once
#include "utils/matrix.hpp"
#include "utils/pattern.hpp"
#include "utils/utility.hpp"
#include "utils/random.hpp"
#include <string>
#include <variant>
#include <memory>

class Texture2D;
class ConstantTexture;

enum class LERP_MODE
{
  NEAREST,
  BILINEAR,
  CORNER
};

class Texture2D
{
public:
  Texture2D() = default;
  ~Texture2D() = default;
  virtual gl::vec3 getTexelColor(float u, float v,
                                 LERP_MODE mode = LERP_MODE::BILINEAR) const = 0;
  virtual gl::vec3 getTexelColor(gl::vec2 uv,
                                 LERP_MODE mode = LERP_MODE::BILINEAR) const
  {
    return this->getTexelColor(uv.x(), uv.y(), mode);
  }
};

class ConstantTexture : public Texture2D
{
public:
  ConstantTexture() = default;
  ConstantTexture(const gl::vec3 color) : _color(color) {};
  ~ConstantTexture() = default;
  gl::vec3 getTexelColor(float u, float v,
                         LERP_MODE mode = LERP_MODE::BILINEAR) const override
  {
    return this->_color;
  }

private:
  gl::vec3 _color;
};

class ImageTexture : public Texture2D
{
public:
  ImageTexture() = default;
  // by default rgb
  ImageTexture(const std::string &filename, bool flip_y = false, bool isHDR = false);
  ~ImageTexture() = default;
  virtual gl::vec3 getTexelColor(float u, float v,
                                 LERP_MODE mode = LERP_MODE::BILINEAR) const override;

private:
  std::string _path;
  std::vector<std::vector<gl::vec3>> texels;
  uint _width;
  uint _height;
};

class CheckerTexture : public Texture2D
{
public:
  CheckerTexture(float scale = 10.f);
  CheckerTexture(gl::vec3 color1, gl::vec3 color2, float scale = 10.f);
  ~CheckerTexture() = default;
  gl::vec3 getTexelColor(float u, float v,
                         LERP_MODE mode = LERP_MODE::BILINEAR) const override;

private:
  gl::vec3 _color1;
  gl::vec3 _color2;
  float _scale;
};

class NoiseTexture : public Texture2D
{
public:
  NoiseTexture() = default;
  NoiseTexture(float scale, int fractal = 1)
      : _scale(scale), _fractal(fractal) {};
  ~NoiseTexture() = default;
  gl::vec3 getTexelColor(float u, float v,
                         LERP_MODE mode = LERP_MODE::BILINEAR) const override;

private:
  float _scale;
  int _fractal;
};

class HDRITexture : public ImageTexture
{
public:
  HDRITexture() = default;
  HDRITexture(const std::string &filename, bool flip_y = false, bool isHDR = true)
      : ImageTexture(filename, flip_y, isHDR) {};
  ~HDRITexture() = default;
  // / Sample the HDRI given a world-space direction vector
  gl::vec3 getTexelColor(const gl::vec3 &direction) const
  {
    gl::vec3 dir_norm = direction.normalize();
    float theta = std::acos(-dir_norm.y());                     // theta from [0, PI] (angle with -Y)
    float phi = std::atan2(-dir_norm.z(), dir_norm.x()) + M_PI; // phi from [0, 2PI]

    // Convert (phi, theta) to UV coordinates [0,1]x[0,1]
    float u_coord = phi / (2.0f * M_PI);
    float v_coord = theta / M_PI;
    return ImageTexture::getTexelColor(u_coord, v_coord, LERP_MODE::BILINEAR);
  };

  // Method to sample a direction from the HDRI (for importance sampling the environment map)
  // Returns a sampled direction and its PDF (w.r.t. solid angle)
  // This requires building a distribution (e.g., from image luminance)
  void sample_direction(float u1, float u2, gl::vec3 &sampled_dir, float &pdf_solid_angle)
  {
    sampled_dir = gl::uniformSampleSphere(u1, u2);
    pdf_solid_angle = 1.f / (4.f * M_PI);
  }

  float pdf_direction(const gl::vec3 &dir) const
  {
    // PDF for uniform spherical sampling
    return 1.f / (4.f * M_PI);
    // For proper importance sampling, this would return PDF based on image luminance.
  }
};

using ColorVariant = std::variant<gl::vec3, std::shared_ptr<Texture2D>>;
namespace gl::texture
{
  inline std::shared_ptr<Texture2D>
  to_texture2d(const ColorVariant &color_like)
  {
    return std::visit(
        [](const auto &color_arg) -> std::shared_ptr<Texture2D>
        {
          if constexpr (std::is_same_v<std::decay_t<decltype(color_arg)>, gl::vec3>)
          {
            return std::make_shared<ConstantTexture>(color_arg);
          }
          else
          { // This branch is taken if color_arg is const std::shared_ptr<Texture2D>&
            if (!color_arg)
            {
              // Handle the case where a nullptr std::shared_ptr<Texture2D> was passed in the variant
              // Option: return a default texture, throw, or allow nullptr to propagate.
              // Propagating nullptr is often acceptable, caller must check.
              // For example, returning a default black texture:
              // return std::make_shared<ConstantTexture>(gl::vec3{0.0f, 0.0f, 0.0f});
              // Or simply return the null pointer:
              return nullptr;
            }
            return color_arg; // Returns the std::shared_ptr<Texture2D>
          }
        },
        color_like);
  };
};
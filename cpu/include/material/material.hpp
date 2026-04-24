#pragma once
#include "base/ray.hpp"
#include "texture/texture.hpp"
#include "external/scene_io.hpp"
#include "materialBase.hpp"
#include "materialMath.hpp"
#include "utils/orthoBasis.hpp"

class Lambertian;
class Mirror;
class Dielectric;
class Phong;
class PhongLike;

class Material
{
public:
  virtual bool
  scatter(const Ray &ray_in, HitRecord &rec, ScatterRecord &srec,
          float uc = gl::rand_num(), // coin-flip sample
          const gl::vec2 &u = {gl::rand_num(),
                               gl::rand_num()}, // 2D microfacet sample
          TransportMode mode = TransportMode::Radiance,
          BxDFReflTransFlags flags = BxDFReflTransFlags::All) const
  {
    return false;
  }

  // required for direct light sampling (non-delta materials), pure bxdf
  // evaluations. Note that we conform to the PBRT convention of
  // wo_world = -ray_in.getDirection()
  virtual gl::vec3 f(const gl::vec3 &wo_world, const gl::vec3 &wi_world,
                     const HitRecord &rec,
                     TransportMode mode = TransportMode::Radiance) const
  {
    return gl::vec3(0.0f);
  };

  // required for non-delta materials, this assume the pdf_ptr must be provided
  // optional to override
  virtual float
  scatter_pdf(const ScatterRecord &srec, const Ray &wi_world,
              TransportMode mode = TransportMode::Radiance,
              BxDFReflTransFlags flags = BxDFReflTransFlags::All) const
  {
    // if the pdf_ptr is not provided, and the scattering is not specular,
    if (srec.pdf_ptr == nullptr && (!srec.is_specular()))
      throw std::runtime_error("pdf_ptr is not provided, and the scattering is not specular");
    if (srec.pdf_ptr == nullptr)
      return 0.f;
    return srec.pdf_ptr->at(wi_world.getDirection().normalize());
  }

  // optional to override,usually required for multiple lobe materials
  virtual float
  scatter_pdf(const gl::vec3 &wo_world, const gl::vec3 &wi_world, const HitRecord &rec,
              TransportMode mode = TransportMode::Radiance,
              BxDFReflTransFlags flags = BxDFReflTransFlags::All) const
  {
    return 0.f;
  }

  virtual gl::vec3 emit(const Ray &ray_in, HitRecord &rec) const
  {
    return gl::vec3(0.0f);
  }

  virtual bool is_emitter() const { return false; }

  // return the emission properties of the material, used for light-conversion
  virtual void get_emission_properties(std::shared_ptr<Texture2D> &out_texture, float &out_intensity) const
  {
    out_texture = nullptr;
    out_intensity = 0.f;
    return;
  }
};

class Lambertian : public Material
{

public:
  Lambertian(const ColorVariant &a)
  {
    albedo = gl::texture::to_texture2d(a);
  }
  // scatter the ray with lambertian reflection
  bool
  scatter(const Ray &ray_in, HitRecord &rec, ScatterRecord &srec,
          float uc = gl::rand_num(), // coin-flip sample
          const gl::vec2 &u = {gl::rand_num(),
                               gl::rand_num()}, // 2D microfacet sample
          TransportMode mode = TransportMode::Radiance,
          BxDFReflTransFlags flags = BxDFReflTransFlags::All) const override
  {
    if (!(flags & BxDFReflTransFlags::Reflection))
      return false;

    srec.sampled_type = BxDFFlags::DiffuseReflection;
    srec.attenuation = albedo->getTexelColor(rec.texCoords) * (1.0f / M_PI);
    srec.pdf_ptr = std::make_shared<CosinePDF>(rec.normal);
    srec.sampled_ray = Ray(rec.position, srec.pdf_ptr->get(uc, u).normalize());
    srec.pdf_val = srec.pdf_ptr->at(srec.sampled_ray.getDirection());
    return true;
  }

  float scatter_pdf(
      const ScatterRecord &srec, const Ray &wi_world,
      TransportMode mode = TransportMode::Radiance,
      BxDFReflTransFlags flags = BxDFReflTransFlags::All) const override
  {
    if (!(flags & BxDFReflTransFlags::Reflection))
      return 0.f;
    if (srec.pdf_ptr == nullptr)
      return 0.f;
    return srec.pdf_ptr->at(wi_world.getDirection().normalize());
  }

  float scatter_pdf(const gl::vec3 &wo_world, const gl::vec3 &wi_world,
                    const HitRecord &rec,
                    TransportMode mode = TransportMode::Radiance,
                    BxDFReflTransFlags flags = BxDFReflTransFlags::All) const override
  {
    if (!(flags & BxDFReflTransFlags::Reflection))
      return 0.f;
    return std::make_shared<CosinePDF>(rec.normal)->at(wi_world);
  }

  gl::vec3 f(const gl::vec3 &wo_world, const gl::vec3 &wi_world,
             const HitRecord &rec,
             TransportMode mode = TransportMode::Radiance) const override
  {
    return albedo->getTexelColor(rec.texCoords) * (1.0f / M_PI);
  }

  std::shared_ptr<Texture2D> albedo;
};

class Mirror : public Material
{
public:
  Mirror(const gl::vec3 &a, float f = 0.f) : albedo(a) { fuzz = f < 1 ? f : 1; }
  // it's hard to use a delta functinon as BRDF,
  // since the floating point precision causes trouble
  // This design is a reference from Raytracing the rest of your life
  bool
  scatter(const Ray &ray_in, HitRecord &rec, ScatterRecord &srec,
          float uc = gl::rand_num(), // coin-flip sample
          const gl::vec2 &u = {gl::rand_num(),
                               gl::rand_num()}, // 2D microfacet sample
          TransportMode mode = TransportMode::Radiance,
          BxDFReflTransFlags flags = BxDFReflTransFlags::All) const override
  {
    if (!(flags & BxDFReflTransFlags::Reflection))
      return false;

    gl::vec3 reflected =
        reflect(ray_in.getDirection().normalize(), rec.normal) +
        gl::on_sphere_random_vec(fuzz);
    srec.sampled_ray = Ray(rec.position, reflected);
    srec.sampled_type = BxDFFlags::SpecularReflection;
    srec.attenuation = albedo;
    srec.pdf_ptr = nullptr;
    srec.pdf_val = 0.0f; // delta function
    return true;
  }

  gl::vec3 albedo;
  float fuzz;
};

class Dielectric : public Material
{
public:
  float ior;
  Dielectric(float ior) : ior(ior) {};

  bool
  scatter(const Ray &ray_in, HitRecord &rec, ScatterRecord &srec,
          float uc = gl::rand_num(), // coin-flip sample
          const gl::vec2 &u = {gl::rand_num(),
                               gl::rand_num()}, // 2D microfacet sample
          TransportMode mode = TransportMode::Radiance,
          BxDFReflTransFlags flags = BxDFReflTransFlags::All) const override
  {
    srec.pdf_ptr = nullptr;
    srec.attenuation = gl::vec3(1.0f);
    float ri_ro = rec.is_inside ? 1.0f / ior : ior;
    bool is_refract = false;
    gl::vec3 out_ray = refract(ray_in.getDirection().normalize(), rec.normal,
                               ri_ro, is_refract);
    srec.sampled_type = is_refract ? BxDFFlags::SpecularTransmission
                                   : BxDFFlags::SpecularReflection;
    srec.sampled_ray = Ray(rec.position, out_ray);
    srec.pdf_val = 0.0f; // delta function
    return true;
  };
};

class PhongLike : public Material
{
public:
  PhongLike(const gl::vec3 &diffuse, const gl::vec3 &specular,
            const gl::vec3 &ambient, float shininess = 0.5f, float fuzz = 0.2f)
      : diffuse(diffuse), specular(specular), ambient(ambient),
        specularProb(shininess) // shininess is used as a probability threshold
        ,
        fuzz(fuzz)
  {
  }

  gl::vec3 diffuse;   // Diffuse reflectance color
  gl::vec3 specular;  // Specular reflectance color
  gl::vec3 ambient;   // Ambient component
  float specularProb; //
  float fuzz;         // Fuzziness for the specular highlight

  // scatter() decides how a ray scatters upon hitting the surface.
  // It probabilistically selects between specular and diffuse scattering.
  bool
  scatter(const Ray &ray_in, HitRecord &rec, ScatterRecord &srec,
          float uc = gl::rand_num(), // coin-flip sample
          const gl::vec2 &u = {gl::rand_num(),
                               gl::rand_num()}, // 2D microfacet sample
          TransportMode mode = TransportMode::Radiance,
          BxDFReflTransFlags flags = BxDFReflTransFlags::All) const override
  {

    if (uc < specularProb)
    {
      // Specular scattering branch:
      srec.sampled_type = BxDFFlags::SpecularReflection;
      // Compute the perfect reflection direction.
      gl::vec3 R = reflect(ray_in.getDirection().normalize(), rec.normal);
      // Add fuzziness to the reflection direction.
      R += gl::on_sphere_random_vec(fuzz);
      R.normalized();
      // Create a new ray in the specular direction.
      srec.sampled_ray = Ray(rec.position, R);
      // For specular, set attenuation as the sum of specular and ambient
      // components.
      srec.attenuation = specular + ambient;
      srec.pdf_ptr = nullptr; // Delta distribution; no PDF used.
      srec.pdf_val = 0.0f;    // Delta function, so PDF is zero.
      return true;
    }
    else
    {
      srec.attenuation = diffuse + ambient;
      // Use a cosine-weighted PDF for diffuse scattering.
      srec.pdf_ptr = std::make_shared<CosinePDF>(rec.normal);
      // Sample a direction from the cosine-weighted PDF.
      srec.sampled_ray =
          Ray(rec.position, srec.pdf_ptr->get(uc, u).normalize());
      srec.sampled_type = BxDFFlags::DiffuseReflection;
      // Set the PDF value for the sampled direction.
      srec.pdf_val = srec.pdf_ptr->at(srec.sampled_ray.getDirection());
      return true;
    }
  }

  gl::vec3 f(const gl::vec3 &wo_world, const gl::vec3 &wi_world,
             const HitRecord &rec,
             TransportMode mode = TransportMode::Radiance) const override
  {
    float p = gl::rand_num();
    if (p < specularProb)
    {
      return gl::vec3(0.0f);
    }
    // f = rho/pi
    return diffuse + ambient;
  }

  float scatter_pdf(const gl::vec3 &wo_world, const gl::vec3 &wi_world,
                    const HitRecord &rec,
                    TransportMode mode = TransportMode::Radiance,
                    BxDFReflTransFlags flags = BxDFReflTransFlags::All) const override
  {
    using namespace gl;
    if (!(flags & BxDFReflTransFlags::Reflection)) // PhongLike is purely reflective
      return 0.f;

    float prob_diffuse = 1.0f - specularProb; // Probability of choosing the diffuse path

    // If diffuse is chosen, the PDF is a CosinePDF.
    // We need to evaluate this CosinePDF for the given wi_world.
    // Ensure wi_world is normalized.
    vec3 wi_world_norm = wi_world.normalize();
    float cos_theta_i = dot(wi_world_norm, rec.normal.normalize());

    if (cos_theta_i <= 0) // Diffuse reflection must be in the hemisphere of the normal
      return 0.f;

    float pdf_diffuse_lobe = cos_theta_i / M_PI;

    // The overall PDF is P(choosing diffuse) * PDF_diffuse(wi | wo)
    // Since PDF_diffuse is independent of wo for Lambertian:
    return prob_diffuse * pdf_diffuse_lobe;
  }
};

class Phong : public Material
{
public:
  // Constructor: diffuse - diffuse color, specular - specular color,
  // ambient - ambient component, shininess - exponent for the specular lobe.
  Phong(const gl::vec3 &diffuse, const gl::vec3 &specular,
        const gl::vec3 &ambient, float shininess)
      : diffuse(diffuse), specular(specular), ambient(ambient),
        shininess(shininess) {}

  gl::vec3 diffuse;  // Diffuse reflectance color.
  gl::vec3 specular; // Specular reflectance color.
  gl::vec3 ambient;  // Ambient component.
  float shininess;   // Shininess exponent for specular highlight.

  // scatter() decides how the ray is scattered upon hitting the surface.
  // This implementation uses importance sampling over two lobes: specular
  // (Phong) and diffuse (cosine-weighted). The branch is chosen based on the
  // relative energies.
  bool
  scatter(const Ray &ray_in, HitRecord &rec, ScatterRecord &srec,
          float uc = gl::rand_num(), // coin-flip sample
          const gl::vec2 &u = {gl::rand_num(),
                               gl::rand_num()}, // 2D microfacet sample
          TransportMode mode = TransportMode::Radiance,
          BxDFReflTransFlags flags = BxDFReflTransFlags::All) const override
  {
    // Compute average intensity for specular and diffuse components.
    float specInt = (specular.x() + specular.y() + specular.z()) / 3.0f;
    float diffInt = (diffuse.x() + diffuse.y() + diffuse.z()) / 3.0f;
    // Use energy-based probability for choosing specular sampling.
    float specProb = specInt / (specInt + diffInt);

    float r = gl::rand_num();
    if (r < specProb)
    {
      // Specular (glossy) branch (non-delta, with a finite Phong lobe).
      srec.sampled_type = BxDFFlags::GlossyReflection;
      // Compute the perfect reflection direction.
      gl::vec3 R = reflect(ray_in.getDirection().normalize(), rec.normal);
      // Set up a Phong-lobe PDF with the perfect reflection direction.
      srec.pdf_ptr = std::make_shared<PhongLobePDF>(R, shininess);
      // Sample a specular direction from the Phong-lobe PDF.
      gl::vec3 sampledDir = srec.pdf_ptr->get(uc, u);
      srec.sampled_ray = Ray(rec.position, sampledDir);
      // Attenuation is typically set to the specular color (ambient might be
      // added separately).
      srec.attenuation = specular;
      // PDF value is computed based on the sampled direction.
      srec.pdf_val = srec.pdf_ptr->at(sampledDir);
      // Note: The PDF value is not used in this case, but it's good practice
      return true;
    }
    else
    {
      // Diffuse branch.
      srec.sampled_type = BxDFFlags::DiffuseReflection;
      srec.attenuation = diffuse;
      srec.pdf_ptr = std::make_shared<CosinePDF>(rec.normal);
      // Sample a direction from the cosine-weighted PDF.
      gl::vec3 sampledDir = srec.pdf_ptr->get(uc, u);
      srec.sampled_ray = Ray(rec.position, sampledDir);
      return true;
    }
  }

  gl::vec3 f(const gl::vec3 &wo_world, const gl::vec3 &wi_world,
             const HitRecord &rec,
             TransportMode mode = TransportMode::Radiance) const override
  {
    using namespace gl;

    vec3 wo = wo_world.normalize(); // toward camera
    vec3 wi = wi_world.normalize(); // toward light

    // 2) Reject below‐surface
    float cosThetaI = dot(rec.normal, wi);
    if (cosThetaI <= 0)
    {
      return vec3(0.f);
    }

    // 3) Energy‐based mixing weight
    float specInt = (specular.x() + specular.y() + specular.z()) / 3.0f;
    float diffInt = (diffuse.x() + diffuse.y() + diffuse.z()) / 3.0f;
    float specProb = specInt / (specInt + diffInt);

    // 4) Diffuse term: Lambertian
    //    f_diffuse = diffuse / π
    vec3 diffBRDF = diffuse * (1.0f / M_PI);

    // 5) Specular term: Phong lobe around the perfect reflection
    //    R = reflect(wo, N)
    vec3 R = gl::pbrt::reflect(wo, rec.normal);
    float cosAlpha = std::max(dot(wi, R), 0.0f);
    float phongTerm = std::pow(cosAlpha, shininess);
    //    normalization = (shininess + 2) / (2π)
    float specNorm = (shininess + 2.0f) / (2.0f * M_PI);
    vec3 specBRDF = specular * (specNorm * phongTerm);

    // 6) Mix BRDF
    vec3 f_val = specProb * specBRDF + (1.0f - specProb) * diffBRDF;

    // 7) Compute PDF for this wi, matching scatter_pdf()
    //    pdf_spec = (shininess+1)/(2π) * cosAlpha^shininess
    float specPDF = (shininess + 1.0f) / (2.0f * M_PI) * phongTerm;
    //    pdf_diff = cosθᵢ / π
    float diffPDF = cosThetaI / M_PI;

    return f_val;
  }

  // In class Phong
  float scatter_pdf(const gl::vec3 &wo_world, const gl::vec3 &wi_world,
                    const HitRecord &rec,
                    TransportMode mode = TransportMode::Radiance,
                    BxDFReflTransFlags flags = BxDFReflTransFlags::All) const override
  {
    using namespace gl;
    if (!(flags & BxDFReflTransFlags::Reflection)) // Phong is purely reflective
      return 0.f;

    // Normalize input directions
    vec3 wo_world_norm = wo_world.normalize();
    vec3 wi_world_norm = wi_world.normalize();

    // Compute energy-based probability for choosing specular sampling (same as in scatter)
    float specInt = (specular.x() + specular.y() + specular.z()) / 3.0f;
    float diffInt = (diffuse.x() + diffuse.y() + diffuse.z()) / 3.0f;
    float totalInt = specInt + diffInt;
    if (totalInt == 0.f)
      return 0.f; // Avoid division by zero if both are black

    float prob_specular_choice = specInt / totalInt;
    float prob_diffuse_choice = diffInt / totalInt; // Or 1.0f - prob_specular_choice

    // PDF from the specular (Phong lobe) branch
    float pdf_specular_lobe = 0.f;
    if (prob_specular_choice > 0)
    {
      vec3 R_perfect = reflect(-wo_world_norm, rec.normal.normalize()); // ray_in.direction = -wo_world
      // The PhongLobePDF is centered around R_perfect.
      // Its 'at' method computes (shininess+1)/(2*PI) * cos^shininess(angle between wi and R)
      PhongLobePDF phong_pdf_obj(R_perfect, shininess);
      pdf_specular_lobe = phong_pdf_obj.at(wi_world_norm);
    }

    // PDF from the diffuse (Cosine lobe) branch
    float pdf_diffuse_lobe = 0.f;
    if (prob_diffuse_choice > 0)
    {
      float cos_theta_i = dot(wi_world_norm, rec.normal.normalize());
      if (cos_theta_i > 0)
      { // Diffuse reflection must be in the hemisphere of the normal
        pdf_diffuse_lobe = cos_theta_i / M_PI;
      }
    }

    // The overall PDF is the weighted sum of the individual lobe PDFs
    // PDF_overall = P(choose specular) * PDF_specular(wi | wo) + P(choose diffuse) * PDF_diffuse(wi | wo)
    return prob_specular_choice * pdf_specular_lobe + prob_diffuse_choice * pdf_diffuse_lobe;
  }
};

class DiffuseEmitter : public Material
{
public:
  DiffuseEmitter(const ColorVariant &a, float intensity = 1.0f)
      : _intensity(intensity)
  {
    _text = gl::texture::to_texture2d(a);
  }

  bool
  scatter(const Ray &ray_in, HitRecord &rec, ScatterRecord &srec,
          float uc = gl::rand_num(), // coin-flip sample
          const gl::vec2 &u = {gl::rand_num(),
                               gl::rand_num()}, // 2D microfacet sample
          TransportMode mode = TransportMode::Radiance,
          BxDFReflTransFlags flags = BxDFReflTransFlags::All) const override
  {
    return false;
  }

  gl::vec3 emit(const Ray &ray_in, HitRecord &rec) const override
  {
    // use unidirectional light or not
    if (gl::dot(rec.normal, -ray_in.getDirection()) > 0)
      return _text->getTexelColor(rec.texCoords) * _intensity;
    return gl::vec3(0.0f);
  }

  bool is_emitter() const override { return true; }

  virtual void get_emission_properties(std::shared_ptr<Texture2D> &out_texture, float &out_intensity) const override
  {
    out_texture = _text;
    out_intensity = _intensity;
  }

private:
  std::shared_ptr<Texture2D> _text;
  float _intensity;
};

class DebugTangentMaterial : public Material
{
public:
  DebugTangentMaterial() = default;

  bool
  scatter(const Ray &, HitRecord &rec, ScatterRecord &srec,
          float uc = gl::rand_num(), // coin-flip sample
          const gl::vec2 &u = {gl::rand_num(),
                               gl::rand_num()}, // 2D microfacet sample
          TransportMode mode = TransportMode::Radiance,
          BxDFReflTransFlags flags = BxDFReflTransFlags::All) const override
  {
    // no scattering—just treat this as an emitter so we see it directly
    return false;
  }

  gl::vec3 emit(const Ray &ray, HitRecord &rec) const override
  {
    auto t = rec.hair_tangent;
    return gl::abs(t);
  }

  bool is_emitter() const override { return true; }
};

class DebugNormalMaterial : public Material
{
public:
  DebugNormalMaterial() = default;

  bool
  scatter(const Ray &, HitRecord &rec, ScatterRecord &srec,
          float uc = gl::rand_num(), // coin-flip sample
          const gl::vec2 &u = {gl::rand_num(),
                               gl::rand_num()}, // 2D microfacet sample
          TransportMode mode = TransportMode::Radiance,
          BxDFReflTransFlags flags = BxDFReflTransFlags::All) const override
  {
    // no scattering—just treat this as an emitter so we see it directly
    return false;
  }

  gl::vec3 emit(const Ray &ray, HitRecord &rec) const override
  {
    return rec.normal * 0.5f + 0.5f;
    // return rec.normal;
  }

  bool is_emitter() const override { return true; }
};

// class Isotropic : public Material
// {
// public:
//   Isotropic(const ColorVariant &a)
//   {
//     _text = gl::texture::to_texture2d(a);
//   }

//   bool
//   scatter(const Ray &ray_in, HitRecord &rec, ScatterRecord &srec,
//           float uc = gl::rand_num(), // coin-flip sample
//           const gl::vec2 &u = {gl::rand_num(),
//                                gl::rand_num()}, // 2D microfacet sample
//           TransportMode mode = TransportMode::Radiance,
//           BxDFReflTransFlags flags = BxDFReflTransFlags::All) const override
//   {
//     // scatter the ray with lambertian reflection
//     // any direction scatter with equal probability
//     // scatter = Ray(rec.position, gl::sphere_random_vec());
//     // attenuation = _text->getTexelColor(rec.texCoords.u(), rec.texCoords.v());
//     // return true;
//     return false;
//   }

// private:
//   std::shared_ptr<Texture2D> _text;
// };
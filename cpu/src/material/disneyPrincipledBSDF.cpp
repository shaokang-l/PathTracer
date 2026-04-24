#include "material/disneyPrincipledBSDF.hpp"

bool DisneyPrincipledBSDF::scatter(const Ray &ray_in, HitRecord &rec, ScatterRecord &srec,
                                   float uc,          // coin-flip sample
                                   const gl::vec2 &u, // 2D microfacet sample
                                   TransportMode mode,
                                   BxDFReflTransFlags flags) const
{

    using namespace gl;

    // if inside, only glass is allowed
    if (rec.is_inside)
    {
        if (glass_mat == nullptr)
            return false;
        return glass_mat->scatter(ray_in, rec, srec, uc, u, mode, flags);
    }

    // 2. Handle non-reflection flags (only transmission allowed)
    if (!(flags & BxDFReflTransFlags::Reflection) && (!!(flags & BxDFReflTransFlags::Transmission)))
    {
        if (glass_mat && specular_transmission > 0.0f && glass_weight > 0.0f)
        {
            return glass_mat->scatter(ray_in, rec, srec, uc, u, mode, flags);
        }
        return false;
    }

    // Pointers to the actual sub-BSDFs corresponding to these weights
    std::vector<std::shared_ptr<Material>> lobes_to_sample = {
        diffuse_mat, metal_mat, glass_mat, clearcoat_mat};

    float pmf_choice;           // Will store the probability of choosing the selected lobe
    float uc_remapped_for_lobe; // The remapped random number for the chosen lobe
    int lobe_index = sampleDiscrete({diffuse_weight, metal_weight, glass_weight, clearcoat_weight}, uc, &pmf_choice, &uc_remapped_for_lobe);

    // No lobe chosen or lobe has zero probability
    if (lobe_index == -1 || pmf_choice <= 1e-6f)
        return false;

    std::shared_ptr<Material> chosen_lobe = lobes_to_sample[lobe_index];
    // non existing lobe
    if (!chosen_lobe)
        return false;

    gl::vec2 u_for_chosen_lobe = {uc_remapped_for_lobe, u[1]};
    bool scattered = chosen_lobe->scatter(ray_in, rec, srec, uc_remapped_for_lobe, u_for_chosen_lobe, mode, flags);

    if (!scattered)
        return false;
    if (srec.is_specular())
    { // extra guard
        srec.pdf_ptr = nullptr;
        return true;
    }
    if (srec.pdf_val <= 1e-6f)
        return false;

    vec3 wo_world = -ray_in.getDirection().normalize();
    vec3 wi_world = srec.sampled_ray.getDirection().normalize();
    // pdf_val (within single lobe), sampled_type, attenuation (within single lobe) are set by the chosen_lobe
    // we need to override the attenuation for the overall mixture
    srec.attenuation = this->f(wo_world, wi_world, rec, mode);
    srec.pdf_val = this->scatter_pdf(wo_world, wi_world, rec, mode, flags);

    // Check the overall mixture PDF
    if (srec.pdf_val <= 1e-7f)
        return false;

    // a fallback pdf for the overall mixture, set for compatibility with the base class
    // set to nullptr is safer, but at the cost of compatibility with the RWOWK Integrator
    srec.pdf_ptr = std::make_shared<CosinePDF>(rec.normal);
    return true;
};

float DisneyPrincipledBSDF::scatter_pdf(const gl::vec3 &wo_world, const gl::vec3 &wi_world,
                                        const HitRecord &rec,
                                        TransportMode mode,
                                        BxDFReflTransFlags flags) const
{
    using namespace gl;
    // 1. Inside, only glass is allowed
    if (rec.is_inside)
    {
        if (glass_mat == nullptr)
            return 0.f;
        return glass_mat->scatter_pdf(wo_world, wi_world, rec, mode, flags);
    }

    // 2. Handle non-reflection flags (only transmission allowed)
    if (!(flags & BxDFReflTransFlags::Reflection) && (!!(flags & BxDFReflTransFlags::Transmission)))
    {
        if (glass_mat && specular_transmission > 0.0f && glass_weight > 0.0f)
        {
            return glass_mat->scatter_pdf(wo_world, wi_world, rec, mode, flags);
        }
        return 0.f;
    }

    float pdf_val = 0.f;
    float sum_weights = diffuse_weight + metal_weight + glass_weight + clearcoat_weight;

    if (diffuse_mat)
        pdf_val += ((diffuse_weight / sum_weights) * diffuse_mat->scatter_pdf(wo_world, wi_world, rec, mode, flags));
    if (metal_mat)
        pdf_val += ((metal_weight / sum_weights) * metal_mat->scatter_pdf(wo_world, wi_world, rec, mode, flags));
    if (glass_mat)
        pdf_val += ((glass_weight / sum_weights) * glass_mat->scatter_pdf(wo_world, wi_world, rec, mode, flags));
    if (clearcoat_mat)
        pdf_val += ((clearcoat_weight / sum_weights) * clearcoat_mat->scatter_pdf(wo_world, wi_world, rec, mode, flags));

    return pdf_val;
};
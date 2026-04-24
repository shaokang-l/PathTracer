
#pragma once
#include "objectList.hpp"

LightList discover_emissive_objects_as_lights(const ObjectList &all_scene_objects)
{
    LightList discovered_lights;

    // Assuming ObjectList has a way to iterate its std::shared_ptr<Hittable> objects.
    // If all_scene_objects.objects is a public std::vector<std::shared_ptr<Hittable>>:
    for (const auto &hittable_in_scene : all_scene_objects.getLists())
    {
        if (!hittable_in_scene)
        {
            continue;
        }

        // 1. Get the material associated with this hittable object (decorators will forward this)
        std::shared_ptr<Material> material = hittable_in_scene->get_material();

        if (material && material->is_emitter())
        {

            // 2. Get the emission properties from the material
            std::shared_ptr<Texture2D> light_texture;
            float light_intensity;
            material->get_emission_properties(light_texture, light_intensity);

            if (light_intensity <= 0.f || !light_texture)
            {
                // Not a valid light source for sampling if intensity is zero or no texture defined
                continue;
            }

            // 3. Get the pointer to the *actual underlying geometric shape*, unwrapping decorators
            std::shared_ptr<Hittable> base_geometric_primitive = hittable_in_scene->get_underlying_shape();
            if (!base_geometric_primitive)
            {
                throw std::runtime_error("No underlying shape found for hittable in scene");
                return discovered_lights;
            }

            // 4. Try to dynamic_cast the base_geometric_primitive to known shape types
            //    that your Light constructors can handle.
            bool light_created = false;
            if (auto rect_x = std::dynamic_pointer_cast<AARectangle<Axis::X>>(base_geometric_primitive))
            {
                discovered_lights.addLight(std::make_shared<QuadLight>(rect_x, light_texture, light_intensity));
                light_created = true;
            }
            else if (auto rect_y = std::dynamic_pointer_cast<AARectangle<Axis::Y>>(base_geometric_primitive))
            {
                discovered_lights.addLight(std::make_shared<QuadLight>(rect_y, light_texture, light_intensity));
                light_created = true;
            }
            else if (auto rect_z = std::dynamic_pointer_cast<AARectangle<Axis::Z>>(base_geometric_primitive))
            {
                discovered_lights.addLight(std::make_shared<QuadLight>(rect_z, light_texture, light_intensity));
                light_created = true;
            }
            else if (auto sphere_shape = std::dynamic_pointer_cast<Sphere>(base_geometric_primitive))
            {
                discovered_lights.addLight(std::make_shared<SphereLight>(sphere_shape, light_texture, light_intensity));
                light_created = true;
            }
            // When you add TriangleMesh lights:
            // else if (auto mesh_shape = std::dynamic_pointer_cast<TriangleMesh>(base_geometric_primitive)) {
            //     discovered_lights.addLight(std::make_shared<MeshAreaLight>(mesh_shape, light_texture, light_intensity));
            //     light_created = true;
            // }

            if (!light_created)
            {
                std::cerr << "Warning: Emissive material found on a base shape (ObjType: "
                          << static_cast<int>(base_geometric_primitive->objtype)
                          << ") for which no specific Light constructor was matched via dynamic_cast." << std::endl;
            }
        }
    }
    return discovered_lights;
}
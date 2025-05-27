#pragma once
#include "scenes_hdri.hpp"
#include "medium/homogeneous.hpp"
#include "render/renderManager.hpp"

SceneInfo absorption_only_medium()
{
    using namespace std;
    using namespace gl;

    auto sphere_material = std::make_shared<DiffuseEmitter>(gl::vec3(0.4, 2.32, 3.2), 1.0f); // A warm emissive color
    auto my_sphere = std::make_shared<Sphere>(gl::vec3(0, 0, 0), 1.0f, sphere_material);

    SceneInfo scene;
    scene.objects.addObject(my_sphere);

    // In your scene setup:
    gl::vec3 sigma_a = gl::vec3(1.5f, 1.5f, 1.5f);
    gl::vec3 sigma_s = gl::vec3(0.0f, 0.0f, 0.0f);
    gl::vec3 Le_vol = gl::vec3(0.0f, 0.0f, 0.0f);
    float g_phase = 0.0f; // Not used by this integrator

    auto global_medium_ptr = std::make_shared<HomogeneousMedium>(sigma_a, sigma_s, 1.0f, Le_vol, 1.0f, g_phase);
    scene.global_medium = global_medium_ptr;
    scene.camera = make_shared<PerspectiveCamera>(
        gl::to_radian(45.f), (float)(scene._width) / (float)(scene._height), 10.f,
        1000.f, vec3(0, 1, 0), vec3(0, 0, 1).normalize(),
        vec3(0, 0, -3));
    scene.bg_color = vec3(0.f);
    return scene;
}

SceneInfo single_scatter_medium()
{
    using namespace std;
    using namespace gl;

    auto sphere_material = std::make_shared<DiffuseEmitter>(gl::vec3(0.4, 2.32, 3.2), 1.0f); // A warm emissive color
    auto my_sphere = std::make_shared<Sphere>(gl::vec3(0, 0, 0), 1.0f, sphere_material);

    SceneInfo scene;
    scene.objects.addObject(my_sphere);

    auto sphere_material2 = std::make_shared<DiffuseEmitter>(gl::vec3(24, 10, 24), 1.0f); // A warm emissive color
    auto my_sphere2 = std::make_shared<Sphere>(gl::vec3(-3, 0, -1.5), 1.0f, sphere_material2);
    scene.objects.addObject(my_sphere2);
    // In your scene setup:
    gl::vec3 sigma_a = gl::vec3(0.1f, 0.1f, 0.1f);
    gl::vec3 sigma_s = gl::vec3(0.7f, 0.7f, 0.7f);
    gl::vec3 Le_vol = gl::vec3(0.0f, 0.0f, 0.0f);
    float g_phase = 0.0f;

    auto global_medium_ptr = std::make_shared<HomogeneousMedium>(sigma_a, sigma_s, 1.0f, Le_vol, 1.0f, g_phase);
    scene.global_medium = global_medium_ptr;
    scene.camera = make_shared<PerspectiveCamera>(
        gl::to_radian(45.f), (float)(scene._width) / (float)(scene._height), 10.f,
        1000.f, vec3(0, 1, 0), vec3(0, 0, 1).normalize(),
        vec3(0, 0, -3));
    scene.bg_color = vec3(0.f);
    return scene;
}

SceneInfo multiple_scatter_medium()
{
    using namespace std;
    using namespace gl;

    SceneInfo scene;
    auto medium1 = std::make_shared<HomogeneousMedium>(gl::vec3(0.05), gl::vec3(0.25f), 1.0f, gl::vec3(0.0f, 0.0f, 0.0f), 1.0f, 0.0f);
    auto medium2 = std::make_shared<HomogeneousMedium>(gl::vec3(3.f), gl::vec3(0.5f), 1.0f, gl::vec3(0.0f, 0.0f, 0.0f), 1.0f, 0.0f);
    auto sphere_material = std::make_shared<DiffuseEmitter>(gl::vec3(0.4, 2.32, 3.2), 1.0f);

    auto mi_1_ptr = std::make_shared<MediumInterface>(medium2, medium1);
    auto my_sphere = std::make_shared<Sphere>(gl::vec3(-0.5, -0.5, 0), 0.75f, nullptr, mi_1_ptr);
    scene.objects.addObject(my_sphere);

    auto mi_2_ptr = std::make_shared<MediumInterface>(nullptr, medium1);
    auto my_sphere2 = std::make_shared<Sphere>(gl::vec3(1, 1, 2), 2.0f, sphere_material, mi_2_ptr);
    scene.objects.addObject(my_sphere2);

    scene.global_medium = medium1;
    scene.camera = make_shared<PerspectiveCamera>(
        gl::to_radian(45.f), (float)(scene._width) / (float)(scene._height), 10.f,
        1000.f, vec3(0, 1, 0), vec3(0, 0, 1).normalize(),
        vec3(0, 0, -4));
    scene.bg_color = vec3(0.f);
    return scene;
}

#pragma once
#include "scenes_disney.hpp"
#include "base/colors.hpp"
#include "material/commonMaterials.hpp"
#include "render/renderManager.hpp"

SceneInfo hdri_uniform_check()
{
    using namespace std;
    using namespace gl;

    SceneInfo scene = cornell_box_base();
    scene.objects.clear();

    shared_ptr<Hittable> sphere =
        make_shared<Sphere>(vec3(280, 250, 190), 150,
                            LAMBERTIAN_RED);

    scene.environment_light = make_shared<EnvironmentLight>(
        "../../assets/hdri/uniform.jpg", 1.0f);
    scene.objects.addObject(sphere);
    return scene;
}

SceneInfo hdri_directional_check()
{
    using namespace std;
    using namespace gl;

    SceneInfo scene = cornell_box_base();
    scene.objects.clear();

    shared_ptr<Hittable> sphere =
        make_shared<Sphere>(vec3(280, 250, 190), 150,
                            LAMBERTIAN_GRAY);

    scene.environment_light = make_shared<EnvironmentLight>(
        "../../assets/hdri/directional.png", 1.0f);
    scene.objects.addObject(sphere);
    return scene;
}

SceneInfo hdri_sunset_check()
{
    using namespace std;
    using namespace gl;

    SceneInfo scene = cornell_box_base();
    scene.objects.clear();

    std::shared_ptr<Hittable> mesh =
        loadOBJMesh("../../assets/bunny_high.obj", ROUGH_GLASS_MAT);
    // order matters!
    // ball = make_shared<Rotate<Axis::Y>>(ball, M_PI);
    mesh = make_shared<Rotate<Axis::Y>>(mesh, M_PI * 1.1f);
    mesh = make_shared<Scale>(mesh, 110.f);
    mesh = std::make_shared<Translate>(mesh, vec3(240, 140, 100));

    shared_ptr<Hittable> plane =
        make_shared<Sphere>(vec3(280, -1000, 190), 1000,
                            SILVER_MAT);

    scene.environment_light = make_shared<EnvironmentLight>(
        "../../assets/hdri/pink_sunset.jpg", 1.5f);
    scene.objects.addObject(mesh);
    scene.objects.addObject(plane);
    return scene;
}
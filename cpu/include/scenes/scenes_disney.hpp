#pragma once
#include "base/colors.hpp"
#include "material/commonMaterials.hpp"
#include "render/renderManager.hpp"
SceneInfo cornell_box_base()
{
    using namespace std;
    using namespace gl;

    SceneInfo scene;
    ObjectList objects;

    auto red_disneydiffuse = make_shared<DisneyDiffuse>(vec3(0.65, 0.05, 0.05), 0.5f, 0.5f);
    auto red_lambertian = make_shared<Lambertian>(vec3(0.65, 0.05, 0.05));
    auto white = make_shared<DisneyDiffuse>(vec3(0.73f), 0.1f, 1.f);
    auto green = make_shared<DisneyDiffuse>(vec3(0.12, 0.45, 0.15), 0.1f, 1.f);
    auto light = make_shared<DiffuseEmitter>(vec3(1.0f), 10);
    // std::array<gl::vec3, 4> vertices;
    // vertices[0] = {150, 554, 100};
    // vertices[1] = {400, 554, 100};
    // vertices[2] = {400, 554, 400};
    // vertices[3] = {150, 554, 400};

    objects.addObject(make_shared<FlipFace>(
        make_shared<AARectangle<Axis::Y>>(554, 150, 400, 100, 400, light)));
    objects.addObject(
        make_shared<AARectangle<Axis::X>>(555, 0, 555, 0, 555, green));
    objects.addObject(make_shared<AARectangle<Axis::X>>(0, 0, 555, 0, 555, red_disneydiffuse));
    objects.addObject(
        make_shared<AARectangle<Axis::Y>>(0, 0, 555, 0, 555, white));
    objects.addObject(
        make_shared<AARectangle<Axis::Y>>(555, 0, 555, 0, 555, white));
    objects.addObject(
        make_shared<AARectangle<Axis::Z>>(555, 0, 555, 0, 555, white));

    scene._height = 300;
    scene._width = 240;
    scene.spp_x = 2;
    scene.spp_y = 2;
    scene._gamma = 1.5;
    scene.camera = make_shared<PerspectiveCamera>(
        gl::to_radian(40.f), (float)(scene._width) / (float)(scene._height), 10.f,
        1000.f, vec3(0, 1, 0), vec3(0, 0, 1.f).normalize(),
        vec3(278.f, 278.f, -800.f));
    scene.objects = objects;
    scene.bg_color = vec3(0.f);
    return scene;
};
SceneInfo cornell_box_disneyDiffuse()
{
    using namespace std;
    using namespace gl;

    SceneInfo scene = cornell_box_base();
    shared_ptr<Hittable> sphere_00 =
        make_shared<Sphere>(vec3(80, 250, 190), 50,
                            make_shared<DisneyDiffuse>(vec3(0.65, 0.05, 0.05), 0.f, 0.0f));

    shared_ptr<Sphere> sphere_01 =
        make_shared<Sphere>(vec3(180, 250, 190), 50,
                            make_shared<DisneyDiffuse>(vec3(0.65, 0.05, 0.05), 0.2f, 0.2f));

    shared_ptr<Hittable> sphere_02 =
        make_shared<Sphere>(vec3(280, 250, 190), 50,
                            make_shared<DisneyDiffuse>(vec3(0.65, 0.05, 0.05), 0.4f, 0.4f));

    shared_ptr<Hittable> sphere_03 =
        make_shared<Sphere>(vec3(380, 250, 190), 50,
                            make_shared<DisneyDiffuse>(vec3(0.65, 0.05, 0.05), 0.6f, 0.6f));

    shared_ptr<Hittable> sphere_04 =
        make_shared<Sphere>(vec3(480, 250, 190), 50,
                            make_shared<DisneyDiffuse>(vec3(0.65, 0.05, 0.05), 0.8f, 0.8f));

    scene.objects.addObject(sphere_00);
    scene.objects.addObject(sphere_01);
    scene.objects.addObject(sphere_02);
    scene.objects.addObject(sphere_03);
    scene.objects.addObject(sphere_04);

    return scene;
}

SceneInfo cornell_box_disneyMetal()
{
    using namespace std;
    using namespace gl;

    SceneInfo scene = cornell_box_base();

    shared_ptr<Hittable> sphere_00 =
        make_shared<Sphere>(vec3(80, 250, 190), 50,
                            make_shared<DisneyMetal>(vec3(0.65, 0.05, 0.05), 0.f, 0.8f));

    shared_ptr<Sphere> sphere_01 =
        make_shared<Sphere>(vec3(180, 250, 190), 50,
                            make_shared<DisneyMetal>(vec3(0.65, 0.05, 0.05), 0.2f, 0.8f));

    shared_ptr<Hittable> sphere_02 =
        make_shared<Sphere>(vec3(280, 250, 190), 50,
                            make_shared<DisneyMetal>(vec3(0.65, 0.05, 0.05), 0.4f, 0.8f));

    shared_ptr<Hittable> sphere_03 =
        make_shared<Sphere>(vec3(380, 250, 190), 50,
                            make_shared<DisneyMetal>(vec3(0.65, 0.05, 0.05), 0.6f, 0.8f));

    shared_ptr<Hittable> sphere_04 =
        make_shared<Sphere>(vec3(480, 250, 190), 50,
                            make_shared<DisneyMetal>(vec3(0.65, 0.05, 0.05), 0.8f, 0.8f));

    scene.objects.addObject(sphere_00);
    scene.objects.addObject(sphere_01);
    scene.objects.addObject(sphere_02);
    scene.objects.addObject(sphere_03);
    scene.objects.addObject(sphere_04);

    return scene;
}

SceneInfo cornell_box_disneyClearcoat()
{
    using namespace std;
    using namespace gl;

    SceneInfo scene = cornell_box_base();

    shared_ptr<Hittable> sphere_00 =
        make_shared<Sphere>(vec3(80, 250, 190), 50,
                            make_shared<DisneyClearcoat>(0.2f));

    scene.objects.addObject(sphere_00);

    shared_ptr<Hittable> sphere_01 =
        make_shared<Sphere>(vec3(180, 250, 190), 50,
                            make_shared<DisneyClearcoat>(0.4f));

    scene.objects.addObject(sphere_01);

    shared_ptr<Hittable> sphere_02 =
        make_shared<Sphere>(vec3(280, 250, 190), 50,
                            make_shared<DisneyClearcoat>(0.6f));

    scene.objects.addObject(sphere_02);

    shared_ptr<Hittable> sphere_03 =
        make_shared<Sphere>(vec3(380, 250, 190), 50,
                            make_shared<DisneyClearcoat>(0.8f));

    scene.objects.addObject(sphere_03);

    shared_ptr<Hittable> sphere_04 =
        make_shared<Sphere>(vec3(480, 250, 190), 50,
                            make_shared<DisneyClearcoat>(1.0f));

    scene.objects.addObject(sphere_04);

    return scene;
}

SceneInfo cornell_box_disneySheen()
{
    using namespace std;
    using namespace gl;

    SceneInfo scene = cornell_box_base();

    // right to left
    shared_ptr<Hittable> sphere_00 =
        make_shared<Sphere>(vec3(80, 250, 190), 50,
                            make_shared<DisneySheen>(vec3(1.00, 1.00, 1.00), 0.2f));

    scene.objects.addObject(sphere_00);

    shared_ptr<Hittable> sphere_01 =
        make_shared<Sphere>(vec3(180, 250, 190), 50,
                            make_shared<DisneySheen>(vec3(1.00, 1.00, 1.00), 0.4f));

    scene.objects.addObject(sphere_01);

    shared_ptr<Hittable> sphere_02 =
        make_shared<Sphere>(vec3(280, 250, 190), 50,
                            make_shared<DisneySheen>(vec3(1.00, 1.00, 1.00), 0.6f));

    scene.objects.addObject(sphere_02);

    shared_ptr<Hittable> sphere_03 =
        make_shared<Sphere>(vec3(380, 250, 190), 50,
                            make_shared<DisneySheen>(vec3(1.00, 1.00, 1.00), 0.8f));

    scene.objects.addObject(sphere_03);

    shared_ptr<Hittable> sphere_04 =
        make_shared<Sphere>(vec3(480, 250, 190), 50,
                            make_shared<DisneySheen>(vec3(1.00, 1.00, 1.00), 1.0f));

    scene.objects.addObject(sphere_04);

    return scene;
}

SceneInfo cornell_box_disneyGlass()
{
    using namespace std;
    using namespace gl;

    SceneInfo scene = cornell_box_base();

    shared_ptr<Hittable> sphere_00 =
        make_shared<Sphere>(vec3(80, 250, 190), 50,
                            make_shared<DisneyGlass>(vec3(1.00, 1.00, 1.00), 0.2f, 0.8f, 1.5f));

    scene.objects.addObject(sphere_00);

    shared_ptr<Hittable> sphere_01 =
        make_shared<Sphere>(vec3(180, 250, 190), 50,
                            make_shared<DisneyGlass>(vec3(1.00, 1.00, 1.00), 0.4f, 0.8f, 1.5f));

    scene.objects.addObject(sphere_01);

    shared_ptr<Hittable> sphere_02 =
        make_shared<Sphere>(vec3(280, 250, 190), 50,
                            make_shared<DisneyGlass>(vec3(1.00, 1.00, 1.00), 0.6f, 0.8f, 1.5f));

    scene.objects.addObject(sphere_02);

    shared_ptr<Hittable> sphere_03 =
        make_shared<Sphere>(vec3(380, 250, 190), 50,
                            make_shared<DisneyGlass>(vec3(1.00, 1.00, 1.00), 0.8f, 0.8f, 1.5f));

    scene.objects.addObject(sphere_03);

    shared_ptr<Hittable> sphere_04 =
        make_shared<Sphere>(vec3(480, 250, 190), 50,
                            make_shared<DisneyGlass>(vec3(1.00, 1.00, 1.00), 1.0f, 0.8f, 1.5f));

    scene.objects.addObject(sphere_04);

    return scene;
}

SceneInfo cornell_box_mfDielectric()
{
    using namespace std;
    using namespace gl;

    SceneInfo scene = cornell_box_base();

    shared_ptr<Hittable> sphere_00 =
        make_shared<Sphere>(vec3(80, 250, 190), 50,
                            make_shared<MFDielectric>(1.5f, 0.001f, 0.001f));

    scene.objects.addObject(sphere_00);

    shared_ptr<Hittable> sphere_01 =
        make_shared<Sphere>(vec3(180, 250, 190), 50,
                            make_shared<MFDielectric>(1.5f, 0.1f, 0.1f));

    scene.objects.addObject(sphere_01);

    shared_ptr<Hittable> sphere_02 =
        make_shared<Sphere>(vec3(280, 250, 190), 50,
                            make_shared<MFDielectric>(1.5f, 0.3f, 0.3f));

    scene.objects.addObject(sphere_02);

    shared_ptr<Hittable> sphere_03 =
        make_shared<Sphere>(vec3(380, 250, 190), 50,
                            make_shared<MFDielectric>(1.5f, 0.5f, 0.5f));

    scene.objects.addObject(sphere_03);

    shared_ptr<Hittable> sphere_04 =
        make_shared<Sphere>(vec3(480, 250, 190), 50,
                            make_shared<MFDielectric>(1.5f, 0.8f, 0.8f));

    scene.objects.addObject(sphere_04);

    return scene;
}

SceneInfo cornell_box_DisneyPrincipledBSDF()
{
    using namespace std;
    using namespace gl;

    SceneInfo scene = cornell_box_base();

    shared_ptr<Hittable> sphere_00 =
        make_shared<Sphere>(vec3(90, 130, 190), 50,
                            DisneyBSDF::DisneyMaterial_Shell0);

    scene.objects.addObject(sphere_00);

    shared_ptr<Hittable> sphere_01 =
        make_shared<Sphere>(vec3(210, 130, 190), 50,
                            DisneyBSDF::DisneyMaterial_Shell1);

    scene.objects.addObject(sphere_01);

    shared_ptr<Hittable> sphere_02 =
        make_shared<Sphere>(vec3(330, 130, 190), 50,
                            DisneyBSDF::DisneyMaterial_Shell2);

    scene.objects.addObject(sphere_02);

    shared_ptr<Hittable> sphere_03 =
        make_shared<Sphere>(vec3(450, 130, 190), 50,
                            DisneyBSDF::DisneyMaterial_Shell3);

    scene.objects.addObject(sphere_03);

    shared_ptr<Hittable> sphere_04 =
        make_shared<Sphere>(vec3(90, 250, 190), 50,
                            DisneyBSDF::DisneyMaterial_Shell4);

    scene.objects.addObject(sphere_04);

    shared_ptr<Hittable> sphere_05 =
        make_shared<Sphere>(vec3(210, 250, 190), 50,
                            DisneyBSDF::DisneyMaterial_Shell5);

    scene.objects.addObject(sphere_05);

    shared_ptr<Hittable> sphere_06 =
        make_shared<Sphere>(vec3(330, 250, 190), 50,
                            DisneyBSDF::DisneyMaterial_Shell6);

    scene.objects.addObject(sphere_06);

    shared_ptr<Hittable> sphere_07 =
        make_shared<Sphere>(vec3(450, 250, 190), 50,
                            DisneyBSDF::DisneyMaterial_Shell7);

    scene.objects.addObject(sphere_07);

    shared_ptr<Hittable> sphere_08 =
        make_shared<Sphere>(vec3(90, 370, 190), 50,
                            DisneyBSDF::DisneyMaterial_Shell8);

    scene.objects.addObject(sphere_08);

    shared_ptr<Hittable> sphere_09 =
        make_shared<Sphere>(vec3(210, 370, 190), 50,
                            DisneyBSDF::DisneyMaterial_Shell9);

    scene.objects.addObject(sphere_09);

    shared_ptr<Hittable> sphere_10 =
        make_shared<Sphere>(vec3(330, 370, 190), 50,
                            DisneyBSDF::DisneyMaterial_Shell10);

    scene.objects.addObject(sphere_10);

    shared_ptr<Hittable> sphere_11 =
        make_shared<Sphere>(vec3(450, 370, 190), 50,
                            DisneyBSDF::DisneyMaterial_Shell11);

    scene.objects.addObject(sphere_11);

    return scene;
}
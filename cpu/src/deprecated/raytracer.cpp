#include "./unit_test.hpp"
#include "./utils/timeit.hpp"
#include <stdio.h>

// #define IMAGE_WIDTH 1500
// #define IMAGE_HEIGHT 1500
// #define SPP_X 10
// #define SPP_Y 10
// #define MAX_DEPTH 3
// #define GAMMA 1.0f
// #define GL_SIMD
// #define F_STOP 1000.f
// #define BVH

// using uchar = unsigned char;
// const char *output_file = "../scene.png";
// const char *input_file = "../Scenes/test1.ascii";

// SceneIO *scene = nullptr;
// std::unique_ptr<PerspectiveCamera> camera = nullptr;
// LightList lights;
// ObjectList prims;
// BVHNode bvh;

// static void loadScene(const char *name) {
//   /* load the scene into the SceneIO data structure using given parsing code */
//   scene = readScene(name);
//   camera.reset(new PerspectiveCamera(
//       scene->camera, (float)IMAGE_WIDTH / (float)IMAGE_HEIGHT, F_STOP));
//   lights = LightList(_get_lights_from_io(scene->lights));
//   prims = ObjectList(_get_primitives_from_io(scene->objects));

//   //--------------------------------custom scene------
//   // auto checker_mat = std::make_shared<CheckerReflector>(20.f);
//   // auto sphere = std::make_shared<Sphere>(gl::vec3(100.f,-1200.f,-900.f),
//   //                                          1000.f, checker_mat);
//   // prims.addObject(sphere);

//   // for (int i = 0; i < 100; i++) {
//   //   auto random_mat = std::make_shared<CustomMaterial>();
//   //   random_mat->emissive_color = gl::sphere_random_vec(0.3f);
//   //   random_mat->diff_color = gl::sphere_random_vec(0.2f);
//   //   random_mat->spec_color = gl::sphere_random_vec(0.2f);
//   //   random_mat->ambient_color = gl::sphere_random_vec(0.2f);
//   //   random_mat->ktran = gl::C_rand(0.5f, 1.0f);
//   //   random_mat->shininess = gl::C_rand(0.0f, 0.5f);

//   //   auto sphere = std::make_shared<Sphere>(gl::sphere_random_vec(9.0f),
//   //                                          gl::C_rand(0.2f, 0.8f),
//   //                                          random_mat);
//   //   prims.addObject(sphere);
//   // }

//   bvh = BVHNode(prims);
//   return;
// }

// /* just a place holder, feel free to edit */
// void render(void) {
//   using namespace gl;
//   FrameBuffer fb(IMAGE_WIDTH, IMAGE_HEIGHT, 3, SPP_X, SPP_Y);
//   auto offsets = fb.getOffsets();
//   uint counter = 0;

// #pragma omp parallel for num_threads(omp_get_num_procs() + 1)
//   {
//     for (int i = 0; i < IMAGE_WIDTH; i++) {
//       std::cout << "Now scanning " << (float(counter) / IMAGE_WIDTH) * 100.f
//                 << " %" << std::endl;

//       for (int j = 0; j < IMAGE_HEIGHT; j++) {
//         auto color = vec3(0.0);
//         for (int k = 0; k < fb.getSampleCount(); k++) {
//           auto sample_color = vec3(0.0);
//           vec2 uv = (vec2(i, j) + offsets[k]) / vec2(IMAGE_WIDTH, IMAGE_HEIGHT);
//           Ray ray = camera->generateRay(uv);
// #ifdef BVH
//           color += getRayColor(ray, prims, bvh, MAX_DEPTH, lights);
// #else
//           color += getRayColor(ray, prims, MAX_DEPTH, lights);
// #endif
//         }

//         // implicit barrier at this section
//         {
//           color /= fb.getSampleCount();
//           fb.setPixelColor(j, i, color);
//         }
//       }

//       counter++;
//     }
//   }

//   // fb.gaussianBlur(3, 1.0f);
//   fb.writeToFile(output_file, GAMMA);
//   return;
// }

// int main(int argc, char *argv[]) {
//   std::string file_path = input_file;
//   auto load = make_decorator(loadScene); 
//   load(file_path.c_str());

//   auto timeit = make_decorator(render);
//   timeit();

//   /* cleanup */
//   if (scene != nullptr) {
//     deleteScene(scene);
//   }

//   return 0;
// }

#include "../../include/test_header.hpp"
#include "../utils/bvh.hpp"
// #define FRAMEBUFFER_TEST
// #define LOAD_TEST
// #define BASIC_SPHERE
// #define BASIC_POLYSETS
// #define OBJECT_LIST
// #define RECURSIVE_TEST
// #define SIMD_TEST
// #define SPHERE_TEST
// #define TEXTURE_TEST
// #define CHECKER_TEST
// #define SHADER_TEST
// #define BVH_TEST
// #define PDF_TEST
// #define DIFFUSE_TEST
// #define HAIRPDF_TEST

int main() {

#ifdef HAIRPDF_TEST
  using namespace gl;

  vec3 basis(0.f, 0.f, 1.f);
  vec3 wo_world(0.f, 1.f, 0.f);
  wo_world.normalized();
  HairMarschner hair(vec3(0.419f, 0.697f, 1.37f), 0.f, 1.55f, 0.7f, 0.5f,
                     to_radian(2.f));
  // pick some fixed wo and hair parameters...
  HairPDF pdf(hair, basis, wo_world);

  // normalization test
  {
    const int N = 200000;
    double sum = 0;
    for (int i = 0; i < N; ++i) {
      // sample a direction uniformly on the sphere:
      float u1 = rand_num(), u2 = rand_num();
      vec3 wi =
          uniformSampleSphere(u1, u2); // your usual uniform-sphere sampler
      float p = pdf.at(wi);
      if (p < 0) {
        std::cout << "Negative PDF value: " << p << std::endl;
        throw std::runtime_error("Negative PDF value");
      }
      sum += p;
    }
    // the average of p(w) over a uniform sphere times 4π should be ≈ 1
    double avg = sum / double(N);
    double estimate = avg * (4.0 * M_PI);
    std::cout << "∫ p(w)dω ≈ " << estimate << std::endl;
  }

#endif

#ifdef DIFFUSE_TEST
  using namespace gl;

  SceneInfo scene = night();
  scene.renderWithInfo("../output.png");

#endif

#ifdef SHADER_TEST
  using namespace gl;
  std::chrono::time_point<std::chrono::system_clock> start, end;
  std::chrono::duration<double> duration;
  start = std::chrono::system_clock::now();

  std::string name = "../Scenes_2/test2.ascii";
  auto scene = readScene(name.c_str());
  auto camera = PerspectiveCamera(scene->camera, 1, F_STOP);
  LightList lights(_get_lights_from_io(scene->lights));
  ObjectList prims(_get_primitives_from_io(scene->objects));

  prims.addObject(std::make_shared<Sphere>(
      vec3(1.02, -0.36, -1.13), 1.0f, std::make_shared<CheckerMaterial>(10.f)));

  prims.addObject(std::make_shared<Sphere>(
      vec3(1.5, -0.36, -1.13), 1.0f,
      std::make_shared<LambertianMaterial>("../unit_test/earth.png")));

  BVHNode bvh(prims);

  uint width = 400, height = 400;
  FrameBuffer fb(width, height, 3, 4, 4);
  auto offsets = fb.getOffsets();
  uint counter = 0;

#pragma omp parallel for
  {
    for (int i = 0; i < width; i++) {
      std::cout << "Now scanning " << (float(counter) / width) * 100.f << " %"
                << std::endl;

      for (int j = 0; j < height; j++) {
        auto color = vec3(0.0);
        for (int k = 0; k < fb.getSampleCount(); k++) {
          auto sample_color = vec3(0.0);
          vec2 uv = (vec2(i, j) + offsets[k]) / vec2(width, height);
          Ray ray = camera.generateRay(uv.u(), uv.v());
          color += getRayColor(ray, prims, bvh, 4u, lights);
        }

// implicit barrier at this section
#pragma omp critical
        {
          color /= fb.getSampleCount();
          fb.setPixelColor(j, i, color);
        }
      }

      counter++;
    }
  }

  // fb.gaussianBlur(3, 1.0f);
  fb.writeToFile("../custom_test.png", 1.0f);

  end = std::chrono::system_clock::now();
  duration = end - start;
  std::cout << duration.count() << " seconds" << std::endl;
#endif

#ifdef CHECKER_TEST
  CheckerTexture checker(200.f);
  NoiseTexture noise(10.f, 2);
  FrameBuffer fb(1000, 1000, 3);
  for (int i = 0; i < 1000; i++) {
    for (int j = 0; j < 1000; j++) {
      fb.setPixelColor(j, i, noise.getTexelColor(i / 1000.f, j / 1000.f));
    }
  }
  fb.writeToFile("../checker_test.png");
#endif

#ifdef TEXTURE_TEST
  ImageTexture tex("../results/scene1.png");
  FrameBuffer fb(1000, 1000, 3);
  for (int i = 0; i < 1000; i++) {
    for (int j = 0; j < 1000; j++) {
      fb.setPixelColor(j, i, tex.getTexelColor(i / 1000.f, j / 1000.f));
    }
  }
  fb.writeToFile("../texture_test.png");
#endif

#ifdef SPHERE_TEST
  auto hit_record = std::make_shared<HitRecord>();
  gl::vec3 center(0.f);
  hit_record->position = gl::vec3(0.f, 0.f, -1.f);
  auto p = (hit_record->position - center).normalize();

  auto phi = atan2(p.z(), p.x());
  auto theta = asin(p.y());
  hit_record->texCoords =
      gl::vec2(1 - (phi + M_PI) / (2 * M_PI), (theta + M_PI / 2) / M_PI);

  std::cout << hit_record->texCoords << std::endl;
#endif

#ifdef BVH_TEST
  // this test code is used to debug the minimalist BVH function
  using namespace gl;
  std::chrono::time_point<std::chrono::system_clock> start, end;
  std::chrono::duration<double> duration;
  start = std::chrono::system_clock::now();

  std::string name = "./Scenes/test1.ascii";
  auto scene = readScene(name.c_str());
  auto camera = PerspectiveCamera(scene->camera, 1, F_STOP);
  LightList lights(_get_lights_from_io(scene->lights));
  ObjectList prims(_get_primitives_from_io(scene->objects));
  BVHNode bvh(prims);

  uint width = 400, height = 400;
  FrameBuffer fb(width, height, 3, 2, 2);
  auto offsets = fb.getOffsets();
  uint counter = 0;

#pragma omp parallel for
  {
    for (int i = 0; i < 400; i++) {
      std::cout << "Now scanning " << (float(counter) / width) * 100.f << " %"
                << std::endl;

      for (int j = 0; j < 400; j++) {
        auto color = vec3(0.0);
        for (int k = 0; k < fb.getSampleCount(); k++) {
          auto sample_color = vec3(0.0);
          vec2 uv = (vec2(i, j) + offsets[k]) / vec2(width, height);
          Ray ray = camera.generateRay(uv.u(), uv.v());
          color += getRayColor(ray, prims, bvh, 5u, lights);
        }

// implicit barrier at this section
#pragma omp critical
        {
          color /= fb.getSampleCount();
          fb.setPixelColor(j, i, color);
        }
      }

      counter++;
    }
  }

  fb.writeToFile("../custom_test.png", 1.0f);

  end = std::chrono::system_clock::now();
  duration = end - start;
  std::cout << duration.count() << " seconds" << std::endl;
#endif

#ifdef SIMD_TEST
  using namespace gl;
  vec4 a(0.0f, 0.0f, 1.0f, 1.0f);
  vec4 b(0.0, 0.0f, 1.0f, 1.0f);
  std::cout << a.normalize() << std::endl;
#endif

#ifdef RECURSIVE_TEST
  using namespace gl;
  std::chrono::time_point<std::chrono::system_clock> start, end;
  std::chrono::duration<double> duration;
  start = std::chrono::system_clock::now();

  std::string name = "../Scenes/test3.ascii";
  auto scene = readScene(name.c_str());
  auto camera = PerspectiveCamera(scene->camera, 1);
  LightList lights(_get_lights_from_io(scene->lights));
  ObjectList prims(_get_primitives_from_io(scene->objects));

  uint width = 1500, height = 1500;
  FrameBuffer fb(width, height, 3, 4, 4);
  auto offsets = fb.getOffsets();
  uint counter = 0;

#pragma omp parallel for num_threads(omp_get_num_procs() + 1)
  {
    for (int i = 0; i < width; i++) {
      std::cout << "Now scanning " << (float(counter) / width) * 100.f << " %"
                << std::endl;

      for (int j = 0; j < height; j++) {
        auto color = vec3(0.0);
        for (int k = 0; k < fb.getSampleCount(); k++) {
          auto sample_color = vec3(0.0);
          vec2 uv = (vec2(i, j) + offsets[k]) / vec2(width, height);
          Ray ray = camera.generateRay(uv);
          color += getRayColor(ray, prims, 5u, lights);
        }

// implicit barrier at this section
#pragma omp critical
        {
          color /= fb.getSampleCount();
          fb.setPixelColor(j, i, color);
        }
      }

      counter++;
    }
  }

  // fb.gaussianBlur(3, 1.0f);
  fb.writeToFile("../test2.png", 1.0f);

  end = std::chrono::system_clock::now();
  duration = end - start;
  std::cout << duration.count() << " seconds" << std::endl;
#endif

#ifdef OBJECT_LIST
  using namespace gl;
  std::chrono::time_point<std::chrono::system_clock> start, end;
  std::chrono::duration<double> duration;
  start = std::chrono::system_clock::now();

  std::string name = "../Scenes/test1.ascii";
  auto scene = readScene(name.c_str());
  auto camera = PerspectiveCamera(scene->camera, 1.33);
  auto [plights, dlights] = _get_lights_from_io(scene->lights);
  ObjectList prims(_get_primitives_from_io(scene->objects));

  uint width = 400, height = 300;
  FrameBuffer fb(width, height, 3, 5, 5);
  auto offsets = fb.getOffsets();

#pragma omp parallel for
  {
    for (int i = 0; i < width; i++) {
      std::cout << "Now scanning" << i << " of " << width << std::endl;

      for (int j = 0; j < height; j++) {
        auto color = vec3(0.0);
        for (int k = 0; k < fb.getSampleCount(); k++) {
          auto sample_color = vec3(0.0);
          vec2 uv = (vec2(i, j) + offsets[k]) / vec2(width, height);
          Ray ray = camera.generateRay(uv);
          auto is_hit = prims.hit(ray);
          if (is_hit != nullptr) {
            auto base_color = is_hit->material->diff_color;
            color += base_color;
          }
        }

// implicit barrier at this section
#pragma omp critical
        {
          color /= fb.getSampleCount();
          fb.setPixelColor(j, i, color);
        }
      }
    }
  }

  fb.writeToFile("../test.png");

  end = std::chrono::system_clock::now();
  duration = end - start;
  std::cout << duration.count() << " seconds" << std::endl;
#endif

#ifdef LOAD_TEST
  std::string name = "../Scenes/test1.ascii";
  auto scene = readScene(name);
  auto camera = PerspectiveCamera(scene->camera);
  auto [plights, dlights] = _get_lights_from_io(scene->lights);
  auto [spheres, polysets] = _get_primitives_from_io(scene->objects);
#endif

#ifdef BASIC_POLYSETS
  std::string name = "../Scenes/test1.ascii";
  auto scene = readScene(name.c_str());
  auto camera = PerspectiveCamera(scene->camera, 1.33);
  // auto camera = PerspectiveCamera(gl::to_radian(45), 1.33, 1, gl::vec3(0, 1,
  // 0), gl::vec3(0, 0, -1), gl::vec3(0, 0, 0));
  auto [plights, dlights] = _get_lights_from_io(scene->lights);
  auto [spheres, polysets] = _get_primitives_from_io(scene->objects);

  uint width = 400, height = 300;
  FrameBuffer fb(width, height, 3);

  for (int i = 0; i < width; i++) {
    for (int j = 0; j < height; j++) {
      Ray ray = camera.generateRay(i / float(width), j / float(height));
      for (const auto &polyset : polysets) {
        for (const auto &tri : polyset.triangles) {
          if (tri.intersect(ray)) {
            fb.setPixelColor(j, i, gl::vec3(1, 0, 0));
          }
        }
      }

      for (const auto &sphere : spheres) {
        if (sphere.intersect(ray)) {
          fb.setPixelColor(j, i, gl::vec3(0, 1, 0));
        }
      }
    }
  }

  fb.writeToFile("../test.png");
#endif

#ifdef BASIC_SPHERE
  std::string name = "../Scenes/test1.ascii";
  auto scene = readScene(name.c_str());
  auto camera = PerspectiveCamera(scene->camera, 1.33);
  // auto camera = PerspectiveCamera(gl::to_radian(45), 1.33, 1, gl::vec3(0, 1,
  // 0), gl::vec3(0, 0, -1), gl::vec3(0, 0, 0));
  auto [plights, dlights] = _get_lights_from_io(scene->lights);
  auto [spheres, polysets] = _get_primitives_from_io(scene->objects);
  uint width = 400, height = 300;
  FrameBuffer fb(width, height, 3);

  for (int i = 0; i < width; i++) {
    for (int j = 0; j < height; j++) {
      Ray ray = camera.generateRay(i / float(width), j / float(height));
      for (const auto &sphere : spheres) {
        if (sphere.intersect(ray)) {
          fb.setPixelColor(j, i, gl::vec3(1.0f, 0.0f, 0.0f));
        }
      }
    }
  }

  fb.writeToFile("../test.png");
#endif

#ifdef FRAMEBUFFER_TEST
  FrameBuffer fb(2000, 1500, 3);
  for (int i = 0; i < 2000; i++) {
    for (int j = 0; j < 1500; j++) {
      fb.setPixelColor(0 + 100, i, gl::vec3(1.0f, 0.0f, 0.0f));
    }
  }
  fb.writeToFile("../test.png");
#endif

#ifdef MATH_TEST

  std::cout << gl::fastExp(1.0f) << std::endl;
  std::cout << gl::fastExp(2.0f) << std::endl;
  std::cout << gl::fastExp(3.0f) << std::endl;
#endif
}
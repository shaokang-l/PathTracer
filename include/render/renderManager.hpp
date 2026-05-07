#pragma once
#include "base/camera.hpp"
#include "base/framebuffer.hpp"
#include "medium/medium.hpp"
#include "base/objectList.hpp"
#include "base/primitive.hpp"
#include "config.hpp"
#include "material/material.hpp"
#include "mesh_io/fbxLoader.hpp"
#include "mesh_io/meshLoader.hpp"
#include "primitives/box.hpp"
#include "primitives/bvh.hpp"
#include "primitives/curve.hpp"
#include "utils/objectTransform.hpp"
#include "utils/timeit.hpp"
#include "base/lightDiscovery.hpp"
#include "light/envLight.hpp"

#ifdef USE_ANALYTICAL_ILLUMIN
#include "integrator/analytical_illumin.hpp"
#elif defined USE_MAXDEPTH_MIS
#include "integrator/maxdepth_mis.hpp"
#elif defined USE_MAXDEPTH_NAIVE
#include "integrator/maxdepth_naive.hpp"
#elif defined USE_MAXDEPTH_NEE
#include "integrator/maxdepth_nee.hpp"
#elif defined USE_MAXDEPTH_RESERVOIR
#include "integrator/maxdepth_reservoir_di.hpp"
#elif defined USE_ROULETTE_NAIVE
#include "integrator/roulette_naive.hpp"
#elif defined USE_MAXDEPTH_VOLUME
#include "integrator/maxdepth_volume.hpp"
#endif

struct SceneInfo
{
  std::shared_ptr<PerspectiveCamera> camera = nullptr;
  std::shared_ptr<BVHNode> bvh = nullptr;
  ObjectList objects;
  std::shared_ptr<EnvironmentLight> environment_light = nullptr;
  gl::vec3 bg_color = gl::vec3(0.7, 0.8, 1.0);
  bool use_bvh = true;
  uint _width = 800;
  uint _height = 800;
  uint spp_x = 2;
  uint spp_y = 2;
  float _gamma = 1.0f;
  std::shared_ptr<Medium> global_medium = nullptr;

  SceneInfo() = default;
  void render(const std::string &out_path = "./output.png",
              bool show_progress = true)
  {

    using namespace gl;
    using namespace std;

    if (camera == nullptr)
    {
      std::cout << "Camera is not initialized!" << std::endl;
      return;
    }

    if (objects.getLists().size() == 0)
    {
      std::cout << "No objects in the scene!" << std::endl;
      return;
    }

#ifdef OVERRIDE_LOCAL_RENDER_VAL
    _width = WIDTH;
    _height = HEIGHT;
    spp_x = SPP_X;
    spp_y = SPP_Y;
    _gamma = GAMMA;
    bg_color = BG_COLOR;
    use_bvh = useBVH;
#endif

    if (use_bvh)
      bvh = make_shared<BVHNode>(objects);

    FrameBuffer fb(_width, _height, spp_x, spp_y);
    auto offsets = fb.getOffsets();
    uint counter = 0;

    // light discovery
    LightList discovered_lights = discover_emissive_objects_as_lights(objects);
    // compatibility with old RTOWK integrator, which requires object to have sample method
    ObjectList light_objects = ObjectList(discovered_lights);

    if (environment_light)
    {
      discovered_lights.addLight(environment_light);
    }


    {
      #pragma omp parallel for schedule(dynamic, 1)
      for (int y = 0; y < _height; y++)
      {

        if (show_progress){
          int localCounter;
          #pragma omp atomic read
          localCounter = counter;

          if (localCounter % 16 == 0) {
              #pragma omp critical
              {
                  std::cout << "Progress: "
                            << 100.0f * localCounter / _height
                            << "%\n";
              }
        }
        }

        for (int x = 0; x < _width; x++)
        {
          auto color = vec3(0.0);

          // per sample
          for (int k = 0; k < fb.getSampleCount(); k++)
          {

            // per sample, reset sampler
            halton_sampler.startSample();
            auto sample_color = vec3(0.0);
            vec2 uv = (vec2(x, y) + offsets[k]) / vec2(_width, _height);
            Ray ray = camera->generateRay(uv.u(), uv.v());

#ifdef USE_ANALYTICAL_ILLUMIN
            color += getRayColor(ray, objects, bg_color, discovered_lights, bvh);
#elif defined USE_MAXDEPTH_NEE
            color +=
                getRayColor(ray, objects, bg_color, discovered_lights, MAX_RAY_DEPTH, bvh);
#elif defined USE_ROULETTE_NAIVE
            color += getRayColor(ray, objects, light_objects, bg_color,
                                 MAX_RAY_DEPTH, bvh);
#elif defined USE_MAXDEPTH_RESERVOIR
            color +=
                getRayColor(ray, objects, bg_color, discovered_lights, MAX_RAY_DEPTH, bvh);
#elif defined USE_MAXDEPTH_NAIVE
            color += getRayColor(ray, objects, light_objects, bg_color,
                                 MAX_RAY_DEPTH, bvh);
#elif defined USE_MAXDEPTH_MIS
            color +=
                getRayColor(ray, objects, bg_color, discovered_lights, MAX_RAY_DEPTH, bvh);
#elif defined USE_MAXDEPTH_VOLUME
            color += getRayColor(ray, objects, bg_color, discovered_lights, MAX_RAY_DEPTH, bvh, global_medium);
#else
            std::cout << "No method selected!" << std::endl;
            std::runtime_error(
                "No method selected! Please define a method in the config.hpp");
#endif
          }

          // average color
          {
            color /= fb.getSampleCount();
            fb.setPixelColor(x, y, color);
          }
        }

        #pragma omp atomic update
        counter++;
      }
    }

    fb.writeToFile(out_path, GAMMA);
  };

  void renderWithInfo(const std::string &out_path = "./output.png",
                      bool time_it = true, bool show_hitcount = true,
                      bool show_progress = true)
  {
    std::chrono::time_point<std::chrono::system_clock> start, end;
    std::chrono::duration<double> duration;
    start = std::chrono::system_clock::now();
    render(out_path, show_progress);
    end = std::chrono::system_clock::now();
    duration = end - start;

    if (time_it)
      std::cout << "Rendering time: " << duration.count() << " seconds"
                << std::endl;
    if (show_hitcount)
      std::cout << "Total hit count: " << gl::hit_count << std::endl;
  }
};
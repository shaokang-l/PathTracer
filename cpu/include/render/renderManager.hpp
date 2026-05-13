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
#include "pt/scene/render_settings.h"
#include <atomic>
#include <unordered_map>

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
  uint max_depth = MAX_RAY_DEPTH;
  float _gamma = 1.0f;
  pt::DebugViewKind debug_view = pt::DebugViewKind::Beauty;
  std::shared_ptr<Medium> global_medium = nullptr;
  bool use_config_defaults = true;

  SceneInfo() = default;

  static gl::vec3 debugHashColor(int id)
  {
    uint32_t x = uint32_t(id + 1) * 747796405u + 2891336453u;
    x = ((x >> ((x >> 28u) + 4u)) ^ x) * 277803737u;
    x = (x >> 22u) ^ x;
    return gl::vec3(float((x >> 0u) & 255u) / 255.f,
                    float((x >> 8u) & 255u) / 255.f,
                    float((x >> 16u) & 255u) / 255.f);
  }

  static gl::vec3 debugAlbedoProbe(const Ray &ray, HitRecord &hit_record)
  {
    if (!hit_record.material)
      return gl::vec3(0.f);

    if (hit_record.material->is_emitter())
      return hit_record.material->emit(ray, hit_record);

    const gl::vec3 wo = -ray.getDirection().normalize();
    const gl::vec3 wi = hit_record.normal.normalize();
    return hit_record.material->f(wo, wi, hit_record) * float(M_PI);
  }

  static gl::vec3 debugVisibilityProbe(const HitRecord &hit_record,
                                       const ObjectList &objects,
                                       const std::shared_ptr<BVHNode> &bvh,
                                       const LightList &lights,
                                       pt::DebugViewKind view)
  {
    if (lights.size_excluding_env() == 0)
      return gl::vec3(0.f);

    int light_id = 0;
    std::shared_ptr<Light> light = nullptr;
    for (uint64_t i = 0; i < lights.size(); ++i)
    {
      if (lights.get(static_cast<int>(i))->type != LightType::ENVIRONMENT_LIGHT)
      {
        light_id = static_cast<int>(i);
        light = lights.get(light_id);
        break;
      }
    }
    if (!light)
      return gl::vec3(0.f);

    const gl::vec3 p_light = light->get_sample(0.5f, 0.5f);
    const gl::vec3 to_light = p_light - hit_record.position;
    const float dist = to_light.length();
    if (dist <= 1e-4f)
      return gl::vec3(0.f);

    Ray shadow_ray(hit_record.position, to_light * (1.0f / dist));
    HitRecord shadow_hit;
    const bool occluded = bvh
      ? bvh->intersect(shadow_ray, shadow_hit, 1e-3f, dist - 1e-3f)
      : objects.intersect(shadow_ray, shadow_hit, 1e-3f, dist - 1e-3f);
    if (occluded)
      return gl::vec3(0.f);

    if (view == pt::DebugViewKind::LightId)
      return debugHashColor(light_id);
    return gl::vec3(1.f);
  }

  gl::vec3 shadeDebugView(const Ray &ray,
                          const std::unordered_map<const Material *, int> &material_ids,
                          const LightList &lights) const
  {
    HitRecord hit_record;
    const bool hit = bvh
      ? bvh->intersect(ray, hit_record, 1e-3f, 1e5f)
      : objects.intersect(ray, hit_record, 1e-3f, 1e5f);

    if (!hit)
      return bg_color;

    if (debug_view == pt::DebugViewKind::Normal)
      return 0.5f * (hit_record.normal.normalize() + gl::vec3(1.f));

    if (debug_view == pt::DebugViewKind::Albedo)
      return debugAlbedoProbe(ray, hit_record);

    if (debug_view == pt::DebugViewKind::MaterialId)
    {
      const auto it = material_ids.find(hit_record.material.get());
      return debugHashColor(it == material_ids.end() ? 0 : it->second);
    }

    if (debug_view == pt::DebugViewKind::Visibility ||
        debug_view == pt::DebugViewKind::LightId)
      return debugVisibilityProbe(hit_record, objects, bvh, lights, debug_view);

    return gl::vec3(0.f);
  }

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

    if (use_config_defaults)
    {
#ifdef OVERRIDE_LOCAL_RENDER_VAL
      _width = WIDTH;
      _height = HEIGHT;
      spp_x = SPP_X;
      spp_y = SPP_Y;
      max_depth = MAX_RAY_DEPTH;
      _gamma = GAMMA;
      bg_color = BG_COLOR;
      use_bvh = useBVH;
#endif
    }

    if (use_bvh)
      bvh = make_shared<BVHNode>(objects);

    FrameBuffer fb(_width, _height, spp_x, spp_y);
    auto offsets = fb.getOffsets();
    std::atomic<uint> counter{0};

    // light discovery
    LightList discovered_lights = discover_emissive_objects_as_lights(objects);
    // compatibility with old RTOWK integrator, which requires object to have sample method
    ObjectList light_objects = ObjectList(discovered_lights);

    if (environment_light)
    {
      discovered_lights.addLight(environment_light);
    }

    std::unordered_map<const Material *, int> material_ids;
    int next_material_id = 0;
    for (const auto &object : objects.getLists())
    {
      const std::shared_ptr<Material> material = object->get_material();
      if (material && material_ids.find(material.get()) == material_ids.end())
      {
        material_ids.emplace(material.get(), next_material_id++);
      }
    }

    #pragma omp parallel for schedule(dynamic, 1)
    for (int y = 0; y < _height; y++)
    {

      if (show_progress)
      {
        uint localCounter = counter.load(std::memory_order_relaxed);

        if (localCounter % 16 == 0)
        {
          #pragma omp critical
          {
            std::cout << "Progress: " << 100.0f * localCounter / _height
                      << "%\n";
          }
        }
      }

      for (int x = 0; x < _width; x++)
      {
        auto color = vec3(0.0);

        if (debug_view != pt::DebugViewKind::Beauty)
        {
          vec2 uv = (vec2(x, y) + vec2(0.5f)) / vec2(_width, _height);
          Ray ray = camera->generateRay(uv.u(), uv.v());
          fb.setPixelColor(y, x, shadeDebugView(ray, material_ids, discovered_lights));
          continue;
        }

        // per sample
        for (int k = 0; k < fb.getSampleCount(); k++)
        {

            const uint64_t sample_id =
                (uint64_t(y) * uint64_t(_width) + uint64_t(x)) *
                    uint64_t(fb.getSampleCount()) +
                uint64_t(k);

            // Per pixel/sample deterministic state. This keeps CPU reference
            // renders stable regardless of OpenMP scheduling.
            gl::seed_rand(sample_id + 1u);
            halton_sampler.startSample(static_cast<std::uint32_t>(sample_id + 1u));
            auto sample_color = vec3(0.0);
            vec2 uv = (vec2(x, y) + offsets[k]) / vec2(_width, _height);
            Ray ray = camera->generateRay(uv.u(), uv.v());

#ifdef USE_ANALYTICAL_ILLUMIN
            color += getRayColor(ray, objects, bg_color, discovered_lights, bvh);
#elif defined USE_MAXDEPTH_NEE
            color +=
                getRayColor(ray, objects, bg_color, discovered_lights, max_depth, bvh);
#elif defined USE_ROULETTE_NAIVE
            color += getRayColor(ray, objects, light_objects, bg_color,
                                 max_depth, bvh);
#elif defined USE_MAXDEPTH_RESERVOIR
            color +=
                getRayColor(ray, objects, bg_color, discovered_lights, max_depth, bvh);
#elif defined USE_MAXDEPTH_NAIVE
            color += getRayColor(ray, objects, light_objects, bg_color,
                                 max_depth, bvh);
#elif defined USE_MAXDEPTH_MIS
            color +=
                getRayColor(ray, objects, bg_color, discovered_lights, max_depth, bvh);
#elif defined USE_MAXDEPTH_VOLUME
            color += getRayColor(ray, objects, bg_color, discovered_lights, max_depth, bvh, global_medium);
#else
            std::cout << "No method selected!" << std::endl;
            std::runtime_error(
                "No method selected! Please define a method in the config.hpp");
#endif
        }

        // average color
        {
          color /= fb.getSampleCount();
          fb.setPixelColor(y, x, color);
        }
      }

      counter.fetch_add(1, std::memory_order_relaxed);
    }

    fb.writeToFile(out_path, _gamma);
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
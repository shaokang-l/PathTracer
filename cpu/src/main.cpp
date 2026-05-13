#include "header.hpp"
#include "pt/scene/render_settings.h"
#include "scenes/scene_xml.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <sstream>
#include <string>
#include <string_view>

namespace {

int parseIntArg(int argc, char **argv, std::string_view name, int fallback)
{
  for (int i = 1; i + 1 < argc; ++i)
  {
    if (std::string_view(argv[i]) == name)
      return std::atoi(argv[i + 1]);
  }
  return fallback;
}

std::string_view parseStringArg(int argc, char **argv, std::string_view name,
                                std::string_view fallback)
{
  for (int i = 1; i + 1 < argc; ++i)
  {
    if (std::string_view(argv[i]) == name)
      return argv[i + 1];
  }
  return fallback;
}

float parseFloatArg(int argc, char **argv, std::string_view name, float fallback)
{
  for (int i = 1; i + 1 < argc; ++i)
  {
    if (std::string_view(argv[i]) == name)
      return std::strtof(argv[i + 1], nullptr);
  }
  return fallback;
}

bool hasArg(int argc, char **argv, std::string_view name)
{
  for (int i = 1; i < argc; ++i)
  {
    if (std::string_view(argv[i]) == name)
      return true;
  }
  return false;
}

gl::vec3 parseVec3Arg(int argc, char **argv, std::string_view name,
                      const gl::vec3 &fallback)
{
  for (int i = 1; i + 1 < argc; ++i)
  {
    if (std::string_view(argv[i]) == name)
    {
      std::string value(argv[i + 1]);
      std::replace(value.begin(), value.end(), ',', ' ');
      std::istringstream in(value);
      float x = fallback.x();
      float y = fallback.y();
      float z = fallback.z();
      in >> x;
      if (!(in >> y))
        y = x;
      if (!(in >> z))
        z = y;
      return gl::vec3(x, y, z);
    }
  }
  return fallback;
}

} // namespace

extern int rejects;
int main(int argc, char **argv)
{

#if defined(_OPENMP)
  std::cout << "✅ OpenMP is enabled (version " << _OPENMP << ")\n"
            << "   Max threads: " << omp_get_max_threads() << "\n"
            << "   Using threads: " << NUM_THREADS << "\n";

  omp_set_num_threads(NUM_THREADS);
#else
  std::cout << "❌ OpenMP is NOT enabled\n";
#endif

  SceneInfo scene;
  bool scene_loaded = false;
  bool load_only = false;
  for (int i = 1; i < argc; ++i)
  {
    if (std::string_view(argv[i]) == "--load-only")
    {
      load_only = true;
    }
  }
  for (int i = 1; i + 1 < argc; ++i)
  {
    if (std::string_view(argv[i]) == "--scene-xml")
    {
      scene = loadMitsubaXmlScene(argv[i + 1]);
      scene_loaded = true;
      break;
    }
  }

  if (!scene_loaded)
  {
    // night_time();
    // scene = cornell_box();

    // scene = cornell_box_disneyDiffuse();
    // scene = cornell_box_disneyMetal();
    // scene = cornell_box_disneyGlass();
    // scene = cornell_box_DisneyPrincipledBSDF();
    // scene = cornell_box_mfDielectric();
    // scene = cornell_box_disneySheen();
    // scene = cornell_box_disneyClearcoat();
    // scene = checkpoint_diffuse();
    scene = cornell_box_modified();
    // scene = custom_mesh();
    // scene = hdri_directional_check();
    // scene = hdri_sunset_check();

    // scene = absorption_only_medium();
    // scene = single_scatter_medium();
    // scene = multiple_scatter_medium();

    // #ifdef HAS_FBX_SDK
    //   scene = fbx_mesh();
    // #else
    //   scene = cornell_box();
    // #endif
    // two_lights();
    // scene = simple_light();
    // scene = diffuse_diffuse();

    // night();
    // checkpoint_2();
    // checkpoint_3();
    // scene = VeachMIS();
    // scene = night();

    // scene = debug_curve();
  }

  pt::RenderSettings defaults;
  defaults.width = static_cast<int>(scene._width);
  defaults.height = static_cast<int>(scene._height);
  defaults.spp = static_cast<int>(scene.spp_x * scene.spp_y);
  defaults.maxDepth = static_cast<int>(scene.max_depth);
  defaults.gamma = scene._gamma;
  defaults.background = pt::Vec3f(scene.bg_color.x(), scene.bg_color.y(), scene.bg_color.z());
  const pt::RenderSettings settings = pt::parseRenderSettings(argc, argv, defaults);

  scene._width = static_cast<uint>(settings.width);
  scene._height = static_cast<uint>(settings.height);
  if (settings.spp > 0)
  {
    const int spp_side = std::max(1, static_cast<int>(std::round(std::sqrt(float(settings.spp)))));
    scene.spp_x = static_cast<uint>(spp_side);
    scene.spp_y = static_cast<uint>(spp_side);
  }
  scene.max_depth = static_cast<uint>(settings.maxDepth);
  scene._gamma = settings.gamma;
  scene.debug_view = settings.debugView;
  scene.bg_color = gl::vec3(settings.background.x, settings.background.y, settings.background.z);
  if (settings.hasCameraOverride)
  {
    const gl::vec3 origin =
        gl::vec3(settings.cameraOrigin.x, settings.cameraOrigin.y, settings.cameraOrigin.z);
    const gl::vec3 target =
        gl::vec3(settings.cameraTarget.x, settings.cameraTarget.y, settings.cameraTarget.z);
    const gl::vec3 up =
        gl::vec3(settings.cameraUp.x, settings.cameraUp.y, settings.cameraUp.z);
    scene.camera = std::make_shared<PerspectiveCamera>(
        gl::to_radian(settings.fov),
        float(scene._width) / float(scene._height),
        10.f,
        1000.f,
        up,
        (target - origin).normalize(),
        origin);
  }

  if (load_only)
  {
    std::cout << "[cpu] load-only: scene loaded successfully" << std::endl;
    return 0;
  }

  // Output to the project folder
  const std::string output_path =
      std::string(parseStringArg(argc, argv, "--output", "./output.png"));
  scene.renderWithInfo(output_path, true, true, false);
  std::cout << rejects << std::endl;
  return 0;
};
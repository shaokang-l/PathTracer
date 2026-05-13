#include "header.hpp"
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

  const int width = parseIntArg(argc, argv, "--width", -1);
  const int height = parseIntArg(argc, argv, "--height", -1);
  const int spp = parseIntArg(argc, argv, "--spp", -1);
  const int max_depth = parseIntArg(argc, argv, "--max-depth", -1);
  const float gamma = parseFloatArg(argc, argv, "--gamma", -1.f);
  scene.bg_color = parseVec3Arg(argc, argv, "--background", scene.bg_color);
  if (width > 0)
    scene._width = static_cast<uint>(width);
  if (height > 0)
    scene._height = static_cast<uint>(height);
  if (spp > 0)
  {
    const int spp_side = std::max(1, static_cast<int>(std::round(std::sqrt(float(spp)))));
    scene.spp_x = static_cast<uint>(spp_side);
    scene.spp_y = static_cast<uint>(spp_side);
  }
  if (max_depth > 0)
    scene.max_depth = static_cast<uint>(max_depth);
  if (gamma > 0.f)
    scene._gamma = gamma;
  if (hasArg(argc, argv, "--camera-origin") || hasArg(argc, argv, "--camera-target"))
  {
    const gl::vec3 origin =
        parseVec3Arg(argc, argv, "--camera-origin", gl::vec3(0.f, 0.f, -18.f));
    const gl::vec3 target =
        parseVec3Arg(argc, argv, "--camera-target", gl::vec3(0.f));
    const gl::vec3 up =
        parseVec3Arg(argc, argv, "--camera-up", gl::vec3(0.f, 1.f, 0.f));
    const float fov = parseFloatArg(argc, argv, "--fov", 45.f);
    scene.camera = std::make_shared<PerspectiveCamera>(
        gl::to_radian(fov),
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
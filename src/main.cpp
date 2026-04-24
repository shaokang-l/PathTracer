#include "header.hpp"
extern int rejects;
int main()
{

#if defined(_OPENMP)
  std::cout << "✅ OpenMP is enabled (version " << _OPENMP << ")\n"
            << "   Max threads: " << omp_get_max_threads() << "\n"
            << "   Using threads: " << NUM_THREADS << "\n";

  omp_set_num_threads(NUM_THREADS);
#else
  std::cout << "❌ OpenMP is NOT enabled\n";
#endif

  // night_time();
  // SceneInfo scene = cornell_box();

  // SceneInfo scene = cornell_box_disneyDiffuse();
  // SceneInfo scene = cornell_box_disneyMetal();
  // SceneInfo scene = cornell_box_disneyGlass();
  SceneInfo scene = cornell_box_DisneyPrincipledBSDF();
  // SceneInfo scene = cornell_box_mfDielectric();
  // SceneInfo scene = cornell_box_disneySheen();
  // SceneInfo scene = cornell_box_disneyClearcoat();
  //  SceneInfo scene = checkpoint_diffuse();
  // SceneInfo scene = cornell_box_modified();
  // SceneInfo scene = custom_mesh();
  // SceneInfo scene = hdri_directional_check();
  // SceneInfo scene = hdri_sunset_check();

  // SceneInfo scene = absorption_only_medium();
  // SceneInfo scene = single_scatter_medium();
  // SceneInfo scene = multiple_scatter_medium();

  //   SceneInfo scene;
  // #ifdef HAS_FBX_SDK
  //   scene = fbx_mesh();
  // #else
  //   scene = cornell_box();
  // #endif
  // two_lights();
  // SceneInfo scene = simple_light();
  // SceneInfo scene = diffuse_diffuse();

  // night();
  //  checkpoint_2();
  // checkpoint_3();
  // SceneInfo scene = VeachMIS();
  //  SceneInfo scene = night();

  // SceneInfo scene = debug_curve();

  // Output to the project folder
  scene.renderWithInfo("./output.png");
  std::cout << rejects << std::endl;
  return 0;
};
#include "Scene.h"

namespace mypt {

  void Scene::computeBounds()
  {
    bounds = owl::box3f();
    for (const auto &m : meshes)
      for (const auto &v : m.vertices)
        bounds.extend(v);
  }

  static void pushBox(TriangleMesh &mesh,
                      const owl::vec3f &lo,
                      const owl::vec3f &hi)
  {
    using owl::vec3f;
    using owl::vec3i;
    const int base = int(mesh.vertices.size());
    const vec3f v[8] = {
      { lo.x, lo.y, lo.z }, { hi.x, lo.y, lo.z },
      { lo.x, hi.y, lo.z }, { hi.x, hi.y, lo.z },
      { lo.x, lo.y, hi.z }, { hi.x, lo.y, hi.z },
      { lo.x, hi.y, hi.z }, { hi.x, hi.y, hi.z },
    };
    for (int i = 0; i < 8; ++i) mesh.vertices.push_back(v[i]);
    const vec3i tris[12] = {
      { 0, 1, 3 }, { 0, 3, 2 },
      { 4, 6, 7 }, { 4, 7, 5 },
      { 0, 2, 6 }, { 0, 6, 4 },
      { 1, 5, 7 }, { 1, 7, 3 },
      { 2, 3, 7 }, { 2, 7, 6 },
      { 0, 4, 5 }, { 0, 5, 1 },
    };
    for (int i = 0; i < 12; ++i)
      mesh.indices.push_back(vec3i(tris[i].x + base,
                                   tris[i].y + base,
                                   tris[i].z + base));
  }

  Scene Scene::makeTestScene()
  {
    using owl::vec3f;
    Scene s;

    MaterialGPU lambertian_0;
    lambertian_0.kind = MATERIAL_LAMBERTIAN;
    lambertian_0.albedo = vec3f(0.75f, 0.75f, 0.75f);
    s.materials.push_back(lambertian_0);

    MaterialGPU lambertian_1;
    lambertian_1.kind = MATERIAL_LAMBERTIAN;
    lambertian_1.albedo = vec3f(0.65f, 0.05f, 0.05f);
    s.materials.push_back(lambertian_1);

    MaterialGPU Lambertian_2;
    Lambertian_2.kind = MATERIAL_LAMBERTIAN;
    Lambertian_2.albedo = vec3f(0.12f, 0.45f, 0.15f);
    s.materials.push_back(Lambertian_2);
    
    MaterialGPU Lambertian_3;
    Lambertian_3.kind = MATERIAL_LAMBERTIAN;
    Lambertian_3.albedo = vec3f(0.8f, 0.7f, 0.3f);
    s.materials.push_back(Lambertian_3);

    MaterialGPU mirror;
    mirror.kind = MATERIAL_MIRROR;
    mirror.albedo = vec3f(0.95f);
    s.materials.push_back(mirror);
    
    MaterialGPU emissive;
    emissive.kind = MATERIAL_EMISSIVE;
    emissive.emission = vec3f(18.f, 18.f, 16.f);
    s.materials.push_back(emissive);

    TriangleMesh floor;
    floor.materialId = 0;
    pushBox(floor, vec3f(-5.f, -0.1f, -5.f), vec3f(5.f, 0.f, 5.f));
    s.meshes.push_back(std::move(floor));

    TriangleMesh leftWall;
    leftWall.materialId = 1;
    pushBox(leftWall, vec3f(-5.0f, 0.f, -5.f), vec3f(-4.9f, 5.f, 5.f));
    s.meshes.push_back(std::move(leftWall));

    TriangleMesh rightWall;
    rightWall.materialId = 2;
    pushBox(rightWall, vec3f(4.9f, 0.f, -5.f), vec3f(5.0f, 5.f, 5.f));
    s.meshes.push_back(std::move(rightWall));

    TriangleMesh diffuseBox;
    diffuseBox.materialId = 3;
    pushBox(diffuseBox, vec3f(-2.0f, 0.f, -1.5f), vec3f(-0.5f, 2.5f, 0.0f));
    s.meshes.push_back(std::move(diffuseBox));

    TriangleMesh mirrorBox;
    mirrorBox.materialId = 4;
    pushBox(mirrorBox, vec3f(0.5f, 0.f, -0.5f), vec3f(2.0f, 1.5f, 1.0f));
    s.meshes.push_back(std::move(mirrorBox));

    TriangleMesh light;
    light.materialId = 5;
    pushBox(light, vec3f(-1.5f, 4.9f, -1.5f), vec3f(1.5f, 5.0f, 1.5f));
    s.meshes.push_back(std::move(light));

    s.computeBounds();
    return s;
  }

} // namespace mypt

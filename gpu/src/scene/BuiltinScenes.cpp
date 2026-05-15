#include "scene/Scene.h"
#include "scene/SceneBuilders.h"

#include <cmath>
#include <utility>
#include <vector>

namespace mypt {

  using namespace scene_detail;

  Scene Scene::makeDisneyCornellScene()
  {
    using owl::vec3f;
    Scene s;

    constexpr float wallT = 0.035f;
    constexpr float L = 5.55f;
    constexpr float kR = 50.f / 50.f;

    const vec3f whiteDiffuse(0.73f);
    const vec3f redDiffuse(0.65f, 0.05f, 0.05f);
    const vec3f greenDiffuse(0.12f, 0.45f, 0.15f);

    s.materials.push_back(makeLambertian(whiteDiffuse));
    s.materials.push_back(makeLambertian(redDiffuse));
    s.materials.push_back(makeLambertian(greenDiffuse));
    s.materials.push_back(makeEmissive(vec3f(10.f)));

    s.materials.push_back(makeDisneyPrincipled(
      vec3f(0.82f, 0.67f, 0.16f), 0.f, 0.f, 0.5f, 0.5f, 0.5f, 0.5f, 0.f,
      0.f, 0.5f, 0.f, 0.5f, 1.5f));
    s.materials.push_back(makeDisneyPrincipled(
      vec3f(0.25f, 0.83f, 0.36f), 0.f, 0.8f, 0.5f, 0.5f, 0.5f, 0.5f, 0.f,
      0.f, 0.5f, 1.f, 0.5f, 1.5f));
    s.materials.push_back(makeDisneyPrincipled(
      vec3f(0.75f, 0.83f, 0.46f), 0.f, 0.1f, 0.5f, 0.5f, 0.5f, 0.5f, 0.f,
      1.f, 0.5f, 0.5f, 0.5f, 1.5f));
    s.materials.push_back(makeDisneyPrincipled(
      vec3f(0.75f, 0.83f, 0.46f), 0.5f, 0.1f, 0.f, 1.f, 0.5f, 0.5f, 0.f,
      0.f, 0.5f, 0.f, 0.5f, 1.5f));
    s.materials.push_back(makeDisneyPrincipled(
      vec3f(0.5f, 0.23f, 0.84f), 0.f, 0.5f, 1.f, 1.f, 0.5f, 0.5f, 0.1f,
      0.5f, 0.5f, 0.5f, 0.5f, 1.5f));
    s.materials.push_back(makeDisneyPrincipled(
      vec3f(0.5f, 0.9f, 0.84f), 1.f, 0.f, 1.f, 1.f, 0.1f, 0.5f, 0.1f,
      0.5f, 0.5f, 0.5f, 0.5f, 1.5f));
    s.materials.push_back(makeDisneyPrincipled(
      vec3f(0.9f, 0.9f, 0.84f), 0.f, 1.f, 1.f, 1.f, 0.1f, 0.5f, 0.1f,
      0.5f, 0.5f, 0.5f, 0.5f, 1.5f));
    s.materials.push_back(makeDisneyPrincipled(
      vec3f(0.2f, 0.2f, 0.3f), 0.5f, 0.5f, 0.5f, 0.5f, 0.2f, 0.5f, 0.1f,
      0.5f, 0.5f, 0.5f, 0.5f, 1.5f));
    s.materials.push_back(makeDisneyPrincipled(
      vec3f(0.5f, 0.6f, 0.7f), 0.2f, 0.8f, 0.2f, 0.7f, 0.1f, 0.f, 0.3f,
      0.5f, 0.5f, 0.5f, 0.5f, 1.5f));
    s.materials.push_back(makeDisneyPrincipled(
      vec3f(0.9f, 0.2f, 0.3f), 0.f, 0.f, 0.8f, 0.3f, 0.1f, 0.f, 0.f,
      1.f, 0.5f, 1.f, 0.5f, 1.5f));
    s.materials.push_back(makeDisneyPrincipled(
      vec3f(0.3f, 0.5f, 0.3f), 1.f, 0.9f, 0.8f, 0.3f, 0.2f, 0.f, 0.3f,
      1.f, 0.5f, 1.f, 0.5f, 1.5f));
    s.materials.push_back(makeDisneyPrincipled(
      vec3f(0.1f, 0.1f, 0.3f), 0.5f, 0.5f, 0.5f, 0.3f, 0.9f, 0.f, 0.1f,
      0.f, 0.5f, 0.f, 0.5f, 1.5f));

    auto pushWallBox = [&](float x0, float x1, float y0, float y1,
                           float z0, float z1, int32_t matId) {
      TriangleMesh w;
      w.materialId = matId;
      pushBox(w, vec3f(x0, y0, z0), vec3f(x1, y1, z1));
      s.meshes.push_back(std::move(w));
    };

    pushWallBox(-L - wallT, -L + wallT, -L, L, -L, L, 1);
    pushWallBox(L - wallT, L + wallT, -L, L, -L, L, 2);
    pushWallBox(-L, L, -L - wallT, -L + wallT, -L, L, 0);
    pushWallBox(-L, L, L - wallT, L + wallT, -L, L, 0);
    pushWallBox(-L, L, -L, L, L - wallT, L + wallT, 0);

    const float xi0 = (150.f - 277.5f) / 50.f;
    const float xi1 = (400.f - 277.5f) / 50.f;
    const float zi0 = (100.f - 277.5f) / 50.f;
    const float zi1 = (400.f - 277.5f) / 50.f;
    const float yCeil = (555.f - 277.5f) / 50.f;

    pushWallBox(-L, L, yCeil - wallT, yCeil + wallT, zi1, L, 0);
    pushWallBox(-L, L, yCeil - wallT, yCeil + wallT, -L, zi0, 0);
    pushWallBox(-L, xi0, yCeil - wallT, yCeil + wallT, zi0, zi1, 0);
    pushWallBox(xi1, L, yCeil - wallT, yCeil + wallT, zi0, zi1, 0);

    TriangleMesh lightMesh;
    lightMesh.materialId = 3;
    const float yLight = (554.f - 277.5f) / 50.f;
    pushBox(lightMesh,
            vec3f(xi0, yLight - wallT, zi0),
            vec3f(xi1, yLight + wallT, zi1));
    s.meshes.push_back(std::move(lightMesh));

    const float sphereCx[] = {90.f, 210.f, 330.f, 450.f};
    const float sphereCy[] = {130.f, 250.f, 370.f};
    const int32_t shellMatBase = 4;
    int shellIdx = 0;
    for (float cy : sphereCy) {
      for (float cx : sphereCx) {
        TriangleMesh sp;
        sp.materialId = shellMatBase + shellIdx;
        appendUvSphere(sp, cornellCpuToGpu(cx, cy, 190.f), kR, 28, 28);
        s.meshes.push_back(std::move(sp));
        ++shellIdx;
      }
    }

    LightGPU quadLight;
    quadLight.kind = LIGHT_QUAD;
    quadLight.emission = s.materials[3].emission;
    const vec3f v0 = cornellCpuToGpu(150.f, 554.f, 100.f);
    quadLight.v0 = v0;
    quadLight.edgeU = cornellCpuToGpu(400.f, 554.f, 100.f) - v0;
    quadLight.edgeV = cornellCpuToGpu(150.f, 554.f, 400.f) - v0;
    quadLight.normal = vec3f(0.f, -1.f, 0.f);
    quadLight.area = owl::length(quadLight.edgeU) * owl::length(quadLight.edgeV);
    s.lights.push_back(quadLight);

    s.computeBounds();
    return s;
  }

  Scene Scene::makeDisneyPrincipledGalleryScene()
  {
    using owl::vec3f;
    Scene s;

    s.materials.push_back(makeLambertian(vec3f(1.f)));
    s.materials.push_back(makeEmissive(vec3f(10.f)));
    s.materials.push_back(makeConductor(vec3f(0.14f, 0.43f, 1.38f),
                                        vec3f(4.54f, 2.455f, 1.914f),
                                        0.0001f));
    s.materials.push_back(makeConductor(vec3f(0.14f, 0.43f, 1.38f),
                                        vec3f(4.54f, 2.455f, 1.914f),
                                        0.1f));

    std::vector<MaterialGPU> disneyMats = disneyPrincipledPresetsFromCommonMaterials();
    const int32_t kDisneyMatBase = int32_t(s.materials.size());
    for (auto &m : disneyMats)
      s.materials.push_back(std::move(m));

    TriangleMesh floor;
    floor.materialId = 0;
    pushBox(floor, vec3f(-40.f, -0.55f, -40.f), vec3f(40.f, -0.5f, 40.f));
    s.meshes.push_back(std::move(floor));

    const ObjData bunnyObj = loadObjData(std::string(PATHTRACER_ASSET_DIR) + "/bunny.obj");
    const float bunnyScale = 24.f;
    const float bunnyRotY = float(M_PI_2);
    const float baseY = -0.5f;

    constexpr int kNBunnies = 17;
    const float dx = 2.65f;
    const float dz = 3.6f;
    const float xRow0 = -10.6f;
    const float z0 = -2.f;
    for (int i = 0; i < kNBunnies; ++i) {
      const int row = (i < 9) ? 0 : 1;
      const int j = (i < 9) ? i : (i - 9);
      const float x = (row == 0) ? (xRow0 + float(j) * dx)
                                 : (xRow0 + 0.5f * dx + float(j) * dx);
      const float z = (row == 0) ? z0 : (z0 + dz);
      s.meshes.push_back(makeMeshFromObjData(bunnyObj,
                                             kDisneyMatBase + i,
                                             bunnyScale,
                                             bunnyRotY,
                                             vec3f(x, baseY, z)));
    }

    TriangleMesh light;
    light.materialId = 1;
    pushBox(light, vec3f(-2.f, 13.95f, -3.f), vec3f(6.f, 14.0f, 5.f));
    s.meshes.push_back(std::move(light));

    TriangleMesh goldBackdrop;
    goldBackdrop.materialId = 2;
    pushBox(goldBackdrop, vec3f(-12.05f, -40.f, -40.f), vec3f(-12.f, 40.f, 40.f));
    s.meshes.push_back(std::move(goldBackdrop));

    TriangleMesh roughGoldBackdrop;
    roughGoldBackdrop.materialId = 3;
    pushBox(roughGoldBackdrop, vec3f(-40.f, -40.f, -8.05f), vec3f(40.f, 40.f, -8.f));
    s.meshes.push_back(std::move(roughGoldBackdrop));

    LightGPU quadLight;
    quadLight.kind = LIGHT_QUAD;
    quadLight.emission = s.materials[1].emission;
    quadLight.v0 = vec3f(-2.f, 13.95f, -3.f);
    quadLight.edgeU = vec3f(8.f, 0.f, 0.f);
    quadLight.edgeV = vec3f(0.f, 0.f, 8.f);
    quadLight.normal = vec3f(0.f, -1.f, 0.f);
    quadLight.area = 64.f;
    s.lights.push_back(quadLight);

    s.computeBounds();
    return s;
  }

  Scene Scene::makeDisneyMaterialLabScene()
  {
    using owl::vec3f;
    Scene s;

    s.materials.push_back(makeLambertian(vec3f(0.025f, 0.025f, 0.03f)));
    s.materials.push_back(makeLambertian(vec3f(0.18f, 0.18f, 0.19f)));
    s.materials.push_back(makeEmissive(vec3f(18.f, 16.f, 13.f)));
    s.materials.push_back(makeEmissive(vec3f(3.0f, 3.8f, 5.2f)));
    s.materials.push_back(makeEmissive(vec3f(8.f, 8.f, 10.f)));
    s.materials.push_back(makeDielectric(1.5f, 0.08f));
    s.materials.push_back(makeConductor(vec3f(0.04f, 0.06f, 0.04f),
                                        vec3f(4.8f, 3.586f, 2.657f),
                                        0.12f));

    std::vector<MaterialGPU> disneyMats = disneyPrincipledPresetsFromCommonMaterials();
    for (MaterialGPU &m : disneyMats) {
      boostDisneyTransmissionForLab(m);
    }
    const int32_t disneyMatBase = int32_t(s.materials.size());
    for (auto &m : disneyMats) {
      s.materials.push_back(std::move(m));
    }

    TriangleMesh floor;
    floor.materialId = 0;
    pushBox(floor, vec3f(-16.f, -0.58f, -9.f), vec3f(16.f, -0.5f, 12.f));
    s.meshes.push_back(std::move(floor));

    TriangleMesh dielectricBackdrop;
    dielectricBackdrop.materialId = 5;
    pushBox(dielectricBackdrop, vec3f(-16.f, -0.6f, 7.2f), vec3f(0.f, 8.5f, 7.32f));
    s.meshes.push_back(std::move(dielectricBackdrop));

    TriangleMesh metalBackdrop;
    metalBackdrop.materialId = 6;
    pushBox(metalBackdrop, vec3f(0.f, -0.6f, 7.2f), vec3f(16.f, 8.5f, 7.32f));
    s.meshes.push_back(std::move(metalBackdrop));

    TriangleMesh sideWall;
    sideWall.materialId = 0;
    pushBox(sideWall, vec3f(-16.2f, -0.6f, -9.f), vec3f(-16.f, 7.f, 7.3f));
    s.meshes.push_back(std::move(sideWall));

    const ObjData bunnyObj = loadObjData(std::string(PATHTRACER_ASSET_DIR) + "/bunny.obj");
    const int n = int(disneyMats.size());
    const float radius = 10.5f;
    const float angleMin = -1.15f;
    const float angleMax = 1.15f;
    const float baseY = -0.18f;

    for (int i = 0; i < n; ++i) {
      const float t = n > 1 ? float(i) / float(n - 1) : 0.f;
      const float a = angleMin + (angleMax - angleMin) * t;
      const float x = std::sin(a) * radius;
      const float z = -2.6f + (1.f - std::cos(a)) * 4.2f;

      TriangleMesh pedestal;
      pedestal.materialId = 1;
      const float pedestalHalf = (i % 2 == 0) ? 0.62f : 0.54f;
      const float pedestalTop = (i % 3 == 0) ? -0.08f : -0.16f;
      pushBox(pedestal,
              vec3f(x - pedestalHalf, -0.5f, z - pedestalHalf),
              vec3f(x + pedestalHalf, pedestalTop, z + pedestalHalf));
      s.meshes.push_back(std::move(pedestal));

      s.meshes.push_back(makeMeshFromObjData(bunnyObj,
                                             disneyMatBase + i,
                                             17.5f,
                                             float(M_PI_2) - a * 0.35f,
                                             vec3f(x, baseY, z)));
    }

    TriangleMesh keyLight;
    keyLight.materialId = 2;
    pushBox(keyLight, vec3f(-9.5f, 4.2f, -4.5f), vec3f(-9.2f, 9.0f, 2.5f));
    s.meshes.push_back(std::move(keyLight));

    TriangleMesh fillLight;
    fillLight.materialId = 3;
    pushBox(fillLight, vec3f(9.0f, 2.8f, -2.5f), vec3f(9.25f, 6.8f, 4.2f));
    s.meshes.push_back(std::move(fillLight));

    TriangleMesh rimLight;
    rimLight.materialId = 4;
    pushBox(rimLight, vec3f(-4.2f, 6.7f, 5.6f), vec3f(4.2f, 7.0f, 5.9f));
    s.meshes.push_back(std::move(rimLight));

    LightGPU key;
    key.kind = LIGHT_QUAD;
    key.emission = s.materials[2].emission;
    key.v0 = vec3f(-9.2f, 4.2f, -4.5f);
    key.edgeU = vec3f(0.f, 4.8f, 0.f);
    key.edgeV = vec3f(0.f, 0.f, 7.f);
    key.normal = normalizeVec(vec3f(1.f, -0.2f, -0.15f));
    key.area = owl::length(key.edgeU) * owl::length(key.edgeV);
    s.lights.push_back(key);

    LightGPU fill;
    fill.kind = LIGHT_QUAD;
    fill.emission = s.materials[3].emission;
    fill.v0 = vec3f(9.0f, 2.8f, -2.5f);
    fill.edgeU = vec3f(0.f, 4.f, 0.f);
    fill.edgeV = vec3f(0.f, 0.f, 6.7f);
    fill.normal = normalizeVec(vec3f(-1.f, -0.2f, -0.1f));
    fill.area = owl::length(fill.edgeU) * owl::length(fill.edgeV);
    s.lights.push_back(fill);

    LightGPU rim;
    rim.kind = LIGHT_QUAD;
    rim.emission = s.materials[4].emission;
    rim.v0 = vec3f(-4.2f, 6.7f, 5.6f);
    rim.edgeU = vec3f(8.4f, 0.f, 0.f);
    rim.edgeV = vec3f(0.f, 0.3f, 0.f);
    rim.normal = normalizeVec(vec3f(0.f, -0.35f, -1.f));
    rim.area = owl::length(rim.edgeU) * owl::length(rim.edgeV);
    s.lights.push_back(rim);

    s.computeBounds();
    return s;
  }

  Scene Scene::makeTestScene()
  {
    using owl::vec3f;
    Scene s;

    s.materials.push_back(makeLambertian(vec3f(1.f)));
    s.materials.push_back(makeEmissive(vec3f(10.f)));
    s.materials.push_back(makeConductor(vec3f(0.14f, 0.43f, 1.38f),
                                        vec3f(4.54f, 2.455f, 1.914f),
                                        0.0001f));
    s.materials.push_back(makeConductor(vec3f(0.14f, 0.43f, 1.38f),
                                        vec3f(4.54f, 2.455f, 1.914f),
                                        0.1f));
    s.materials.push_back(makeConductor(vec3f(0.04f, 0.06f, 0.04f),
                                        vec3f(4.8f, 3.586f, 2.657f),
                                        0.0001f));
    s.materials.push_back(makeDielectric(1.5f, 0.0001f));
    s.materials.push_back(makeThinDielectric(1.5f));
    s.materials.push_back(makeLambertian(vec3f(0.8f, 0.2f, 0.2f)));
    s.materials.push_back(makeDisneyPrincipled(
      vec3f(0.7f, 0.05f, 0.05f), 0.0f, 0.75f, 0.0f, 0.6f, 0.3f, 0.9f,
      0.0f, 0.0f, 0.0f, 1.0f, 0.95f, 1.5f));

    TriangleMesh floor;
    floor.materialId = 0;
    pushBox(floor, vec3f(-40.f, -0.55f, -40.f), vec3f(40.f, -0.5f, 40.f));
    s.meshes.push_back(std::move(floor));

    const ObjData bunnyObj = loadObjData(std::string(PATHTRACER_ASSET_DIR) + "/bunny.obj");
    const float bunnyScale = 24.f;
    const float bunnyRotY = float(M_PI_2);
    const float baseY = -0.5f;
    const vec3f bunnyPositions[] = {
      vec3f(-5.8f, baseY, -1.5f),
      vec3f(-3.4f, baseY,  1.6f),
      vec3f(-0.9f, baseY, -1.4f),
      vec3f( 1.6f, baseY,  1.5f),
      vec3f( 4.1f, baseY, -1.3f),
      vec3f( 6.2f, baseY,  1.6f),
      vec3f( 8.0f, baseY, -1.2f),
    };
    const int32_t bunnyMaterials[] = {7, 5, 6, 2, 3, 4, 8};

    for (int i = 0; i < 7; ++i) {
      s.meshes.push_back(makeMeshFromObjData(bunnyObj,
                                             bunnyMaterials[i],
                                             bunnyScale,
                                             bunnyRotY,
                                             bunnyPositions[i]));
    }

    TriangleMesh light;
    light.materialId = 1;
    pushBox(light, vec3f(-2.f, 13.95f, -3.f), vec3f(6.f, 14.0f, 5.f));
    s.meshes.push_back(std::move(light));

    TriangleMesh goldBackdrop;
    goldBackdrop.materialId = 2;
    pushBox(goldBackdrop, vec3f(-12.05f, -40.f, -40.f), vec3f(-12.f, 40.f, 40.f));
    s.meshes.push_back(std::move(goldBackdrop));

    TriangleMesh roughGoldBackdrop;
    roughGoldBackdrop.materialId = 3;
    pushBox(roughGoldBackdrop, vec3f(-40.f, -40.f, -8.05f), vec3f(40.f, 40.f, -8.f));
    s.meshes.push_back(std::move(roughGoldBackdrop));

    LightGPU quadLight;
    quadLight.kind = LIGHT_QUAD;
    quadLight.emission = s.materials[1].emission;
    quadLight.v0 = vec3f(-2.f, 13.95f, -3.f);
    quadLight.edgeU = vec3f(8.f, 0.f, 0.f);
    quadLight.edgeV = vec3f(0.f, 0.f, 8.f);
    quadLight.normal = vec3f(0.f, -1.f, 0.f);
    quadLight.area = 64.f;
    s.lights.push_back(quadLight);

    s.computeBounds();
    return s;
  }

} // namespace mypt

#include "scene/Scene.h"

namespace mypt {

  void Scene::computeBounds()
  {
    bounds = owl::box3f();
    for (const auto &m : meshes)
      for (const auto &v : m.vertices)
        bounds.extend(v);
  }

} // namespace mypt

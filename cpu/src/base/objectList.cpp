#include "base/objectList.hpp"

AABB ObjectList::getAABB(float t0, float t1) {
  AABB aabb;
  bool is_first = true;
  for (const auto &object : this->objects) {
    aabb = is_first ? object->getAABB(t0, t1)
                    : AABB::merge(aabb, object->getAABB(t0, t1));
    is_first = false;
  }
  return aabb;
}
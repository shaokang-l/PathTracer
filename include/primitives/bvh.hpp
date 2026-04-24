#pragma once
#include "base/objectList.hpp"
#include "base/primitive.hpp"
#include "utils/utility.hpp"
#include <algorithm>
#include <cstdlib>
#include <functional>

class BVHNode : public Hittable
{
public:
  BVHNode() = default;
  BVHNode(ObjectList &object_list, uint i, uint j, float t0, float t1);
  BVHNode(ObjectList &object_list, float t0 = 0.0f, float t1 = 1.0f)
      : BVHNode(object_list, 0, object_list.getLists().size(), t0, t1) {};

  bool intersect(const Ray &ray, HitRecord &hit_record, float tmin,
                 float tmax) const override;

  AABB getAABB(float t0, float t1) override { return this->box; };

  // compute on construction
  AABB box;
  std::shared_ptr<Hittable> left;
  std::shared_ptr<Hittable> right;
};

inline bool BVHNode::intersect(const Ray &ray, HitRecord &hit_record,
                               float tmin = 0.0001, float tmax = 1e5) const
{

  if (!this->box.intersect(ray, tmin, tmax))
  {
    return false;
  }

  auto left_hit = this->left->intersect(ray, hit_record, tmin, tmax);
  auto right_hit = this->right->intersect(ray, hit_record, tmin,
                                          left_hit ? hit_record.t : tmax);

  return left_hit || right_hit;
};

// randomly choose an axis
// sort the primitives
// put half in each subtree
inline BVHNode::BVHNode(ObjectList &object_list, uint i, uint j, float t0,
                        float t1)
{

  int axis = gl::C_rand_int(0, 3);
  auto in_list = object_list;
  auto list_copy = in_list.getLists();

  auto compare = [&](std::shared_ptr<Hittable> p1, std::shared_ptr<Hittable> p2,
                     int axis_index)
  {
    auto aabb1 = p1->getAABB(t0, t1);
    auto aabb2 = p2->getAABB(t0, t1);
    // Use center point instead of min for better handling of thin AABBs
    return (aabb1.get_min()[axis_index] + aabb1.get_max()[axis_index]) * 0.5f <
           (aabb2.get_min()[axis_index] + aabb2.get_max()[axis_index]) * 0.5f;
  };

  auto comparator =
      std::bind(compare, std::placeholders::_1, std::placeholders::_2, axis);

  // single element
  if (j - i == 1)
  {
    this->left = list_copy[i];
    this->right = list_copy[i];
    // 2 elements, compare the min
  }
  else if (j - i == 2)
  {

    if (compare(list_copy[i], list_copy[i + 1], axis))
    {
      this->left = list_copy[i];
      this->right = list_copy[i + 1];
    }
    else
    {
      this->left = list_copy[i + 1];
      this->right = list_copy[i];
    }

    // 3 elements, sort and divide
  }
  else
  {
    std::sort(list_copy.begin() + i, list_copy.begin() + j, comparator);

    uint mid = i + (j - i) / 2;
    this->left = std::make_shared<BVHNode>(in_list, i, mid, t0, t1);
    this->right = std::make_shared<BVHNode>(in_list, mid, j, t0, t1);
  }

  this->box =
      AABB::merge(this->left->getAABB(t0, t1), this->right->getAABB(t0, t1));
  this->objtype = ObjType::BVH_OBJ;
};

inline void print_BVH(std::shared_ptr<BVHNode> root)
{
  std::vector<std::shared_ptr<Hittable>> curr, next;
  curr.push_back(root);

  while (!curr.empty())
  {
    for (auto &node : curr)
    {
      if (node->objtype == ObjType::BVH_OBJ)
        std::cout << "1 ";
      else
        std::cout << "0 ";
    }

    std::cout << std::endl;

    for (auto &node : curr)
    {
      if (node->objtype == ObjType::BVH_OBJ)
      {
        next.push_back(std::dynamic_pointer_cast<BVHNode>(node)->left);
        next.push_back(std::dynamic_pointer_cast<BVHNode>(node)->right);
      }
    }
    curr = next;
    next.clear();
  }
}

inline void printBT(const std::string &prefix, std::shared_ptr<Hittable> node,
                    bool isLeft)
{
  if (node != nullptr)
  {
    std::cout << prefix;

    std::cout << (isLeft ? "├──" : "└──");

    // print the value of the node
    std::cout << "1" << std::endl;

    if (node->objtype == ObjType::BVH_OBJ)
    {
      bool is_dup = false;
      if (std::dynamic_pointer_cast<BVHNode>(node)->left ==
          std::dynamic_pointer_cast<BVHNode>(node)->right)
        is_dup = true;
      printBT(prefix + (isLeft ? "│   " : "    "),
              std::dynamic_pointer_cast<BVHNode>(node)->left, true);
      if (!is_dup)
        printBT(prefix + (isLeft ? "│   " : "    "),
                std::dynamic_pointer_cast<BVHNode>(node)->right, false);
    }
  }
}

void printBT(std::shared_ptr<Hittable> node) { printBT("", node, false); }
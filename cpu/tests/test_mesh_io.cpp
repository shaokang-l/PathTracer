// tests/test_mesh_io.cpp
#include "mesh_io/meshLoader.hpp"
#include "primitives/triangleMesh.hpp"
#include <gtest/gtest.h>

TEST(MeshLoader, SimpleTriangle) {
  using namespace gl;
  auto mat = gl::DefaultMaterial;
  auto mesh = loadOBJMesh("../../tests/data/minimal.obj", mat);

  auto const &md = mesh->getMeshData();
  // 3 verts, 1 face
  assert(md.indices.size() == 1u);
  assert(md.positions.size() == 3u);
  assert(md.normals.size() == 0u);
  assert(md.uvs.size() == 0u);

  EXPECT_EQ(md.positions[0], vec3(0, 0, 0));
  EXPECT_EQ(md.positions[1], vec3(1, 0, 0));
  EXPECT_EQ(md.positions[2], vec3(0, 1, 0));

  auto aabb = mesh->getAABB(0, 1);
  EXPECT_TRUE((aabb.get_min() - gl::vec3(0, 0, 0)).near_zero(1e-5));
  EXPECT_TRUE((aabb.get_max() - gl::vec3(1, 1, 0)).near_zero(1e-5));

  Ray r(gl::vec3(0.25f, 0.25f, -1), gl::vec3(0, 0, 1));
  HitRecord rec;
  ASSERT_TRUE(mesh->intersect(r, rec, 0.001f, 100.f));

  EXPECT_FLOAT_EQ(rec.position.x(), 0.25f);
  EXPECT_FLOAT_EQ(rec.position.y(), 0.25f);
  EXPECT_FLOAT_EQ(rec.position.z(), 0.0f);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

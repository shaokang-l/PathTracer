#pragma once
#include "./meshData.hpp"
#include "base/primitive.hpp"
#include "primitives/triangleMesh.hpp"
#include <memory>
#include <string>

#ifdef HAS_FBX_SDK
// Forward declarations for FBX SDK
namespace fbxsdk
{
  class FbxManager;
  class FbxScene;
  class FbxNode;
} // namespace fbxsdk

std::shared_ptr<TriangleMesh> loadFBXMesh(const std::string &path,
                                          std::shared_ptr<Material> mat,
                                          std::shared_ptr<MediumInterface> medium_interface = nullptr);

// Helper functions
void processFBXNode(fbxsdk::FbxNode *node, MeshData &data);
void processFBXMesh(fbxsdk::FbxNode *node, MeshData &data);
#else
// Stub function that throws when FBX SDK is not available
inline std::shared_ptr<TriangleMesh>
loadFBXMesh(const std::string &path, std::shared_ptr<Material> mat)
{
  throw std::runtime_error(
      "FBX support is not enabled. Please rebuild with FBX SDK.");
}
#endif
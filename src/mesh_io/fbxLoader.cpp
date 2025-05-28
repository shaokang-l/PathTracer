#include "mesh_io/fbxLoader.hpp"

#ifdef HAS_FBX_SDK
#include <fbxsdk.h>
#include <iostream>
#include <stdexcept>

using namespace fbxsdk;

std::shared_ptr<TriangleMesh> loadFBXMesh(const std::string &path,
                                          std::shared_ptr<Material> mat,
                                          std::shared_ptr<MediumInterface> medium_interface)
{
  // Initialize the FBX SDK
  FbxManager *fbxManager = FbxManager::Create();
  if (!fbxManager)
  {
    throw std::runtime_error("Failed to create FBX Manager");
  }

  // Create an IOSettings object
  FbxIOSettings *ios = FbxIOSettings::Create(fbxManager, IOSROOT);
  fbxManager->SetIOSettings(ios);

  // Create an importer
  FbxImporter *importer = FbxImporter::Create(fbxManager, "");
  if (!importer->Initialize(path.c_str(), -1, fbxManager->GetIOSettings()))
  {
    throw std::runtime_error(
        "Failed to initialize FBX importer: " +
        std::string(importer->GetStatus().GetErrorString()));
  }

  // Create a new scene
  FbxScene *scene = FbxScene::Create(fbxManager, "Scene");
  if (!scene)
  {
    throw std::runtime_error("Failed to create FBX scene");
  }

  // Import the scene
  if (!importer->Import(scene))
  {
    throw std::runtime_error("Failed to import FBX scene");
  }

  // Convert the scene to use right-handed coordinates
  FbxAxisSystem::OpenGL.ConvertScene(scene);

  // Process the scene
  MeshData data;
  FbxNode *rootNode = scene->GetRootNode();
  if (rootNode)
  {
    processFBXNode(rootNode, data);
  }

  // Clean up
  importer->Destroy();
  fbxManager->Destroy();

  return std::make_shared<TriangleMesh>(std::move(data), mat, medium_interface);
}

void processFBXNode(FbxNode *node, MeshData &data)
{
  if (!node)
    return;

  // Process the current node if it's a mesh
  if (node->GetNodeAttribute() &&
      node->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eMesh)
  {
    processFBXMesh(node, data);
  }

  // Process all children
  for (int i = 0; i < node->GetChildCount(); i++)
  {
    processFBXNode(node->GetChild(i), data);
  }
}

void processFBXMesh(FbxNode *node, MeshData &data)
{
  FbxMesh *mesh = node->GetMesh();
  if (!mesh)
    return;

  // Get the mesh's control points (vertices)
  FbxVector4 *controlPoints = mesh->GetControlPoints();
  int numVertices = mesh->GetControlPointsCount();

  // Get vertex positions
  for (int i = 0; i < numVertices; i++)
  {
    FbxVector4 vertex = controlPoints[i];
    data.positions.emplace_back(vertex[0], vertex[1], vertex[2]);
  }

  // Get vertex normals
  FbxGeometryElementNormal *normalElement = mesh->GetElementNormal();
  if (normalElement)
  {
    for (int i = 0; i < numVertices; i++)
    {
      FbxVector4 normal;
      if (normalElement->GetReferenceMode() == FbxGeometryElement::eDirect)
      {
        normal = normalElement->GetDirectArray().GetAt(i);
      }
      else
      {
        int index = normalElement->GetIndexArray().GetAt(i);
        normal = normalElement->GetDirectArray().GetAt(index);
      }
      data.normals.emplace_back(normal[0], normal[1], normal[2]);
    }
  }

  // Get UV coordinates
  FbxGeometryElementUV *uvElement = mesh->GetElementUV();
  if (uvElement)
  {
    for (int i = 0; i < numVertices; i++)
    {
      FbxVector2 uv;
      if (uvElement->GetReferenceMode() == FbxGeometryElement::eDirect)
      {
        uv = uvElement->GetDirectArray().GetAt(i);
      }
      else
      {
        int index = uvElement->GetIndexArray().GetAt(i);
        uv = uvElement->GetDirectArray().GetAt(index);
      }
      data.uvs.emplace_back(uv[0], uv[1]);
    }
  }

  // Get polygon indices
  int numPolygons = mesh->GetPolygonCount();
  for (int i = 0; i < numPolygons; i++)
  {
    int numVertices = mesh->GetPolygonSize(i);
    if (numVertices != 3)
    {
      std::cerr << "Warning: Non-triangle face found in FBX mesh. Skipping."
                << std::endl;
      continue;
    }

    // Get the three vertices of the triangle
    int v1 = mesh->GetPolygonVertex(i, 0);
    int v2 = mesh->GetPolygonVertex(i, 1);
    int v3 = mesh->GetPolygonVertex(i, 2);

    // Add the triangle indices
    data.indices.push_back({v1, v2, v3});

    // Add normal indices if available
    if (normalElement)
    {
      FbxVector4 n1, n2, n3;
      mesh->GetPolygonVertexNormal(i, 0, n1);
      mesh->GetPolygonVertexNormal(i, 1, n2);
      mesh->GetPolygonVertexNormal(i, 2, n3);
      data.normalIndices.push_back({v1, v2, v3}); // Use vertex indices for now
    }

    // Add UV indices if available
    if (uvElement)
    {
      FbxVector2 uv1, uv2, uv3;
      bool unmapped;
      mesh->GetPolygonVertexUV(i, 0, uvElement->GetName(), uv1, unmapped);
      mesh->GetPolygonVertexUV(i, 1, uvElement->GetName(), uv2, unmapped);
      mesh->GetPolygonVertexUV(i, 2, uvElement->GetName(), uv3, unmapped);
      data.uvIndices.push_back({v1, v2, v3}); // Use vertex indices for now
    }
  }
}
#endif

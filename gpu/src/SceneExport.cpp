#include "SceneExport.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace mypt {
namespace {

  static std::string xmlEscape(const std::string &s)
  {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
      switch (c) {
      case '&': out += "&amp;"; break;
      case '<': out += "&lt;"; break;
      case '>': out += "&gt;"; break;
      case '"': out += "&quot;"; break;
      default: out += c; break;
      }
    }
    return out;
  }

  static std::string vecString(const owl::vec3f &v)
  {
    std::ostringstream out;
    out << std::setprecision(7) << v.x << " " << v.y << " " << v.z;
    return out.str();
  }

  static bool isEmissive(const MaterialGPU &m)
  {
    return m.kind == MATERIAL_EMISSIVE &&
           (m.emission.x > 0.f || m.emission.y > 0.f || m.emission.z > 0.f);
  }

  static void writeMaterial(std::ostream &out,
                            const MaterialGPU &m,
                            int id)
  {
    out << "  <bsdf id=\"mat_" << id << "\" ";
    switch (m.kind) {
    case MATERIAL_LAMBERTIAN:
      out << "type=\"diffuse\">\n"
          << "    <rgb name=\"reflectance\" value=\"" << vecString(m.albedo) << "\"/>\n"
          << "  </bsdf>\n\n";
      break;
    case MATERIAL_MIRROR:
      out << "type=\"conductor\">\n"
          << "    <rgb name=\"specularReflectance\" value=\"" << vecString(m.albedo) << "\"/>\n"
          << "  </bsdf>\n\n";
      break;
    case MATERIAL_CONDUCTOR:
      out << "type=\"conductor\">\n"
          << "    <rgb name=\"eta\" value=\"" << vecString(m.eta) << "\"/>\n"
          << "    <rgb name=\"k\" value=\"" << vecString(m.k) << "\"/>\n"
          << "    <float name=\"alpha\" value=\"" << std::max(m.alpha_x, m.alpha_y) << "\"/>\n"
          << "  </bsdf>\n\n";
      break;
    case MATERIAL_DIELECTRIC:
    case MATERIAL_THIN_DIELECTRIC:
      out << "type=\"dielectric\">\n"
          << "    <float name=\"intIOR\" value=\"" << m.ior << "\"/>\n"
          << "  </bsdf>\n\n";
      break;
    case MATERIAL_DISNEY_PRINCIPLED:
      out << "type=\"disneybsdf\">\n"
          << "    <rgb name=\"baseColor\" value=\"" << vecString(m.baseColor) << "\"/>\n"
          << "    <float name=\"specularTransmission\" value=\"" << m.specularTransmission << "\"/>\n"
          << "    <float name=\"metallic\" value=\"" << m.metallic << "\"/>\n"
          << "    <float name=\"subsurface\" value=\"" << m.subsurface << "\"/>\n"
          << "    <float name=\"specular\" value=\"" << m.specular << "\"/>\n"
          << "    <float name=\"roughness\" value=\"" << m.roughness << "\"/>\n"
          << "    <float name=\"specularTint\" value=\"" << m.specularTint << "\"/>\n"
          << "    <float name=\"anisotropic\" value=\"" << m.anisotropic << "\"/>\n"
          << "    <float name=\"sheen\" value=\"" << m.sheen << "\"/>\n"
          << "    <float name=\"sheenTint\" value=\"" << m.sheenTint << "\"/>\n"
          << "    <float name=\"clearcoat\" value=\"" << m.clearcoat << "\"/>\n"
          << "    <float name=\"clearcoatGloss\" value=\"" << m.clearcoatGloss << "\"/>\n"
          << "    <float name=\"eta\" value=\"" << m.ior << "\"/>\n"
          << "  </bsdf>\n\n";
      break;
    case MATERIAL_EMISSIVE:
    default:
      out << "type=\"diffuse\">\n"
          << "    <rgb name=\"reflectance\" value=\"0.8 0.8 0.8\"/>\n"
          << "  </bsdf>\n\n";
      break;
    }
  }

  static void writeObj(const TriangleMesh &mesh, const std::filesystem::path &path)
  {
    std::ofstream out(path);
    if (!out) {
      throw std::runtime_error("Failed to write OBJ: " + path.string());
    }

    out << std::setprecision(7);
    for (const owl::vec3f &v : mesh.vertices) {
      out << "v " << v.x << " " << v.y << " " << v.z << "\n";
    }
    for (const owl::vec3i &f : mesh.indices) {
      out << "f " << (f.x + 1) << " " << (f.y + 1) << " " << (f.z + 1) << "\n";
    }
  }

} // namespace

void exportMitsubaXmlScene(const Scene &scene, const std::string &xmlPathString)
{
  const std::filesystem::path xmlPath(xmlPathString);
  const std::filesystem::path outDir =
    xmlPath.has_parent_path() ? xmlPath.parent_path() : std::filesystem::path(".");
  std::filesystem::create_directories(outDir);

  const std::filesystem::path meshDir =
    outDir / (xmlPath.stem().string() + "_meshes");
  std::filesystem::create_directories(meshDir);

  std::ofstream xml(xmlPath);
  if (!xml) {
    throw std::runtime_error("Failed to write XML: " + xmlPath.string());
  }

  xml << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n\n"
      << "<scene version=\"0.5.0\">\n"
      << "  <integrator type=\"path\">\n"
      << "    <integer name=\"maxDepth\" value=\"8\"/>\n"
      << "  </integrator>\n\n";

  for (size_t i = 0; i < scene.materials.size(); ++i) {
    writeMaterial(xml, scene.materials[i], int(i));
  }

  for (size_t i = 0; i < scene.meshes.size(); ++i) {
    const TriangleMesh &mesh = scene.meshes[i];
    std::ostringstream name;
    name << "mesh_" << std::setw(3) << std::setfill('0') << i << ".obj";
    const std::filesystem::path objPath = meshDir / name.str();
    writeObj(mesh, objPath);

    const std::filesystem::path relativeObj =
      std::filesystem::relative(objPath, outDir);
    xml << "  <shape type=\"obj\" id=\"mesh_" << i << "\">\n"
        << "    <string name=\"filename\" value=\""
        << xmlEscape(relativeObj.generic_string()) << "\"/>\n";

    if (mesh.materialId >= 0 &&
        size_t(mesh.materialId) < scene.materials.size() &&
        isEmissive(scene.materials[mesh.materialId])) {
      xml << "    <emitter type=\"area\">\n"
          << "      <rgb name=\"radiance\" value=\""
          << vecString(scene.materials[mesh.materialId].emission) << "\"/>\n"
          << "    </emitter>\n";
    } else {
      xml << "    <ref name=\"bsdf\" id=\"mat_" << mesh.materialId << "\"/>\n";
    }
    xml << "  </shape>\n\n";
  }

  xml << "  <sensor type=\"perspective\">\n"
      << "    <float name=\"fov\" value=\"45\"/>\n"
      << "    <transform name=\"toWorld\">\n"
      << "      <lookAt origin=\"0, 5, 18\" target=\"0, 0, 0\" up=\"0, 1, 0\"/>\n"
      << "    </transform>\n"
      << "    <sampler type=\"independent\">\n"
      << "      <integer name=\"sampleCount\" value=\"256\"/>\n"
      << "    </sampler>\n"
      << "    <film type=\"hdrfilm\">\n"
      << "      <integer name=\"width\" value=\"1280\"/>\n"
      << "      <integer name=\"height\" value=\"720\"/>\n"
      << "    </film>\n"
      << "  </sensor>\n"
      << "</scene>\n";
}

} // namespace mypt


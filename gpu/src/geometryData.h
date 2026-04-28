// ======================================================================== //
// geometryData.h - host/device SBT records for geometry programs.
//
// These structs are mirrored by OWLVarDecl declarations on the host.
// ======================================================================== //

#pragma once

#include <cstdint>
#include <owl/common/math/vec.h>

using namespace owl;

// Per-geom SBT record (one per OWLGeom, filled via owlGeomSet*).
struct TriangleMeshSBT {
  vec3f       *vertex;   // device pointer
  vec3i       *index;    // device pointer
  int32_t     materialId;
};

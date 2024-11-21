//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-11-11 16:09:31
//

#pragma once

#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <recast/Recast.h>
#include <recast/DetourNavMesh.h>
#include <recast/DetourNavMeshBuilder.h>
#include <recast/DetourNavMeshQuery.h>
#include <recast/DetourCrowd.h>

#define NOMINMAX
#include "wn_common.h"
#include "wn_gltf.h"

struct navmesh
{
    rcConfig config;
    rcHeightfield* height_field;
    rcCompactHeightfield* compact_height_field;
    rcContourSet* contour_set;
    rcPolyMesh* poly_mesh;
    rcPolyMeshDetail* poly_mesh_detail;

    dtNavMesh* mesh;
    dtNavMeshQuery* query;
    dtCrowd* agent_manager;
};

struct navmesh_build_info
{
    std::vector<gltf_vertex> vertices;
    std::vector<u32> indices;

    glm::vec3 min = glm::vec3(0.0f);
    glm::vec3 max = glm::vec3(0.0f);
};

void navmesh_init(navmesh *mesh, const navmesh_build_info& info);
void navmesh_free(navmesh *mesh);

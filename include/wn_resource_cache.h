//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-11-05 16:41:25
//

#pragma once

#include <unordered_map>

#include "wn_gltf.h"
#include "wn_d3d12.h"
#include "wn_video.h"

enum resource_type
{
    ResourceType_GLTF,
    ResourceType_Texture,
    ResourceType_Skybox, /// @note(ame): unused
    ResourceType_Audio, /// @note(ame): unused
    ResourceType_Navmesh, /// @note(ame): unused
    ResourceType_Script /// @note(ame): unused
};

struct resource
{
    std::string path;
    resource_type type;

    gltf_model model;
    texture tex;
    u32 ref_count = 0; /// @note(ame): used for resource reuse
};

struct resource_cache
{
    std::unordered_map<std::string, resource*> resources;
};

void resource_cache_init();
resource *resource_cache_get(const std::string& path, resource_type type, bool gen_collisions = true);
void resource_cache_give_back(resource *res);
void resource_cache_free();

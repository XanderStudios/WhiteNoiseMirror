//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-10-27 13:57:47
//

#pragma once

#include <cgltf/cgltf.h>
#include <glm/glm.hpp>

#include <string>
#include <array>
#include <unordered_map>
#include <memory>

#include "wn_d3d12.h"
#include "wn_bitmap.h"
#include "wn_physics.h"

struct resource;

constexpr u32 MAX_BONE_WEIGHTS = 4;

struct gltf_vertex
{
    glm::vec3 Position;
    glm::vec2 UV;
    glm::vec3 Normals;
    i32 MaxBoneInfluence[MAX_BONE_WEIGHTS];
    f32 Weights[MAX_BONE_WEIGHTS];
};

struct gltf_texture
{
    resource* handle = nullptr;
    tvc_entry* view = nullptr;
};

struct gltf_material
{
    gltf_texture* albedo;
    bool has_albedo;
};

struct gltf_primitive
{
    physics_body body;
    buffer vertex_buffer = {};
    buffer index_buffer = {};

    u32 vtx_count = 0;
    u32 idx_count = 0;
    i32 material_index = -1;
};

struct gltf_node
{
    std::vector<gltf_primitive> primitives;
    std::array<buffer, FRAMES_IN_FLIGHT> model_buffer;

    std::string name;
    glm::mat4 transform;
    gltf_node *parent;
    std::vector<gltf_node*> children;
};

struct gltf_model
{
    std::string path;
    std::string directory;
    bool gen_collisions;

    gltf_node *root;
    std::vector<gltf_material> materials;
    std::unordered_map<std::string, gltf_texture> textures;
    u32 physics_counter = 0;

    u32 vtx_count;
    u32 idx_count;
};

void gltf_model_load(gltf_model *model, const std::string& path, bool generate_collisions = true);
void gltf_model_free(gltf_model *model);

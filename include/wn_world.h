//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-11-02 15:57:59
//

#pragma once

#include "wn_gltf.h"
#include "wn_script.h"
#include "wn_audio.h"
#include "wn_resource_cache.h"

enum EntityType
{
    EntityType_NotPrecised,
    EntityType_Player,
    EntityType_Trigger,
    EntityType_Enemy
};

struct transform
{
    glm::vec3 position;
    glm::quat rotation;
    glm::vec3 scale;

    glm::vec3 forward;
    glm::vec3 up;
    glm::vec3 right;

    glm::mat4 matrix;
};

struct entity
{
    /// @brief Entity info
    u64 id;
    std::string name;
    transform entity_transform;
    EntityType type;

    /// @brief Components
    bool has_model;
    resource* model;

    bool has_physics_body;
    physics_body physics_body;

    /// @note(ame): unique to player entity
    bool has_physics_character;
    physics_character character;

    bool has_scripts;
    std::vector<game_script> scripts;
};

struct game_world
{
    /// @note(ame): World info
    std::string name;
    std::string serialization_path;
    /// @todo(ame): resource cache
    glm::vec3 player_start_position;

    /// @note(ame): Game objects
    resource* level;
    entity player;
    std::vector<entity> entities;
};

struct game_world_info
{
    std::string level_path;
    glm::vec3 start_pos;
    /// @todo(ame): entities and whatnot
};

void game_world_init(game_world *world, game_world_info *info);
void game_world_update(game_world *world);
void game_world_free(game_world *world);

/// @todo(ame): Serialization

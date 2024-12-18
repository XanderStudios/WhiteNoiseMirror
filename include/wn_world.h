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
#include "wn_ai.h"

struct game_world;

enum entity_type
{
    EntityType_NotPrecised,
    EntityType_Player,
    EntityType_Trigger,
    EntityType_Enemy
};

enum trigger_type
{
    TriggerType_NotPrecised,
    TriggerType_Transition,
    TriggerType_Camera
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
    entity_type type;
    game_world *parent_world;

    /// @brief Components
    bool has_model;
    resource* model;

    bool has_physics_body;
    physics_body physics_body;

    bool has_trigger;
    physics_trigger trigger;
    u32 trigger_id;
    trigger_type t_type;

    /// @note(ame): TriggerType_Transition
    std::string trigger_transition;

    /// @note(ame): TriggerType_Camera
    glm::vec3 point_position;
    glm::vec3 point_forward;
    glm::mat4 view_matrix;

    /// @note(ame): unique to player entity
    bool has_physics_character;
    physics_character character;

    bool has_scripts;
    std::vector<game_script> scripts;
};

struct game_world
{
    /// @note(ame): World descriptor
    std::string name;
    std::string serialization_path;
    glm::vec3 start_position;
    glm::vec3 bbox_min;
    glm::vec3 bbox_max;

    /// @note(ame): World render data
    glm::mat4 main_camera_view;
    bool using_player_cam = true;

    /// @note(ame): World navmesh
    navmesh world_navmesh;

    /// @note(ame): Game objects
    resource* level;
    entity player;
    std::vector<entity*> entities;

    /// @note(ame): register trigger callbacks
    std::vector<std::function<void(entity* e1, entity *e2)>> on_enter_callbacks;
    std::vector<std::function<void(entity* e1, entity *e2)>> on_stay_callbacks;
    std::vector<std::function<void(entity* e1, entity *e2)>> on_exit_callbacks;
};

struct game_world_info
{
    std::string level_path;
    glm::vec3 start_pos;
    /// @todo(ame): entities and whatnot
};

void game_world_init(game_world *world, game_world_info *info);
void game_world_load(game_world *world, const std::string& path);
void game_world_save(game_world *world, const std::string& path = "");
void game_world_remove_entity(game_world *world, entity* e);
entity* game_world_add_trigger(game_world *world, glm::vec3 position, glm::vec3 size, glm::quat q = glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
void game_world_update(game_world *world, f32 dt);
void game_world_free(game_world *world);

/// @todo(ame): Serialization

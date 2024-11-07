//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-11-02 16:20:21
//

#include <json/json.hpp>

#include "wn_world.h"
#include "wn_player.h"
#include "wn_filesystem.h"
#include "wn_util.h"
#include "wn_notification.h"

void game_world_init(game_world *world, game_world_info *info)
{
    /// @note(ame): initialize level
    world->level = resource_cache_get(info->level_path, ResourceType_GLTF, true);

    /// @note(ame): initialize player
    player_init(&world->player, info->start_pos);

    /// @note(ame): initialize entities

    log("[world] Loaded world");
}

void game_world_load(game_world *world, const std::string& path)
{
    nlohmann::json root = fs_loadjson(path);

    /// @note(ame): load levels
    world->name = root["name"];
    world->serialization_path = path;
    world->level = resource_cache_get(root["level_model"], ResourceType_GLTF, true);
    
    /// @note(ame): Load start position and player
    world->start_position.x = root["start_pos"][0].template get<float>();
    world->start_position.y = root["start_pos"][1].template get<float>();
    world->start_position.z = root["start_pos"][2].template get<float>();

    physics_clear_characters();
    player_init(&world->player, world->start_position);

    /// @note(ame): Load triggers
    u32 trigger_id = 0;
    auto trigger_array = root["triggers"];
    for (auto& trigger : trigger_array) {
        glm::vec3 trigger_position;
        trigger_position.x = trigger["position"][0].template get<float>();
        trigger_position.y = trigger["position"][1].template get<float>();
        trigger_position.z = trigger["position"][2].template get<float>();

        glm::vec3 trigger_size;
        trigger_size.x = trigger["size"][0].template get<float>();
        trigger_size.y = trigger["size"][1].template get<float>();
        trigger_size.z = trigger["size"][2].template get<float>();

        entity* new_entity = game_world_add_trigger(world, trigger_position, trigger_size);

        std::string type = trigger["type"].template get<std::string>();
        if (type.compare("transition") == 0) {
            new_entity->trigger_transition = trigger["transition_level"].template get<std::string>();
            new_entity->t_type = TriggerType_Transition;
        }
    }

    /// @note(ame): Done!
    log("[world] Loaded world %s", path.c_str());
}

void game_world_remove_entity(game_world *world, entity* e)
{
    for (u64 i = 0; i < world->entities.size(); i++) {
        if (e->id == world->entities[i]->id) {
            /// @note(ame): Found a match
            world->entities.erase(world->entities.begin() + i);
            if (e->has_trigger) {
                physics_trigger_free(&e->trigger);
            }
            delete e;
        }
    }
}

entity* game_world_add_trigger(game_world *world, glm::vec3 position, glm::vec3 size)
{
    entity* new_entity = new entity;
    new_entity->type = EntityType_Trigger;
    new_entity->has_trigger = true;
    new_entity->id = wn_uuid();
    new_entity->has_trigger = true;
    new_entity->trigger_id = new_entity->id;

    physics_trigger_init(&new_entity->trigger, position, size, new_entity);

    auto function = [&](entity* trigger, entity *e) {
        if (ImGui::IsKeyDown(ImGuiKey_Space)) {
            notification_payload payload;
            payload.type = NotificationType_LevelChange;
            payload.level_change.level_path = trigger->trigger_transition;
            game_send_notification(payload);
        }
    };
    world->on_stay_callbacks.push_back(function);
    new_entity->trigger.on_trigger_stay = world->on_stay_callbacks.back();

    new_entity->t_type = TriggerType_NotPrecised;
    world->entities.push_back(new_entity);
    return new_entity;
}

void game_world_save(game_world *world, const std::string& path)
{
    std::string save_path = path;
    if (path == "") {
        save_path = world->serialization_path;
    }

    nlohmann::json root;

    /// @note(ame): Scene data
    root["name"] = world->name;
    root["level_model"] = world->level->path;
    root["start_pos"][0] = world->start_position.x;
    root["start_pos"][1] = world->start_position.y;
    root["start_pos"][2] = world->start_position.z;

    /// @note(ame): Triggers
    root["triggers"] = nlohmann::json::array();
    for (auto& entity : world->entities) {
        if (entity->type == EntityType_Trigger) {
            nlohmann::json entity_root;
            
            entity_root["position"][0] = entity->trigger.position.x;
            entity_root["position"][1] = entity->trigger.position.y;
            entity_root["position"][2] = entity->trigger.position.z;
        
            entity_root["size"][0] = entity->trigger.size.x;
            entity_root["size"][1] = entity->trigger.size.y;
            entity_root["size"][2] = entity->trigger.size.z;

            entity_root["type"] = "none";
            if (entity->t_type == TriggerType_Transition) {
                entity_root["type"] = "transition";
                entity_root["transition_level"] = entity->trigger_transition;
            }

            root["triggers"].push_back(entity_root);
        }
    }

    fs_writejson(save_path, root);    
}

void game_world_update(game_world *world)
{
    player_update(&world->player);
}

void game_world_free(game_world *world)
{
    for (auto& entity : world->entities) {
        if (entity->has_trigger) {
            physics_trigger_free(&entity->trigger);
        }
        delete entity;
    }

    world->on_stay_callbacks.clear();
    world->entities.clear();
    player_free(&world->player);
    resource_cache_give_back(world->level);
}

//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-11-02 16:20:21
//

#include "wn_world.h"
#include "wn_player.h"

void game_world_init(game_world *world, game_world_info *info)
{
    /// @note(ame): initialize level
    world->level = resource_cache_get(info->level_path, ResourceType_GLTF, true);

    /// @note(ame): initialize player
    player_init(&world->player, info->start_pos);

    /// @note(ame): initialize entities

    log("[world] Loaded world");
}

void game_world_update(game_world *world)
{
    player_update(&world->player);
}

void game_world_free(game_world *world)
{
    player_free(&world->player);
    resource_cache_give_back(world->level);
}

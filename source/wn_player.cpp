//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-11-02 16:25:30
//

#include "wn_player.h"

void player_init(entity *p, glm::vec3 start_pos)
{
    p->name = "Player";

    p->has_model = true;
    p->model = resource_cache_get("assets/gltfs/player/untitled.gltf", ResourceType_GLTF, false);

    p->has_physics_character = true;
    physics_character_init(&p->character, new capsule_shape(0.5f, 1.5), start_pos, reinterpret_cast<void*>(p));
}

void player_update(entity *p)
{
    glm::vec3 velocity(0.0f);
    if (ImGui::IsKeyDown(ImGuiKey_UpArrow))
        velocity.z = -5.0f;
    if (ImGui::IsKeyDown(ImGuiKey_DownArrow)) {
        velocity.z = 5.0f;
    }
    if (ImGui::IsKeyDown(ImGuiKey_LeftArrow)) {
        velocity.x = -5.0f;
    }
    if (ImGui::IsKeyDown(ImGuiKey_RightArrow)) {
        velocity.x = 5.0f;
    }
    physics_character_move(&p->character, velocity);
}

void player_free(entity *p)
{
    resource_cache_give_back(p->model);
    physics_character_free(&p->character);
}

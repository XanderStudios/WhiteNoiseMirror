//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-11-02 16:25:30
//

#include "wn_player.h"
#include "wn_input.h"
#include "wn_debug_renderer.h"

struct player_data {
    // player controller
    glm::vec3 forward = glm::vec3(0.0f, 0.0f, 1.0f);
    glm::vec3 position;
    ray_result last_ray;

    // tpc
    f32 yaw;
    f32 pitch;
    glm::vec3 cam_position;
    glm::mat4 view;
} p_data;

void player_init(entity *p, glm::vec3 start_pos)
{
    p->name = "Player";

    p->has_model = true;
    p->model = resource_cache_get("assets/gltfs/player/untitled.gltf", ResourceType_GLTF, false);

    p->has_physics_character = true;
    physics_character_init(&p->character, new capsule_shape(0.5f, 1.5, physics_materials::CharacterMaterial), start_pos, reinterpret_cast<void*>(p));

    p_data = {};
}

void player_update(entity *p, f32 dt)
{
    glm::vec3 position = physics_character_get_position(&p->character);

    f32 h_distance = -3.0f * std::cos(glm::radians(p_data.pitch));
    f32 v_distance = -3.0f * std::sin(glm::radians(p_data.pitch));
    f32 theta = glm::radians(p_data.yaw);
    f32 offset_x = h_distance * std::sin(theta);
    f32 offset_z = h_distance * std::cos(theta);
    p_data.cam_position.x = position.x - offset_x;
    p_data.cam_position.y = position.y + v_distance;
    p_data.cam_position.z = position.z - offset_z;

    /// @note(ame): Player controller
    p_data.forward.x = position.x - p_data.cam_position.x;
    p_data.forward.z = position.z - p_data.cam_position.z;
    p_data.forward = glm::normalize(p_data.forward);

    /// @note(ame): test raycast
    glm::vec3 offset_position = position - (p_data.forward * 0.1f);
    p_data.last_ray = physics_character_trace_ray(&p->character, offset_position, p_data.cam_position);
    if (p_data.last_ray.hit) {
        glm::vec3 reverse_ray_dir = glm::normalize(position - p_data.cam_position);
        p_data.cam_position = p_data.last_ray.point + (reverse_ray_dir * 0.02f);
    }

    p_data.view = glm::lookAt(p_data.cam_position, position, glm::vec3(0.0f, 1.0f, 0.0f));

    f32 speed_modifier = 3.0f;
    glm::vec3 velocity(0.0f);
    
    if (input_get_mapping_value("Sprint", true))
        speed_modifier = 5.0f;

    /// @note(ame): gamepad
    {
        velocity = -p_data.forward * input_get_mapping_value_analog("ForwardBackward") * speed_modifier;
        p_data.yaw -= input_get_mapping_value_analog("RotateLeftRight");
        p_data.pitch -= input_get_mapping_value_analog("RotateUpDown");
    }

    /// @note(ame): keyboard and mouse
    {
        if (input_is_mouse_down(SDL_BUTTON_LEFT)) {
            p_data.yaw -= input_get_mouse_dx() * 0.1f;
            p_data.pitch -= input_get_mouse_dy() * 0.1f;
        }

        if (input_get_mapping_value("Forward", true))
            velocity = p_data.forward * speed_modifier;
        if (input_get_mapping_value("Backward", true))
            velocity = -p_data.forward * speed_modifier;
        if (input_get_mapping_value("TurnLeft", true))
            p_data.yaw += 1.0f;
        if (input_get_mapping_value("TurnRight", true))
            p_data.yaw -= 1.0f;
    }

    physics_character_move(&p->character, velocity);

    p_data.position = position;
}

void player_debug_draw(entity *p)
{
    debug_draw_line(p_data.position, p_data.position + p_data.forward);
}

void player_free(entity *p)
{
    resource_cache_give_back(p->model);
    physics_character_free(&p->character);
}

glm::mat4 player_get_view(entity *p)
{
    return p_data.view;
}

glm::vec3 player_get_forward(entity *p)
{
    return p_data.forward;
}

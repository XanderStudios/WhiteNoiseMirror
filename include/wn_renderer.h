//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-11-02 16:36:49
//

#pragma once

#include <array>

#include "wn_d3d12.h"
#include "wn_world.h"
#include "wn_debug_renderer.h"
#include "wn_physics.h"

struct game_forward
{
    hot_pipeline pipeline;

    texture white_texture;
    texture color_buffer;
    texture depth_buffer;

    sampler texture_sampler;

    std::array<buffer, FRAMES_IN_FLIGHT> flashlight_buffer;

    glm::vec3 flashlight_position;
    glm::vec3 flashlight_direction;
    f32 cutoff;
};

struct game_composite
{
    hot_pipeline pipeline;
    texture ldr_buffer;
};

struct game_render_info
{
    /// @note(ame): camera info
    glm::mat4 current_view;
    glm::mat4 current_projection;
    glm::vec3 current_position;
    glm::vec3 current_forward;
    u32 width;
    u32 height;

    /// @note(ame): world info
    game_world *world;

    /// @note(ame): internals
    video_frame* frame;

    /// @todo(ame): execute different passes depending on settings (aa, shadows, ao, etc)
    bool render_meshes;
    bool render_debug;
    bool lit_scene;
};

struct game_renderer
{
    /// @note(ame): passes
    game_forward forward;
    game_composite composite;
    debug_renderer* debug;

    /// @note(ame): per-frame pointer
    game_render_info *info;
};

extern game_renderer renderer;

void game_renderer_init(u32 width, u32 height);
void game_renderer_rebuild();
void game_renderer_render(game_render_info *info);
void game_renderer_free();

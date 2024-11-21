//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-10-26 01:17:07
//

#include <cstdio>
#include <functional>
#include <vector>

#include <SDL3/SDL.h>
#include <imgui_impl_sdl3.h>
#include <imgui.h>

#include "wn_video.h"
#include "wn_output.h"
#include "wn_shader.h"
#include "wn_audio.h"
#include "wn_bitmap.h"
#include "wn_debug_camera.h"
#include "wn_timer.h"
#include "wn_gltf.h"
#include "wn_physics.h"
#include "wn_debug_renderer.h"
#include "wn_script.h"
#include "wn_world.h"
#include "wn_renderer.h"
#include "wn_steam.h"
#include "wn_resource_cache.h"
#include "wn_uploader.h"
#include "wn_notification.h"
#include "wn_util.h"
#include "wn_editor.h"
#include "wn_player.h"
#include "wn_input.h"
#include "wn_discord.h"
#include "wn_dev_console.h"
#include "wn_cvar.h"

#define WINDOW_WIDTH 1600
#define WINDOW_HEIGHT 900

notification_handler noti_handler;

int main(void)
{
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
        throw_error("Failed to initialize SDL3!");
    }

    SDL_Window* window = SDL_CreateWindow("WHITE NOISE", WINDOW_WIDTH, WINDOW_HEIGHT, 0);
    if (!window) {
        throw_error("Failed to create SDL3 window");
    }

    /// @note(ame): initialize system
    steam_init();
    bitmap_compress_recursive("assets/");
    cvar_load("assets/cvars.json");
    discord_init();
    video_init(window);
    resource_cache_init();
    audio_init();
    physics_init();
    script_system_init();
    game_renderer_init(WINDOW_WIDTH, WINDOW_HEIGHT);
    input_init();
    dev_console_init();

    /// @note(ame): init mappings
    input_add_mapping_binding_key("Forward", SDLK_Z);
    input_add_mapping_binding_key("Backward", SDLK_S);
    input_add_mapping_binding_key("TurnLeft", SDLK_Q);
    input_add_mapping_binding_key("TurnRight", SDLK_D);
    input_add_mapping_binding_axis("ForwardBackward", AXIS_LEFT_STICK_Y);
    input_add_mapping_binding_axis("RotateLeftRight", AXIS_RIGHT_STICK_X);
    input_add_mapping_binding_axis("RotateLeftRight", AXIS_LEFT_STICK_X);
    input_add_mapping_binding_axis("RotateUpDown", AXIS_RIGHT_STICK_Y);
    input_add_mapping_binding_key("Interact", SDLK_SPACE);
    input_add_mapping_binding_gamepad("Interact", SDL_GAMEPAD_BUTTON_SOUTH);
    input_add_mapping_binding_key("Sprint", SDLK_LSHIFT);
    input_add_mapping_binding_gamepad("Sprint", SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER);

    /// @note(ame): asset loading
    game_world world;
    game_world_load(&world, "assets/levels/corridor_0.json");
    uploader_ctx_flush();

    debug_camera camera;
    debug_camera_init(&camera);

    timer t;
    f32 last_frame = 0.0f;
    timer_init(&t);

    bool editor_mode = true;
    bool draw_mesh = true;
    bool draw_skeleton = false;
    bool vsync = true;

    console_var* draw_debug = cvar_get("mat_draw_debug");
    console_var* lit = cvar_get("mat_lit");

    /// @note(ame): main loop
    bool exit = false;
    while (!exit) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT)
                exit = true;
            ImGui_ImplSDL3_ProcessEvent(&event);
            input_update(&event);
        }

        if (input_is_key_pressed(SDLK_F1)) {
            editor_mode = !editor_mode;
        }

        // Calculate dt
        f32 time = timer_elasped(&t);
        f32 dt = (time - last_frame) / 1000.0f;
        last_frame = time;

        // update camera
        camera.width = WINDOW_WIDTH;
        camera.height = WINDOW_HEIGHT;

        // input camera
        ImGuiIO& io = ImGui::GetIO();

        glm::mat4 view_to_use = glm::mat4(1.0f);
        // update systems
        if (!editor_mode) {
            audio_update();
            physics_update();
            game_world_update(&world, dt);
            view_to_use = world.main_camera_view;
        } else {
            if (!io.WantCaptureMouse && editor_mode)
                debug_camera_update(&camera, dt);
            view_to_use = camera.view;
        }

        player_debug_draw(&world.player);

        // render world
        video_frame frame = video_begin();
        command_buffer_begin(frame.cmd_buffer);

        glm::vec3 player_pos = physics_character_get_position(&world.player.character);
        game_render_info render_info = {
            view_to_use, 
            camera.projection,
            player_pos + glm::vec3(0.0f, 0.0f, 0.5f),
            player_get_forward(&world.player),
            WINDOW_WIDTH,
            WINDOW_HEIGHT,
            &world,
            &frame,
            draw_mesh,
            draw_debug->as.b,
            lit->as.b
        };
        game_renderer_render(&render_info);

        // render ui
        command_buffer_begin_gui(frame.cmd_buffer, WINDOW_WIDTH, WINDOW_HEIGHT);
        command_buffer_image_barrier(frame.cmd_buffer, frame.backbuffer, LAYOUT_RENDER, 0);
        command_buffer_set_render_targets(frame.cmd_buffer, { frame.backbuffer_view }, nullptr);

        /// @note(ame): Panel
        if (editor_mode) {
            dev_console_draw(nullptr, nullptr);

            /// @note(ame): World panel
            ImGui::Begin("Renderer");
            if (ImGui::TreeNodeEx("Renderer", ImGuiTreeNodeFlags_Framed)) {
                ImGui::Checkbox("Draw Mesh", &draw_mesh);
                ImGui::Checkbox("Draw Skeleton", &draw_skeleton);
                ImGui::Checkbox("Draw Bounding Boxes", &draw_debug->as.b);
                ImGui::Checkbox("VSync", &vsync);
                ImGui::Checkbox("Lit", &lit->as.b);
                ImGui::TreePop();
            }
            ImGui::End();

            editor_manipulate(&world, &render_info);
        }

        command_buffer_end_gui(frame.cmd_buffer);

        /// @note(ame): Clear screen for loading screens
        for (auto& noti : noti_handler.notifications) {
            if (noti.type == NotificationType_LevelChange)
                command_buffer_clear_render_target(frame.cmd_buffer, { frame.backbuffer_view }, 0, 0, 0);
        }

        // finish
        command_buffer_image_barrier(frame.cmd_buffer, frame.backbuffer, LAYOUT_PRESENT);
        command_buffer_end(frame.cmd_buffer);
        video_end(&frame);
        video_present(vsync);

        // update input
        input_post_frame();

        /// @note(ame): reload shaders
        game_renderer_rebuild();

        /// @note(ame): reload levels
        if (!noti_handler.notifications.empty()) {
            for (auto& noti : noti_handler.notifications) {
                switch (noti.type) {
                    case NotificationType_LevelChange: {
                        audio_source_play(&audio.door_open);
                        
                        game_world temp;
                        game_world_load(&temp, noti.level_change.level_path);
                        game_world_free(&world);
                        uploader_ctx_flush();
                        world = temp;
                        
                        /// @note(ame): reset editor
                        editor_reset();
                        
                        audio_source_play(&audio.door_close);
                        
                        break;
                    }
                }
                noti_handler.notifications.clear();
            }
        }

        //
        discord_run_callbacks(editor_mode ? "DEBUG MODE" : "PLAY MODE", "On map " + world.name);
        //
    }

    video_wait();
    game_world_free(&world);
    
    dev_console_shutdown();
    input_exit();
    game_renderer_free();
    script_system_exit();
    physics_exit();
    audio_exit();
    resource_cache_free();
    video_exit();
    discord_exit();
    cvar_save("assets/cvars.json");
    steam_exit();

    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

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
#include "wn_gamepad.h"
#include "wn_resource_cache.h"
#include "wn_uploader.h"

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720

int main(void)
{
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK)) {
        throw_error("Failed to initialize SDL3!");
    }

    SDL_Window* window = SDL_CreateWindow("WHITE NOISE", WINDOW_WIDTH, WINDOW_HEIGHT, 0);
    if (!window) {
        throw_error("Failed to create SDL3 window");
    }

    /// @note(ame): initialize systems
    
    // steam_init();
    video_init(window);
    resource_cache_init();
    audio_init();
    physics_init();
    script_system_init();
    game_renderer_init(WINDOW_WIDTH, WINDOW_HEIGHT);
    gamepad_init();

    /// @note(ame): asset loading
    physics_trigger trigger;
    physics_trigger_init(&trigger, glm::vec3(1.5f, 0.765089f, 10.041f), glm::vec3(1.0f, 1.0f, 2.0f), &trigger);
    trigger.on_trigger_stay = [&](entity *e) {
        if (ImGui::IsKeyDown(ImGuiKey_Space))
            log("Player opened a door");
    };

    game_world_info info = {
        "assets/gltfs/road/untitled.gltf",
        glm::vec3(-1.0f, 1.25f, 0.0f)
    };

    game_world world;
    game_world_init(&world, &info);
    uploader_ctx_flush();

    debug_camera camera;
    debug_camera_init(&camera);

    timer t;
    f32 last_frame = 0.0f;
    timer_init(&t);

    bool draw_mesh = true;
    bool draw_debug = false;
    bool draw_skeleton = false;
    bool lit_scene = false;
    bool vsync = true;

    /// @note(ame): main loop
    bool exit = false;
    while (!exit) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT)
                exit = true;

            ImGui_ImplSDL3_ProcessEvent(&event);
            gamepad_update(&event);
        }

        // Calculate dt
        f32 time = timer_elasped(&t);
        f32 dt = (time - last_frame) / 1000.0f;
        last_frame = time;

        // update camera
        camera.width = WINDOW_WIDTH;
        camera.height = WINDOW_HEIGHT;
        debug_camera_update(&camera);

        // update audio
        audio_update();

        // update physics
        physics_update();

        // update world
        game_world_update(&world);

        // render world
        video_frame frame = video_begin();
        command_buffer_begin(frame.cmd_buffer);

        game_render_info render_info = {
            camera.view,
            camera.projection,
            physics_character_get_position(&world.player.character) + glm::vec3(0.0f, 0.0f, 0.3f),
            glm::vec3(0.0f, 0.0f, 1.0f),
            WINDOW_WIDTH,
            WINDOW_HEIGHT,
            &world,
            &frame,
            draw_mesh,
            draw_debug,
            lit_scene
        };
        game_renderer_render(&render_info);

        // render ui
        command_buffer_begin_gui(frame.cmd_buffer, WINDOW_WIDTH, WINDOW_HEIGHT);
        command_buffer_image_barrier(frame.cmd_buffer, frame.backbuffer, LAYOUT_RENDER, 0);
        command_buffer_set_render_targets(frame.cmd_buffer, { frame.backbuffer_view }, nullptr);
        ImGui::Begin("Animation");
        ImGui::Checkbox("Draw Mesh", &draw_mesh);
        ImGui::Checkbox("Draw Skeleton", &draw_skeleton);
        ImGui::Checkbox("Draw Debug", &draw_debug);
        ImGui::Checkbox("VSync", &vsync);
        ImGui::Checkbox("Lit", &lit_scene);
        ImGui::End();
        command_buffer_end_gui(frame.cmd_buffer);

        // finish
        command_buffer_image_barrier(frame.cmd_buffer, frame.backbuffer, LAYOUT_PRESENT);
        command_buffer_end(frame.cmd_buffer);
        video_end(&frame);
        video_present(vsync);

        // input camera
        ImGuiIO& io = ImGui::GetIO();
        if (!io.WantCaptureMouse)
            debug_camera_input(&camera, dt);

        // reload renderer if needed
        game_renderer_rebuild();
    }

    video_wait();
    game_world_free(&world);
    
    gamepad_exit();
    game_renderer_free();
    script_system_exit();
    physics_exit();
    audio_exit();
    video_exit();
    resource_cache_free();
    // steam_exit();

    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

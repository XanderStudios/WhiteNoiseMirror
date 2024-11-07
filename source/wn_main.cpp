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
#include <imguizmo/ImGuizmo.h>

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
#include "wn_notification.h"
#include "wn_util.h"

#define WINDOW_WIDTH 1600
#define WINDOW_HEIGHT 900

notification_handler noti_handler;

int main(void)
{
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK)) {
        throw_error("Failed to initialize SDL3!");
    }

    SDL_Window* window = SDL_CreateWindow("WHITE NOISE", WINDOW_WIDTH, WINDOW_HEIGHT, 0);
    if (!window) {
        throw_error("Failed to create SDL3 window");
    }

    /// @note(ame): pre-game launch
    bitmap_compress_recursive("assets/");

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
    bool draw_debug = false;
    bool draw_skeleton = false;
    bool lit_scene = false;
    bool vsync = true;

    bool select_player = false;
    entity* selected_trigger = nullptr;
    ImGuizmo::OPERATION operation = ImGuizmo::OPERATION::TRANSLATE;

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

        if (ImGui::IsKeyPressed(ImGuiKey_F1, false)) {
            editor_mode = !editor_mode;
        }

        // Calculate dt
        f32 time = timer_elasped(&t);
        f32 dt = (time - last_frame) / 1000.0f;
        last_frame = time;

        // update camera
        camera.width = WINDOW_WIDTH;
        camera.height = WINDOW_HEIGHT;
        debug_camera_update(&camera);

        // update systems
        if (!editor_mode) {
            audio_update();
            physics_update();
            game_world_update(&world);
        }

        // render world
        video_frame frame = video_begin();
        command_buffer_begin(frame.cmd_buffer);

        glm::vec3 player_pos = physics_character_get_position(&world.player.character);
        game_render_info render_info = {
            camera.view, 
            camera.projection,
            player_pos + glm::vec3(0.0f, 0.0f, 0.3f),
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

        /// @note(ame): Panel
        if (editor_mode) {
            /// @note(ame): World panel
            ImGui::Begin("World");
            if (ImGui::TreeNodeEx("Renderer", ImGuiTreeNodeFlags_Framed)) {
                ImGui::Checkbox("Draw Mesh", &draw_mesh);
                ImGui::Checkbox("Draw Skeleton", &draw_skeleton);
                ImGui::Checkbox("Draw Debug", &draw_debug);
                ImGui::Checkbox("VSync", &vsync);
                ImGui::Checkbox("Lit", &lit_scene);
                ImGui::TreePop();
            }
            if (ImGui::TreeNodeEx("Scene", ImGuiTreeNodeFlags_Framed)) {
                ImGui::Checkbox("Edit Player", &select_player);
                if (ImGui::Button("Save Start Position")) {
                    world.start_position = player_pos;
                }
                ImGui::Separator();
                if (ImGui::RadioButton("Translate", operation == ImGuizmo::OPERATION::TRANSLATE))
                    operation = ImGuizmo::OPERATION::TRANSLATE;
                ImGui::SameLine();
                if (ImGui::RadioButton("Rotate", operation == ImGuizmo::OPERATION::ROTATE))
                    operation = ImGuizmo::OPERATION::ROTATE;
                if (ImGui::TreeNodeEx("Triggers", ImGuiTreeNodeFlags_Framed)) {
                    if (ImGui::Button("New Trigger")) {
                        selected_trigger = game_world_add_trigger(&world, glm::vec3(0.0f), glm::vec3(1.0f));
                    }
                    if (selected_trigger) {
                        if (ImGui::Button("Remove Selected")) {
                            game_world_remove_entity(&world, selected_trigger);
                            selected_trigger = nullptr;
                        }
                    }
                    for (auto& entity : world.entities) {
                        if (entity->type == EntityType_Trigger) {
                            std::string name = "Trigger " + std::to_string(entity->trigger_id);
                            if (ImGui::Selectable(name.c_str())) {
                                selected_trigger = entity;
                            }
                        }
                    }
                    ImGui::TreePop();
                }

                ImGui::Separator();
                if (ImGui::Button("Save")) {
                    game_world_save(&world);
                }
                ImGui::TreePop();
            }
            if (ImGui::TreeNodeEx("World Info", ImGuiTreeNodeFlags_Framed)) {
                ImGui::Text("Player Position: (%.3f, %.3f, %.3f)", player_pos.x, player_pos.y, player_pos.z);
                ImGui::TreePop();
            }
            ImGui::End();

            /// @todo(ame): Entity panel
            ImGui::Begin("Selected Object");
            if (selected_trigger) {
                ImGui::Text("TRIGGER:");
                
                static const char* trigger_types[] = { "Not Precised", "Transition" };
                ImGui::Combo("Trigger Type", (int*)&selected_trigger->t_type, trigger_types, 2, 2);
            
                if (selected_trigger->t_type == TriggerType_Transition) {
                    char buffer[512];
                    strcpy(buffer, selected_trigger->trigger_transition.c_str());
                    ImGui::InputText("Level Path", buffer, 512);
                    std::string wrapped_buffer = std::string(buffer);
                    if (fs_exists(wrapped_buffer)) {
                        selected_trigger->trigger_transition = std::string(buffer);
                    } else {
                        ImGui::TextColored(ImVec4(1, 0, 0, 1), "%s doesn't exist", buffer);
                    }
                }
            }
            ImGui::End();

            /// @note(ame): Guizmo context
            ImGuiIO& io = ImGui::GetIO();
            ImGui::SetNextWindowPos({0, 0});
            ImGui::SetNextWindowSize({f32(WINDOW_WIDTH), f32(WINDOW_HEIGHT)});
            ImGui::Begin("ImGuizmo Context", nullptr, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoInputs);

            ImGuizmo::SetOrthographic(false);
            ImGuizmo::SetDrawlist();
            ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);

            if (select_player) {
                glm::mat4 matrix(1.0f);
                glm::vec3 rot(0.0f);
                glm::vec3 scale(1.0f);

                ImGuizmo::RecomposeMatrixFromComponents(glm::value_ptr(player_pos),
                                                        glm::value_ptr(rot),
                                                        glm::value_ptr(scale),
                                                        glm::value_ptr(matrix));

                ImGuizmo::Manipulate(glm::value_ptr(render_info.current_view),
                                     glm::value_ptr(render_info.current_projection),
                                     ImGuizmo::OPERATION::TRANSLATE,
                                     ImGuizmo::MODE::WORLD,
                                     glm::value_ptr(matrix),
                                     nullptr,
                                     nullptr);

                ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(matrix),
                                                      glm::value_ptr(player_pos),
                                                      glm::value_ptr(rot),
                                                      glm::value_ptr(scale));

                physics_character_set_position(&world.player.character, player_pos);
            }

            if (selected_trigger) {
                glm::mat4 matrix(1.0f);
                glm::vec3 scale(1.0f);

                glm::vec3 trigger_position = physics_trigger_get_position(&selected_trigger->trigger);
                glm::vec3 trigger_rotation = physics_trigger_get_rotation(&selected_trigger->trigger);

                /*
                    Jolt gives euler angles in radians
                    Convert it to euler degrees to send it for recomposition for ImGuizmo
                    Get the euler degrees back
                    Convert the euler degrees back to radians
                    Convert the euler radians back to a quaternion to send to Jolt
                */

                trigger_rotation = glm::degrees(trigger_rotation);
                ImGuizmo::RecomposeMatrixFromComponents(glm::value_ptr(trigger_position),
                                                        glm::value_ptr(trigger_rotation),
                                                        glm::value_ptr(scale),
                                                        glm::value_ptr(matrix));

                ImGuizmo::Manipulate(glm::value_ptr(render_info.current_view),
                                     glm::value_ptr(render_info.current_projection),
                                     operation,
                                     ImGuizmo::MODE::WORLD,
                                     glm::value_ptr(matrix),
                                     nullptr,
                                     nullptr);

                ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(matrix),
                                                      glm::value_ptr(trigger_position),
                                                      glm::value_ptr(trigger_rotation),
                                                      glm::value_ptr(scale));
                trigger_rotation = glm::radians(trigger_rotation);

                physics_trigger_set_position(&selected_trigger->trigger, trigger_position);
                physics_trigger_set_rotation(&selected_trigger->trigger, trigger_rotation);
            }

            ImGui::End();
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

        // input camera
        ImGuiIO& io = ImGui::GetIO();
        if (!io.WantCaptureMouse)
            debug_camera_input(&camera, dt);

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
                        selected_trigger = nullptr;
                        
                        audio_source_play(&audio.door_close);
                        
                        break;
                    }
                }
                noti_handler.notifications.clear();
            }
        }
    }

    video_wait();
    game_world_free(&world);
    
    gamepad_exit();
    game_renderer_free();
    script_system_exit();
    physics_exit();
    audio_exit();
    resource_cache_free();
    video_exit();
    // steam_exit();

    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-11-08 02:16:37
//

#include <imgui_impl_sdl3.h>
#include <imgui.h>
#include <imguizmo/ImGuizmo.h>

#include "wn_editor.h"

struct editor_data
{
    bool select_player;
    ImGuizmo::OPERATION operation;
    entity* selected_trigger;
};

editor_data editor;

void editor_manipulate(game_world *world, game_render_info *info)
{
    glm::vec3 player_pos = physics_character_get_position(&world->player.character);

    ImGui::Begin("World");
    if (ImGui::TreeNodeEx("Scene", ImGuiTreeNodeFlags_Framed)) {
        ImGui::Checkbox("Edit Player", &editor.select_player);
        if (ImGui::Button("Save Start Position")) {
            world->start_position = player_pos;
        }
        ImGui::Separator();

        if (ImGui::RadioButton("Translate", editor.operation == ImGuizmo::OPERATION::TRANSLATE))
            editor.operation = ImGuizmo::OPERATION::TRANSLATE;
        ImGui::SameLine();
        if (ImGui::RadioButton("Rotate", editor.operation == ImGuizmo::OPERATION::ROTATE))
            editor.operation = ImGuizmo::OPERATION::ROTATE;

        if (ImGui::TreeNodeEx("Triggers", ImGuiTreeNodeFlags_Framed)) {
            if (ImGui::Button("New Trigger")) {
                editor.selected_trigger = game_world_add_trigger(world, glm::vec3(0.0f), glm::vec3(1.0f));
            }
            if (editor.selected_trigger) {
                if (ImGui::Button("Unselect")) {
                    editor.selected_trigger = nullptr;
                }
                if (ImGui::Button("Remove Selected")) {
                    game_world_remove_entity(world, editor.selected_trigger);
                    editor.selected_trigger = nullptr;
                }
            }
            for (auto& entity : world->entities) {
                if (entity->type == EntityType_Trigger) {
                    std::string name = "Trigger " + std::to_string(entity->trigger_id);
                    if (ImGui::Selectable(name.c_str())) {
                        editor.selected_trigger = entity;
                    }
                }
            }
            ImGui::TreePop();
        }
        ImGui::Separator();
        if (ImGui::Button("Save")) {
            game_world_save(world);
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
    if (editor.selected_trigger) {
        ImGui::Text("TRIGGER:");
        
        static const char* trigger_types[] = { "Not Precised", "Transition", "Camera" };
        ImGui::Combo("Trigger Type", (int*)&editor.selected_trigger->t_type, trigger_types, 3, 3);
    
        if (editor.selected_trigger->t_type == TriggerType_Transition) {
            char buffer[512];
            strcpy(buffer, editor.selected_trigger->trigger_transition.c_str());
            ImGui::InputText("Level Path", buffer, 512);
            std::string wrapped_buffer = std::string(buffer);
            if (fs_exists(wrapped_buffer)) {
                editor.selected_trigger->trigger_transition = std::string(buffer);
            } else {
                ImGui::TextColored(ImVec4(1, 0, 0, 1), "%s doesn't exist", buffer);
            }
        }
    }
    ImGui::End();

    /// @note(ame): Guizmo context
    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos({0, 0});
    ImGui::SetNextWindowSize({f32(info->width), f32(info->height)});
    ImGui::Begin("ImGuizmo Context", nullptr, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoInputs);

    ImGuizmo::SetOrthographic(false);
    ImGuizmo::SetDrawlist();
    ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);
    
    if (editor.select_player) {
        glm::mat4 matrix(1.0f);
        glm::vec3 rot(0.0f);
        glm::vec3 scale(1.0f);

        ImGuizmo::RecomposeMatrixFromComponents(glm::value_ptr(player_pos),
                                                glm::value_ptr(rot),
                                                glm::value_ptr(scale),
                                                glm::value_ptr(matrix));
        
        ImGuizmo::Manipulate(glm::value_ptr(info->current_view),
                             glm::value_ptr(info->current_projection),
                             ImGuizmo::OPERATION::TRANSLATE,
                             ImGuizmo::WORLD,
                             glm::value_ptr(matrix),
                             nullptr,
                             nullptr);
        
        ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(matrix),
                                              glm::value_ptr(player_pos),
                                              glm::value_ptr(rot),
                                              glm::value_ptr(scale));
        physics_character_set_position(&world->player.character, player_pos);
    }

    if (editor.selected_trigger) {
        glm::mat4 matrix(1.0f);
        glm::vec3 scale(1.0f);
        glm::vec3 trigger_position = physics_trigger_get_position(&editor.selected_trigger->trigger);
        glm::vec3 trigger_rotation = physics_trigger_get_rotation(&editor.selected_trigger->trigger);
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

        ImGuizmo::Manipulate(glm::value_ptr(info->current_view),
                             glm::value_ptr(info->current_projection),
                             editor.operation,
                             ImGuizmo::WORLD,
                             glm::value_ptr(matrix),
                             nullptr,
                             nullptr);

        ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(matrix),
                                              glm::value_ptr(trigger_position),
                                              glm::value_ptr(trigger_rotation),
                                              glm::value_ptr(scale));
        trigger_rotation = glm::radians(trigger_rotation);

        physics_trigger_set_position(&editor.selected_trigger->trigger, trigger_position);
        physics_trigger_set_rotation(&editor.selected_trigger->trigger, trigger_rotation);
    }
    ImGui::End();
}

void editor_reset()
{
    editor.selected_trigger = nullptr;
}

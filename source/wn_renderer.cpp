//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-11-02 16:41:48
//

#include <functional>

#include "wn_renderer.h"

game_renderer renderer;

/// FORWARD
void forward_init(u32 width, u32 height);
void forward_rebuild();
void forward_render(game_render_info *info);
void forward_free();
///

/// COMPOSITE
void composite_init(u32 width, u32 height);
void composite_rebuild();
void composite_render(game_render_info *info);
void composite_free();
///

/// DEBUG
void debug_renderer_render(game_render_info *info);
///

void game_renderer_init(u32 width, u32 height)
{
    forward_init(width, height);
    composite_init(width, height);
    renderer.debug = new debug_renderer();
    video_wait();

    log("[renderer] Initialized renderer");
}

void game_renderer_rebuild()
{
    forward_rebuild();
    composite_rebuild();
}

void game_renderer_render(game_render_info *info)
{
    renderer.info = info;

    forward_render(info);
    composite_render(info);
    debug_renderer_render(info);
}

void game_renderer_free()
{
    composite_free();
    forward_free();
    renderer.debug->free_resources();
    delete renderer.debug;
}

///------------ COMPOSITE PASS ------------///
void composite_init(u32 width, u32 height)
{
    renderer.composite.pipeline.entries = { RootSignatureEntry_SRV, RootSignatureEntry_UAV };
    renderer.composite.pipeline.push_constant_size = 0;
    hot_pipeline_add_shader(&renderer.composite.pipeline, "shaders/composite/composite.hlsl", ShaderType_Compute);
    hot_pipeline_build(&renderer.composite.pipeline);

    texture_init(&renderer.composite.ldr_buffer, width, height, DXGI_FORMAT_R8G8B8A8_UNORM, TEXTURE_UAV);
}

void composite_rebuild()
{
    hot_pipeline_rebuild(&renderer.composite.pipeline);
}

void composite_render(game_render_info *info)
{
    video_frame* frame = info->frame;

    /// @note(ame): dispatch shader
    command_buffer_image_barrier(frame->cmd_buffer, &renderer.forward.color_buffer, LAYOUT_SHADER);
    command_buffer_image_barrier(frame->cmd_buffer, &renderer.composite.ldr_buffer, LAYOUT_STORAGE);
    command_buffer_set_compute_pipeline(frame->cmd_buffer, &renderer.composite.pipeline.compute);
    command_buffer_set_compute_srv(frame->cmd_buffer, &tvc_get_entry(&renderer.forward.color_buffer, TextureViewType_ShaderResource)->view, 0);
    command_buffer_set_compute_uav(frame->cmd_buffer, &tvc_get_entry(&renderer.composite.ldr_buffer, TextureViewType_Storage, 0)->view, 1);
    command_buffer_dispatch(frame->cmd_buffer, info->width / 8, info->height / 8, 1);
    command_buffer_image_barrier(frame->cmd_buffer, &renderer.forward.color_buffer, LAYOUT_COMMON);

    /// @note(ame): Copy to backbuffer
    command_buffer_image_barrier(frame->cmd_buffer, &renderer.composite.ldr_buffer, LAYOUT_COPYSRC);
    command_buffer_image_barrier(frame->cmd_buffer, frame->backbuffer, LAYOUT_COPYDST);
    command_buffer_copy_texture_to_texture(frame->cmd_buffer, frame->backbuffer, &renderer.composite.ldr_buffer);
    command_buffer_image_barrier(frame->cmd_buffer, frame->backbuffer, LAYOUT_COMMON);
    command_buffer_image_barrier(frame->cmd_buffer, &renderer.composite.ldr_buffer, LAYOUT_COMMON);
}

void composite_free()
{
    texture_free(&renderer.composite.ldr_buffer);
    hot_pipeline_free(&renderer.composite.pipeline);
}

///------------ FORWARD PASS ------------///

void forward_init(u32 width, u32 height)
{
    texture_init(&renderer.forward.white_texture, 1, 1, DXGI_FORMAT_R8G8B8A8_UNORM);
    texture_init(&renderer.forward.depth_buffer, width, height, DXGI_FORMAT_D32_FLOAT, TEXTURE_DSV);
    texture_init(&renderer.forward.color_buffer, width, height, DXGI_FORMAT_R32G32B32A32_FLOAT, TEXTURE_RTV);

    sampler_init(&renderer.forward.texture_sampler, SamplerAddress_Wrap, SamplerFilter_Linear);

    renderer.forward.pipeline.entries = { RootSignatureEntry_PushConstants, RootSignatureEntry_CBV, RootSignatureEntry_CBV, RootSignatureEntry_SRV, RootSignatureEntry_Sampler };
    renderer.forward.pipeline.push_constant_size = sizeof(glm::mat4) * 2 + sizeof(glm::vec4);
    renderer.forward.pipeline.specs.depth = true;
    renderer.forward.pipeline.specs.depth_format = DXGI_FORMAT_D32_FLOAT;
    renderer.forward.pipeline.specs.op = DepthOp_Less;
    renderer.forward.pipeline.specs.wireframe = false;
    renderer.forward.pipeline.specs.formats = { DXGI_FORMAT_R32G32B32A32_FLOAT };

    hot_pipeline_add_shader(&renderer.forward.pipeline, "shaders/forward/tri_vert.hlsl", ShaderType_Vertex);
    hot_pipeline_add_shader(&renderer.forward.pipeline, "shaders/forward/tri_frag.hlsl", ShaderType_Pixel);
    hot_pipeline_build(&renderer.forward.pipeline);

    for (i32 i = 0; i < FRAMES_IN_FLIGHT; i++) {
        buffer_init(&renderer.forward.flashlight_buffer[i], 256, 0, BufferType_Constant);
        buffer_build_constant(&renderer.forward.flashlight_buffer[i]);
    }

    uint32_t col = 0xFFFFFFFF;

    void *data;
    buffer staging;
    buffer_init(&staging, texture_get_size(&renderer.forward.white_texture), 0, BufferType_Copy);
    buffer_map(&staging, 0, 0, &data);
    memset(data, col, texture_get_size(&renderer.forward.white_texture));
    buffer_unmap(&staging);

    command_buffer cmd;
    command_buffer_init(&cmd, D3D12_COMMAND_LIST_TYPE_DIRECT, false);
    command_buffer_begin(&cmd, false);
    command_buffer_copy_buffer_to_texture(&cmd, &renderer.forward.white_texture, &staging);
    command_buffer_end(&cmd);
    command_queue_submit(&video.graphics_queue, { &cmd });
    video_wait();
    buffer_free(&staging);
}

void forward_rebuild()
{
    hot_pipeline_rebuild(&renderer.forward.pipeline);
}

void forward_render(game_render_info *info)
{
    struct temp_data {
        glm::mat4 view;
        glm::mat4 proj;
        glm::vec4 pos;
    };
    temp_data push_const = {
        info->current_view,
        info->current_projection,
        glm::vec4(info->current_position, 1.0f)
    };
    
    video_frame* frame = info->frame;
    auto& depth = renderer.forward.depth_buffer;
    auto& dsv = tvc_get_entry(&renderer.forward.depth_buffer, TextureViewType_DepthTarget)->view;

    /// @note(ame): upload flashlight data
    struct flashlight_data {
        glm::vec3 pos;
        f32 pad;

        glm::vec3 forward;
        f32 cutoff;

        f32 outer_cutoff;
        bool lit;
        glm::vec2 pad0;
    };
    flashlight_data flashlight = {
        info->current_position,
        0.0f,

        info->current_forward,
        glm::cos(glm::radians(20.5f)),

        glm::cos(glm::radians(25.5f)),
        info->lit_scene,
        glm::vec2(0.0f)
    };

    void *flashlight_mapped;
    buffer_map(&renderer.forward.flashlight_buffer[frame->frame_index], 0, 0, &flashlight_mapped);
    memcpy(flashlight_mapped, &flashlight, sizeof(flashlight));
    buffer_unmap(&renderer.forward.flashlight_buffer[frame->frame_index]);

    command_buffer_image_barrier(frame->cmd_buffer, &renderer.forward.color_buffer, LAYOUT_RENDER);
    command_buffer_image_barrier(frame->cmd_buffer, &depth, LAYOUT_DEPTH);
    command_buffer_set_render_targets(frame->cmd_buffer, { &tvc_get_entry(&renderer.forward.color_buffer, TextureViewType_RenderTarget)->view }, &dsv);
    command_buffer_clear_render_target(frame->cmd_buffer, &tvc_get_entry(&renderer.forward.color_buffer, TextureViewType_RenderTarget)->view, 0.0f, 0.0f, 0.0f);
    command_buffer_clear_depth_target(frame->cmd_buffer, &dsv);
    command_buffer_viewport(frame->cmd_buffer, info->width, info->height);
    command_buffer_set_topology(frame->cmd_buffer, GeomTopology_Triangles);
    command_buffer_set_graphics_pipeline(frame->cmd_buffer, &renderer.forward.pipeline.graphics);
    command_buffer_set_graphics_push_constants(frame->cmd_buffer, &push_const, sizeof(push_const), 0);
    command_buffer_set_graphics_cbv(frame->cmd_buffer, &renderer.forward.flashlight_buffer[frame->frame_index], 2);
    command_buffer_set_graphics_sampler(frame->cmd_buffer, &renderer.forward.texture_sampler, 4);
    std::function<void(video_frame* frame, gltf_node*, gltf_model* model, glm::mat4 transform)> draw_node =
        [&](video_frame* frame, gltf_node *node, gltf_model* model, glm::mat4 transform) {
            if (!node->children.empty()) {
                for (gltf_node* child : node->children) {
                    draw_node(frame, child, model, transform);
                }
            }

            for (auto& primitive : node->primitives) {
                gltf_material& mat = model->materials[primitive.material_index];
                texture_view* alb_view = mat.has_albedo
                                       ? &tvc_get_entry(&mat.albedo->handle->tex, TextureViewType_ShaderResource, TEXTURE_ALL_MIPS)->view
                                       : &tvc_get_entry(&renderer.forward.white_texture, TextureViewType_ShaderResource)->view;
                
                glm::mat4 global_transform = transform * node->transform * (node->parent ? node->parent->transform : glm::mat4(1.0f));

                void *data;
                buffer_map(&node->model_buffer[frame->frame_index], 0, 0, &data);
                memcpy(data, &global_transform, sizeof(glm::mat4));
                buffer_unmap(&node->model_buffer[frame->frame_index]);
                
                command_buffer_set_graphics_cbv(frame->cmd_buffer, &node->model_buffer[frame->frame_index], 1);
                command_buffer_set_vertex_buffer(frame->cmd_buffer, &primitive.vertex_buffer);
                command_buffer_set_index_buffer(frame->cmd_buffer, &primitive.index_buffer);
                command_buffer_set_graphics_srv(frame->cmd_buffer, alb_view, 3);
                command_buffer_draw_indexed(frame->cmd_buffer, primitive.idx_count);
            }
    };
    if (info->render_meshes) {
        draw_node(frame, info->world->player.model->model.root, &info->world->player.model->model, physics_character_get_transform(&info->world->player.character));
        draw_node(frame, info->world->level->model.root, &info->world->level->model, glm::mat4(1.0f));
    }
}

void forward_free()
{
    for (i32 i = 0; i < FRAMES_IN_FLIGHT; i++) {
        buffer_free(&renderer.forward.flashlight_buffer[i]);
    }

    texture_free(&renderer.forward.color_buffer);
    texture_free(&renderer.forward.white_texture);
    texture_free(&renderer.forward.depth_buffer);
    sampler_free(&renderer.forward.texture_sampler);
   
    hot_pipeline_free(&renderer.forward.pipeline);
}

///------------ DEBUG PASS ------------///

void debug_renderer_render(game_render_info *info)
{
    if (info->render_debug) {
        physics_draw();
        renderer.debug->flush(info->frame, info->current_view, info->current_projection);
    }
}

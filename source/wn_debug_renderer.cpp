//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-10-31 16:31:42
//

#include "wn_debug_renderer.h"

debug_renderer::debug_renderer()
{
    compiled_shader vert = shader_compile("shaders/debug/line_vert.hlsl", ShaderType_Vertex);
    compiled_shader frag = shader_compile("shaders/debug/line_frag.hlsl", ShaderType_Pixel);

    root_signature_init(&line_signature, { RootSignatureEntry_PushConstants }, sizeof(glm::mat4) * 2);

    pipeline_desc desc;
    desc.depth = false;
    desc.depth_format = DXGI_FORMAT_UNKNOWN;
    desc.formats = { DXGI_FORMAT_R8G8B8A8_UNORM };
    desc.line = true;
    desc.shaders[ShaderType_Vertex] = vert;
    desc.shaders[ShaderType_Pixel] = frag;
    desc.wireframe = false;
    desc.signature = &line_signature;
    graphics_pipeline_init(&line_pipeline, &desc);

    for (i32 i = 0; i < FRAMES_IN_FLIGHT; i++) {
        buffer_init(&line_transfer[i], MAX_LINES * sizeof(debug_line_vertex), 0, BufferType_Constant);
        buffer_init(&line_vtx[i], MAX_LINES * sizeof(debug_line_vertex), sizeof(debug_line_vertex), BufferType_Vertex);
    }

    Initialize();
}

void debug_renderer::free_resources()
{
    for (i32 i = 0; i < FRAMES_IN_FLIGHT; i++) {
        buffer_free(&line_vtx[i]);
        buffer_free(&line_transfer[i]);
    }
    root_signature_free(&line_signature);
    graphics_pipeline_free(&line_pipeline);
}

void debug_renderer::flush(video_frame* f, glm::mat4 view, glm::mat4 proj)
{
    if (!lines.empty()) {
        std::vector<debug_line_vertex> vertices;
        for (auto& line : lines) {
            vertices.push_back({ line.A, line.Color });
            vertices.push_back({ line.B, line.Color });
        }

        void *data;
        buffer_map(&line_transfer[f->frame_index], 0, 0, &data);
        memcpy(data, vertices.data(), vertices.size() * sizeof(debug_line_vertex));
        buffer_unmap(&line_transfer[f->frame_index]);
        
        glm::mat4 matrices[2] = {
            view,
            proj
        };

        command_buffer_buffer_barrier(f->cmd_buffer, &line_vtx[f->frame_index], D3D12_RESOURCE_STATE_COPY_DEST);
        command_buffer_buffer_barrier(f->cmd_buffer, &line_transfer[f->frame_index], D3D12_RESOURCE_STATE_COPY_SOURCE);
        command_buffer_copy_buffer_to_buffer(f->cmd_buffer, &line_vtx[f->frame_index], &line_transfer[f->frame_index]);
        command_buffer_buffer_barrier(f->cmd_buffer, &line_vtx[f->frame_index], D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
        command_buffer_buffer_barrier(f->cmd_buffer, &line_transfer[f->frame_index], D3D12_RESOURCE_STATE_COMMON);
        
        command_buffer_image_barrier(f->cmd_buffer, f->backbuffer, LAYOUT_RENDER, 0);
        command_buffer_set_render_targets(f->cmd_buffer, { f->backbuffer_view }, nullptr);
        command_buffer_viewport(f->cmd_buffer, f->backbuffer->width, f->backbuffer->height);
        command_buffer_set_graphics_pipeline(f->cmd_buffer, &line_pipeline);
        command_buffer_set_vertex_buffer(f->cmd_buffer, &line_vtx[f->frame_index]);
        command_buffer_set_topology(f->cmd_buffer, GeomTopology_Lines);
        command_buffer_set_graphics_push_constants(f->cmd_buffer, matrices, sizeof(matrices), 0);
        command_buffer_draw(f->cmd_buffer, vertices.size());

        lines.clear();
    }
}

/// @note(ame): overrides
void debug_renderer::DrawLine(JPH::RVec3Arg inFrom, JPH::RVec3Arg inTo, JPH::ColorArg inColor)
{
    JPH::Vec4 color = inColor.ToVec4();

    if (lines.size() <= MAX_LINES) {
        lines.push_back({
            glm::vec3(inFrom.GetX(), inFrom.GetY(), inFrom.GetZ()),
            glm::vec3(inTo.GetX(), inTo.GetY(), inTo.GetZ()),
            glm::vec3(color.GetX(), color.GetY(), color.GetZ())
        });
    }
}

void debug_draw_line(glm::vec3 p0, glm::vec3 p1, glm::vec3 color)
{
    JPH::DebugRenderer::sInstance->DrawArrow(JPH::Vec3(p0.x, p0.y, p0.z), JPH::Vec3(p1.x, p1.y, p1.z), JPH::Color(color.x * 255, color.y * 255, color.z * 255), 0.05f);
}

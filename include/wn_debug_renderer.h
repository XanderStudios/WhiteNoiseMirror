//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-10-31 16:10:27
//

#pragma once

#define JPH_DEBUG_RENDERER
#include <jolt/Jolt.h>
#include <jolt/Renderer/DebugRenderer.h>
#include <jolt/Renderer/DebugRendererSimple.h>

#include <glm/glm.hpp>

#include <string_view>
#include <atomic>
#include <memory>
#include <vector>
#include <array>

#include "wn_common.h"
#include "wn_video.h"
#include "wn_d3d12.h"
#include "wn_debug_camera.h"

struct debug_line
{
    glm::vec3 A;
    glm::vec3 B;
    glm::vec3 Color;
};

struct debug_line_vertex
{
    glm::vec3 position;
    glm::vec3 color;
};

constexpr u32 MAX_LINES = 5192 * 8;

/// @note(ame): Forced to use OOP by the Jolt gods
struct debug_renderer : public JPH::DebugRendererSimple
{
    debug_renderer();
    ~debug_renderer() = default;

    /// @note(ame): methods
    void free_resources();
    void flush(video_frame* f, glm::mat4 view, glm::mat4 proj);

    /// @note(ame): Internals
    graphics_pipeline line_pipeline;
    root_signature line_signature;

    /// @note(ame): Batch implementation
    struct BatchImpl
    {
        bool draw_indexed;
        buffer vertex_buffer;
        buffer index_buffer;

        std::atomic<u32> ref_count;

        ~BatchImpl()
        {
            buffer_free(&vertex_buffer);
            buffer_free(&index_buffer);
        }
    };

    std::vector<debug_line> lines;
    std::array<buffer, FRAMES_IN_FLIGHT> line_transfer;
    std::array<buffer, FRAMES_IN_FLIGHT> line_vtx;
    std::vector<BatchImpl> batches;

    /// @note(ame): Override members
    void DrawLine(JPH::RVec3Arg inFrom, JPH::RVec3Arg inTo, JPH::ColorArg inColor) override;
    void DrawText3D(JPH::RVec3Arg inPosition, const std::string_view &inString, JPH::ColorArg inColor = JPH::Color::sWhite, float inHeight = 0.5f) override {}
};

void debug_draw_line(glm::vec3 p0, glm::vec3 p1, glm::vec3 color = glm::vec3(1.0f));

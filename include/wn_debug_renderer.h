//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-10-31 16:10:27
//

#pragma once

#define JPH_DEBUG_RENDERER
#include <jolt/Jolt.h>
#include <jolt/Renderer/DebugRenderer.h>

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

constexpr u32 MAX_LINES = 5192 * 4;

/// @note(ame): Forced to use OOP by the Jolt gods
struct debug_renderer : public JPH::DebugRenderer
{
    debug_renderer();
    ~debug_renderer() = default;

    /// @note(ame): methods
    void free_resources();
    void flush(video_frame* f, glm::mat4 view, glm::mat4 proj);

    /// @note(ame): Internals
    graphics_pipeline line_pipeline;
    root_signature line_signature;

    std::vector<debug_line> lines;
    std::array<buffer, FRAMES_IN_FLIGHT> line_transfer;
    std::array<buffer, FRAMES_IN_FLIGHT> line_vtx;

    /// @note(ame): Batch implementation
    struct BatchImpl : public JPH::RefTargetVirtual
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

        void AddRef() override { ++ref_count; }
        void Release() override	{ if (--ref_count == 0) delete this; }
    };

    /// @note(ame): Override members
    void DrawLine(JPH::RVec3Arg inFrom, JPH::RVec3Arg inTo, JPH::ColorArg inColor) override;
    void DrawTriangle(JPH::RVec3Arg inV1, JPH::RVec3Arg inV2, JPH::RVec3Arg inV3, JPH::ColorArg inColor, ECastShadow inCastShadow = ECastShadow::Off) override;
    Batch CreateTriangleBatch(const Triangle *inTriangles, int inTriangleCount) override;
    Batch CreateTriangleBatch(const Vertex *inVertices, i32 inVertexCount, const u32 *inIndices, i32 inIndexCount) override;
    void DrawGeometry(JPH::RMat44Arg inModelMatrix, const JPH::AABox &inWorldSpaceBounds, f32 inLODScaleSq, JPH::ColorArg inModelColor, const GeometryRef &inGeometry, ECullMode inCullMode = ECullMode::CullBackFace, ECastShadow inCastShadow = ECastShadow::On, EDrawMode inDrawMode = EDrawMode::Solid) override;
    void DrawText3D(JPH::RVec3Arg inPosition, const std::string_view &inString, JPH::ColorArg inColor = JPH::Color::sWhite, float inHeight = 0.5f);
};

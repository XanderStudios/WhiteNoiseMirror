//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-10-26 16:42:05
//

#pragma once

#include <vector>
#include <string>

#include "wn_common.h"

enum shader_type
{
    ShaderType_Vertex,
    ShaderType_Pixel,
    ShaderType_Compute
    /// @todo(ame): Mesh, amplification, raytracing?
};

struct shader_header
{
    shader_type type;
    u32 low_time;
    u32 high_time;
    u32 size;
};

struct compiled_shader
{
    bool errors = false;
    std::vector<uint8_t> bytes;
};

compiled_shader shader_compile(const std::string& path, shader_type type);

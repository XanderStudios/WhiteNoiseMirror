//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-10-26 12:52:38
//

#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <SDL3/SDL.h>

#include "wn_common.h"
#include "wn_d3d12.h"

/*
VIDEO TODOLIST:
- Descriptor heap
- Swap chain
- Fences for sync
*/

struct video_device
{
    IDXGIFactory7 *factory;
    IDXGIAdapter1 *adapter;
    IDXGIDevice *dxgi;
    ID3D12Device *device;
    ID3D12Debug1 *debug;
    ID3D12DebugDevice *debug_device;

    descriptor_heap rtv_heap;
    descriptor_heap dsv_heap;
    descriptor_heap shader_heap;
    descriptor_heap sampler_heap;

    command_queue graphics_queue;
    /// @todo(ame): compute queue?
    /// @todo(ame): copy queue?

    swapchain swap;

    /// @note(ame): sync
    fence graphics_fence;
    u64 frame_values[FRAMES_IN_FLIGHT];
    u32 frame_index;

    /// @note(ame): per frame
    command_buffer buffers[FRAMES_IN_FLIGHT];

    /// @note(ame): other
    descriptor font_descriptor;
};

struct video_frame
{
    command_buffer *cmd_buffer;
    texture *backbuffer;
    texture_view *backbuffer_view;
    u32 frame_index;
};

extern video_device video;

void video_init(SDL_Window* window, bool debug = true);
void video_resize(u32 width, u32 height);
void video_wait();

video_frame video_begin();
void video_end(video_frame *frame);
void video_present(bool vsync);

void video_exit();

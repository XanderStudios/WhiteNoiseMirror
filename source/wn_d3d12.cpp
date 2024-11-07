//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-10-26 13:52:08
//

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_dx12.h>
#include <d3d12shader.h>
#include <dxcapi.h>

#include <locale>
#include <codecvt>

#include "wn_d3d12.h"
#include "wn_video.h"
#include "wn_output.h"
#include "wn_util.h"

/// @note(ame): globals
texture_view_cache tvc;
gpu_resource_tracker gpu_tracker;

/// @note(ame): descriptor

void descriptor_heap_init(descriptor_heap *heap, D3D12_DESCRIPTOR_HEAP_TYPE type, u32 size)
{
    heap->descriptors.resize(size, false);
    heap->type = type;
    heap->heap_size = size;
    heap->shader_visible = false;

    D3D12_DESCRIPTOR_HEAP_DESC pipeline_desc = {};
    pipeline_desc.Type = type;
    pipeline_desc.NumDescriptors = size;
    pipeline_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    if (type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV || type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER) {
        pipeline_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        heap->shader_visible = true;
    }

    heap->increment_size = video.device->GetDescriptorHandleIncrementSize(type);

    HRESULT result = video.device->CreateDescriptorHeap(&pipeline_desc, IID_PPV_ARGS(&heap->heap));
    if (FAILED(result)) {
        throw_error("Failed to create descriptor heap!");
    }
}

descriptor descriptor_heap_alloc(descriptor_heap *heap)
{
    i32 index = -1;

    for (i32 i = 0; i < heap->heap_size; i++) {
        if (heap->descriptors[i] == false) {
            heap->descriptors[i] = true;
            index = i;
            break;
        }
    }

    descriptor pipeline_desc;
    pipeline_desc.parent = heap;
    pipeline_desc.heap_index = index;
    pipeline_desc.valid = true;
    pipeline_desc.cpu = heap->heap->GetCPUDescriptorHandleForHeapStart();
    pipeline_desc.cpu.ptr += index * heap->increment_size;
    if (heap->shader_visible) {
        pipeline_desc.gpu = heap->heap->GetGPUDescriptorHandleForHeapStart();
        pipeline_desc.gpu.ptr += index * heap->increment_size;
    }
    return pipeline_desc;
}

void descriptor_heap_free(descriptor_heap *heap)
{
    SAFE_RELEASE(heap->heap);
}

void descriptor_free(descriptor *descriptor)
{
    if (!descriptor->valid)
        return;
    descriptor->parent->descriptors[descriptor->heap_index] = false;
    descriptor->valid = false;
    descriptor->parent = nullptr;
}

/// @note(ame): command queue

void command_queue_init(command_queue *queue, D3D12_COMMAND_LIST_TYPE type)
{
    D3D12_COMMAND_QUEUE_DESC pipeline_desc = {};
    pipeline_desc.Type = type;

    HRESULT result = video.device->CreateCommandQueue(&pipeline_desc, IID_PPV_ARGS(&queue->queue));
    if (FAILED(result))
        throw_error("Failed to create D3D12 command queue");
}

void command_queue_wait(command_queue *queue, fence *fence, u64 value)
{
    queue->queue->Wait(fence->fence, value);
}

void command_queue_signal(command_queue *queue, fence *fence, u64 value)
{
    queue->queue->Signal(fence->fence, value);
}

void command_queue_free(command_queue *queue)
{
    SAFE_RELEASE(queue->queue);
}

void command_queue_submit(command_queue *queue, const std::vector<command_buffer*> buffers)
{
    std::vector<ID3D12CommandList*> lists;
    for (auto& buffer : buffers) {
        lists.push_back(buffer->list);
    }

    queue->queue->ExecuteCommandLists(lists.size(), lists.data());
}

/// @note(ame): swapchain

void swapchain_init(swapchain *swap, HWND window)
{
    swap->hwnd = window;

    RECT client_rect;
    GetClientRect(window, &client_rect);
    swap->width = client_rect.right - client_rect.left;
    swap->height = client_rect.bottom - client_rect.top;

    DXGI_SWAP_CHAIN_DESC1 pipeline_desc = {};
    pipeline_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    pipeline_desc.SampleDesc.Count = 1;
    pipeline_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    pipeline_desc.BufferCount = FRAMES_IN_FLIGHT;
    pipeline_desc.Scaling = DXGI_SCALING_NONE;
    pipeline_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    pipeline_desc.Width = swap->width;
    pipeline_desc.Height = swap->height;

    IDXGISwapChain1 *temp;
    HRESULT result = video.factory->CreateSwapChainForHwnd(video.graphics_queue.queue, window, &pipeline_desc, nullptr, nullptr, &temp);
    if (FAILED(result)) {
        throw_error("Failed to create swap chain!");
    }
    temp->QueryInterface(&swap->swapchain);
    temp->Release();

    swapchain_resize(swap, swap->width, swap->height);
}

void swapchain_resize(swapchain *swap, u32 width, u32 height)
{
    for (int i = 0; i < FRAMES_IN_FLIGHT; i++) {
        SAFE_RELEASE(swap->resources[i]);
        descriptor_free(&swap->descriptors[i]); /// @note(ame): if the descriptor doesn't exist, nothing will happen so it's fine
    }

    swap->swapchain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);

    for (int i = 0; i < FRAMES_IN_FLIGHT; i++) {
        HRESULT result = swap->swapchain->GetBuffer(i, IID_PPV_ARGS(&swap->resources[i]));
        if (FAILED(result))
            throw_error("Failed to get swapchain buffer");

        swap->descriptors[i] = descriptor_heap_alloc(&video.rtv_heap);
        video.device->CreateRenderTargetView(swap->resources[i], nullptr, swap->descriptors[i].cpu);

        swap->views[i].handle = swap->descriptors[i];
        swap->views[i].type = TextureViewType_RenderTarget;

        swap->wrappers[i].width = width;
        swap->wrappers[i].height = height;
        swap->wrappers[i].format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swap->wrappers[i].levels = 1;
        swap->wrappers[i].state = D3D12_RESOURCE_STATE_COMMON;
        swap->wrappers[i].resource.resource = swap->resources[i];
    }
}

void swapchain_present(swapchain *swap, bool vsync)
{
    swap->swapchain->Present(vsync ? true == 1 : false, 0);
}

void swapchain_free(swapchain *swap)
{
    for (int i = 0; i < FRAMES_IN_FLIGHT; i++) {
        SAFE_RELEASE(swap->resources[i]);
        descriptor_free(&swap->descriptors[i]);
    }
    SAFE_RELEASE(swap->swapchain);
}

/// @note(ame): fence

void fence_init(fence *fence)
{
    fence->value = 0;
    HRESULT result = video.device->CreateFence(fence->value, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence->fence));
    if (FAILED(result)) {
        throw_error("Failed to create fence");
    }
}

void fence_free(fence *fence)
{
    SAFE_RELEASE(fence->fence);
}

u64 fence_signal(fence *fence, command_queue *queue)
{
    fence->value++;
    queue->queue->Signal(fence->fence, fence->value);
    return fence->value;
}

void fence_wait(fence *fence, u64 value)
{
    if (fence_completed_value(fence) < value) {
        HANDLE event = CreateEvent(nullptr, false, false, nullptr);
        fence->fence->SetEventOnCompletion(value, event);
        if (WaitForSingleObject(event, 10'000'000) == WAIT_TIMEOUT) {
            throw_error("D3D12 Device Lost");
        }
    }
}

u64 fence_completed_value(fence *fence)
{
    return fence->fence->GetCompletedValue();
}

/// @note(ame): command buffer

void command_buffer_init(command_buffer *buf, D3D12_COMMAND_LIST_TYPE type, bool close)
{
    HRESULT result = video.device->CreateCommandAllocator(type, IID_PPV_ARGS(&buf->allocator));
    if (FAILED(result))
        throw_error("Failed to create command allocator");
    
    result = video.device->CreateCommandList(0, type, buf->allocator, nullptr, IID_PPV_ARGS(&buf->list));
    if (FAILED(result)) {
        throw_error("Failed to create command list");
    }

    if (close) {
        buf->list->Close();
    }
}

void command_buffer_begin(command_buffer *buf, bool reset)
{
    if (reset) {
        buf->allocator->Reset();
        buf->list->Reset(buf->allocator, nullptr);
    }

    ID3D12DescriptorHeap* heaps[] = {
        video.shader_heap.heap,
        video.sampler_heap.heap
    };
    buf->list->SetDescriptorHeaps(2, heaps);
}

void command_buffer_end(command_buffer *buf)
{
    buf->list->Close();
}

void command_buffer_free(command_buffer *buf)
{
    SAFE_RELEASE(buf->list);
    SAFE_RELEASE(buf->allocator);
}

void command_buffer_set_render_targets(command_buffer *buf, const std::vector<texture_view*>& views, texture_view *depth)
{
    std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> rtvs;
    D3D12_CPU_DESCRIPTOR_HANDLE* dsv = nullptr;

    for (auto& rt : views) {
        rtvs.push_back(rt->handle.cpu);
    }
    if (depth) {
        dsv = &depth->handle.cpu;
    }

    buf->list->OMSetRenderTargets(rtvs.size(), rtvs.data(), false, dsv);
}

void command_buffer_buffer_barrier(command_buffer *buf, buffer* b, D3D12_RESOURCE_STATES state)
{
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = b->resource;
    barrier.Transition.StateBefore = b->state;
    barrier.Transition.StateAfter = state;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    
    if (barrier.Transition.StateBefore == D3D12_RESOURCE_STATE_UNORDERED_ACCESS && barrier.Transition.StateAfter == D3D12_RESOURCE_STATE_UNORDERED_ACCESS) {
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
        barrier.UAV.pResource = b->resource;
    } else {
        if (barrier.Transition.StateBefore == barrier.Transition.StateAfter)
            return;
    }
    
    buf->list->ResourceBarrier(1, &barrier);
    b->state = state;
}

void command_buffer_image_barrier(command_buffer *buf, texture *tex, D3D12_RESOURCE_STATES state, i32 mip)
{
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = tex->resource;
    barrier.Transition.StateBefore = tex->state;
    barrier.Transition.StateAfter = state;
    barrier.Transition.Subresource = mip == TEXTURE_ALL_MIPS ? D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES : mip;
    
    if (barrier.Transition.StateBefore == D3D12_RESOURCE_STATE_UNORDERED_ACCESS && barrier.Transition.StateAfter == D3D12_RESOURCE_STATE_UNORDERED_ACCESS) {
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
        barrier.UAV.pResource = tex->resource;
    } else {
        if (barrier.Transition.StateBefore == barrier.Transition.StateAfter)
            return;
    }
    
    buf->list->ResourceBarrier(1, &barrier);
    tex->state = state;
}

void command_buffer_clear_render_target(command_buffer *buf, texture_view *view, f32 r, f32 g, f32 b)
{
    f32 values[] = { r, g, b, 1.0f };
    buf->list->ClearRenderTargetView(view->handle.cpu, values, 0, nullptr);
}

void command_buffer_clear_depth_target(command_buffer *buf, texture_view *view)
{
    buf->list->ClearDepthStencilView(view->handle.cpu, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
}

void command_buffer_set_graphics_pipeline(command_buffer *buf, graphics_pipeline *pipeline)
{
    buf->list->SetGraphicsRootSignature(pipeline->signature->signature);
    buf->list->SetPipelineState(pipeline->state);
}

void command_buffer_set_vertex_buffer(command_buffer *buf, buffer *v)
{
    buf->list->IASetVertexBuffers(0, 1, &v->vbv);
}

void command_buffer_set_index_buffer(command_buffer *buf, buffer *i)
{
    buf->list->IASetIndexBuffer(&i->ibv);
}

void command_buffer_set_graphics_srv(command_buffer *buf, texture_view *view, i32 index)
{
    buf->list->SetGraphicsRootDescriptorTable(index, view->handle.gpu);
}

void command_buffer_set_graphics_cbv(command_buffer *buf, buffer *view, i32 index)
{
    buf->list->SetGraphicsRootDescriptorTable(index, view->cbv.gpu);
}

void command_buffer_set_graphics_sampler(command_buffer *buf, sampler *s, i32 index)
{
    buf->list->SetGraphicsRootDescriptorTable(index, s->handle.gpu);
}

void command_buffer_set_graphics_push_constants(command_buffer *buf, const void *data, u64 size, i32 index)
{
    buf->list->SetGraphicsRoot32BitConstants(index, size / 4, data, 0);
}

void command_buffer_set_compute_pipeline(command_buffer *buf, compute_pipeline *pipeline)
{
    buf->list->SetComputeRootSignature(pipeline->signature->signature);
    buf->list->SetPipelineState(pipeline->state);
}

void command_buffer_set_compute_srv(command_buffer *buf, texture_view *view, i32 index)
{
    buf->list->SetComputeRootDescriptorTable(index, view->handle.gpu);
}

void command_buffer_set_compute_cbv(command_buffer *buf, buffer *view, i32 index)
{
    buf->list->SetComputeRootDescriptorTable(index, view->cbv.gpu);
}

void command_buffer_set_compute_uav(command_buffer *buf, texture_view *view, i32 index)
{
    buf->list->SetComputeRootDescriptorTable(index, view->handle.gpu);
}

void command_buffer_set_compute_sampler(command_buffer *buf, sampler *s, i32 index)
{
    buf->list->SetComputeRootDescriptorTable(index, s->handle.gpu);
}

void command_buffer_set_compute_push_constants(command_buffer *buf, const void *data, u64 size, i32 index)
{
    buf->list->SetComputeRoot32BitConstants(index, size / 4, data, 0);
}

void command_buffer_viewport(command_buffer *buf, u32 width, u32 height)
{
    D3D12_VIEWPORT viewport;
    viewport.Width = width;
    viewport.Height = height;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    viewport.TopLeftX = 0.0f;
    viewport.TopLeftY = 1.0f;

    D3D12_RECT rect;
    rect.right = width;
    rect.bottom = height;
    rect.top = 0.0f;
    rect.left = 0.0f;

    buf->list->RSSetViewports(1, &viewport);
    buf->list->RSSetScissorRects(1, &rect);
}

void command_buffer_set_topology(command_buffer *buf, geom_topology top)
{
    buf->list->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY(top));
}

void command_buffer_draw(command_buffer *buf, u32 count)
{
    buf->list->DrawInstanced(count, 1, 0, 0);
}

void command_buffer_draw_indexed(command_buffer *buf, u32 count)
{
    buf->list->DrawIndexedInstanced(count, 1, 0, 0, 0);
}

void command_buffer_dispatch(command_buffer *buf, u32 x, u32 y, u32 z)
{
    buf->list->Dispatch(x, y, z);
}

void command_buffer_begin_gui(command_buffer *buf, u32 width, u32 height)
{
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize.x = width;
    io.DisplaySize.y = height;

    ImGui_ImplDX12_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
}

void command_buffer_end_gui(command_buffer *buf)
{
    ImGuiIO& io = ImGui::GetIO();

    ID3D12DescriptorHeap* heaps[] = { video.shader_heap.heap, video.sampler_heap.heap };
    buf->list->SetDescriptorHeaps(2, heaps);

    ImGui::Render();
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), buf->list);

    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }
}

void command_buffer_copy_texture_to_texture(command_buffer *buf, texture *dst, texture *src)
{
    buf->list->CopyResource(dst->resource, src->resource);
}

/// Courtesy to Dihara Wijetunga's Wolfenstein PT
void command_buffer_copy_buffer_to_texture(command_buffer *buf, texture *dst, buffer *src, u32 mip_count)
{
    D3D12_RESOURCE_DESC desc = dst->resource.resource->GetDesc();

    std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> footprints(dst->levels);
    std::vector<u32> num_rows(dst->levels);
    std::vector<u64> row_sizes(dst->levels);
    u64 total_size = 0;

    video.device->GetCopyableFootprints(&desc, 0, dst->levels, 0, footprints.data(), num_rows.data(), row_sizes.data(), &total_size);

    for (uint32_t i = 0; i < dst->levels; i++) {
        D3D12_TEXTURE_COPY_LOCATION src_copy = {};
        src_copy.pResource = src->resource;
        src_copy.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        src_copy.PlacedFootprint = footprints[i];

        D3D12_TEXTURE_COPY_LOCATION dst_copy = {};
        dst_copy.pResource = dst->resource;
        dst_copy.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        dst_copy.SubresourceIndex = i;

        buf->list->CopyTextureRegion(&dst_copy, 0, 0, 0, &src_copy, nullptr);
    }
}

void command_buffer_copy_buffer_to_buffer(command_buffer *buf, buffer *dst, buffer *src)
{
    buf->list->CopyResource(dst->resource, src->resource);
}

/// @note(ame): texture

void texture_init(texture *tex, u32 width, u32 height, DXGI_FORMAT format, u32 flags, u32 levels, bool copy, const std::string& name)
{
    tex->uuid = wn_uuid();
    tex->width = width;
    tex->height = height;
    tex->levels = levels;
    tex->format = format;

    D3D12_HEAP_PROPERTIES properties = {};
    properties.Type = copy ? D3D12_HEAP_TYPE_UPLOAD : D3D12_HEAP_TYPE_DEFAULT;

    D3D12_RESOURCE_DESC pipeline_desc = {};
    pipeline_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    pipeline_desc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    pipeline_desc.Width = width;
    pipeline_desc.Height = height;
    pipeline_desc.DepthOrArraySize = 1;
    pipeline_desc.Format = DXGI_FORMAT(format);
    pipeline_desc.SampleDesc.Count = 1;
    pipeline_desc.SampleDesc.Quality = 0;
    pipeline_desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    pipeline_desc.Flags = D3D12_RESOURCE_FLAGS(flags);
    pipeline_desc.MipLevels = levels;

    gpu_resource_alloc(&tex->resource, &pipeline_desc, &properties, D3D12_RESOURCE_STATE_COMMON, name);
}

void texture_free(texture *tex)
{
    gpu_resource_free(&tex->resource);
}

u64 texture_get_size(texture *tex, u32 mip)
{
    D3D12_RESOURCE_DESC desc = tex->resource.resource->GetDesc();
    
    u64 val = 0;
    if (mip == TEXTURE_ALL_MIPS) {
        video.device->GetCopyableFootprints(&desc, 0, tex->levels, 0, nullptr, nullptr, nullptr, &val);
    } else {
        video.device->GetCopyableFootprints(&desc, mip, 1, 0, nullptr, nullptr, nullptr, &val);
    }
    return val;
}

/// @note(ame): texture view

void texture_view_init(texture_view *view, texture *tex, texture_view_type type, u32 mip)
{
    view->parent_texture = tex;
    view->type = type;

    switch (type) {
        case TextureViewType_RenderTarget: {
            view->handle = descriptor_heap_alloc(&video.rtv_heap);

            D3D12_RENDER_TARGET_VIEW_DESC pipeline_desc = {};
            pipeline_desc.Format = tex->format;
            pipeline_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
            video.device->CreateRenderTargetView(tex->resource, &pipeline_desc, view->handle.cpu);
            break;
        };
        case TextureViewType_DepthTarget: {
            view->handle = descriptor_heap_alloc(&video.dsv_heap);

            D3D12_DEPTH_STENCIL_VIEW_DESC pipeline_desc = {};
            pipeline_desc.Format = tex->format;
            pipeline_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
            video.device->CreateDepthStencilView(tex->resource, &pipeline_desc, view->handle.cpu);
            break;
        };
        case TextureViewType_ShaderResource: {
            view->handle = descriptor_heap_alloc(&video.shader_heap);

            D3D12_SHADER_RESOURCE_VIEW_DESC pipeline_desc = {};
            pipeline_desc.Format = tex->format;
            pipeline_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            pipeline_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            if (mip == TEXTURE_ALL_MIPS) {
                pipeline_desc.Texture2D.MipLevels = tex->levels;
                pipeline_desc.Texture2D.MostDetailedMip = 0;
            } else {
                pipeline_desc.Texture2D.MipLevels = 1;
                pipeline_desc.Texture2D.MostDetailedMip = mip;
            }
            video.device->CreateShaderResourceView(tex->resource, &pipeline_desc, view->handle.cpu);
            break;
        };
        case TextureViewType_Storage: {
            view->handle = descriptor_heap_alloc(&video.shader_heap);

            D3D12_UNORDERED_ACCESS_VIEW_DESC pipeline_desc = {};
            pipeline_desc.Format = tex->format;
            pipeline_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
            if (mip == TEXTURE_ALL_MIPS) {
                pipeline_desc.Texture2D.MipSlice = 0;
            } else {
                pipeline_desc.Texture2D.MipSlice = mip;
            }
            video.device->CreateUnorderedAccessView(tex->resource, nullptr, &pipeline_desc, view->handle.cpu);
            break;
        };
    }
}

void texture_view_free(texture_view *view)
{
    /// @note(ame): will automatically free the descriptor from the right heap
    descriptor_free(&view->handle);
}

/// @note(ame): pipelines

void graphics_pipeline_init(graphics_pipeline *pipeline, pipeline_desc *desc)
{
    compiled_shader vert = desc->shaders[ShaderType_Vertex];
    compiled_shader pixel = desc->shaders[ShaderType_Pixel];

    D3D12_SHADER_DESC vertex_desc = {};
    ID3D12ShaderReflection* vertex_reflection = shader_get_reflection(&vert, &vertex_desc);

    D3D12_GRAPHICS_PIPELINE_STATE_DESC pipeline_desc = {};
    pipeline_desc.VS.pShaderBytecode = vert.bytes.data();
    pipeline_desc.VS.BytecodeLength = vert.bytes.size();
    pipeline_desc.PS.pShaderBytecode = pixel.bytes.data();
    pipeline_desc.PS.BytecodeLength = pixel.bytes.size();
    for (int RTVIndex = 0; RTVIndex < desc->formats.size(); RTVIndex++) {
        pipeline_desc.BlendState.RenderTarget[RTVIndex].SrcBlend = D3D12_BLEND_ONE;
        pipeline_desc.BlendState.RenderTarget[RTVIndex].DestBlend = D3D12_BLEND_ZERO;
        pipeline_desc.BlendState.RenderTarget[RTVIndex].BlendOp = D3D12_BLEND_OP_ADD;
        pipeline_desc.BlendState.RenderTarget[RTVIndex].SrcBlendAlpha = D3D12_BLEND_ONE;
        pipeline_desc.BlendState.RenderTarget[RTVIndex].DestBlendAlpha = D3D12_BLEND_ZERO;
        pipeline_desc.BlendState.RenderTarget[RTVIndex].BlendOpAlpha = D3D12_BLEND_OP_ADD;
        pipeline_desc.BlendState.RenderTarget[RTVIndex].LogicOp = D3D12_LOGIC_OP_NOOP;
        pipeline_desc.BlendState.RenderTarget[RTVIndex].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

        pipeline_desc.RTVFormats[RTVIndex] = DXGI_FORMAT(desc->formats[RTVIndex]);
    }
    pipeline_desc.NumRenderTargets = desc->formats.size();
    pipeline_desc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
    pipeline_desc.RasterizerState.FillMode = desc->wireframe ? D3D12_FILL_MODE_WIREFRAME : D3D12_FILL_MODE_SOLID;
    pipeline_desc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    pipeline_desc.RasterizerState.FrontCounterClockwise = false;
    pipeline_desc.PrimitiveTopologyType = desc->line ? D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE : D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pipeline_desc.SampleDesc.Count = 1;
    if (desc->depth) {
        pipeline_desc.DepthStencilState.DepthEnable = true;
        pipeline_desc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
        pipeline_desc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC(desc->op);
        pipeline_desc.DSVFormat = DXGI_FORMAT(desc->depth_format);
    }
    if (desc->signature) {
        pipeline_desc.pRootSignature = desc->signature->signature;
        pipeline->signature = desc->signature;
    }

    std::vector<D3D12_INPUT_ELEMENT_DESC> input_element_descs;
    std::vector<std::string> input_element_semantic_names;

    input_element_descs.reserve(vertex_desc.InputParameters);
    input_element_semantic_names.reserve(vertex_desc.InputParameters);

    for (i32 i = 0; i < vertex_desc.InputParameters; i++) {
        D3D12_SIGNATURE_PARAMETER_DESC parameter_desc = {};
        vertex_reflection->GetInputParameterDesc(i, &parameter_desc);

        input_element_semantic_names.push_back(parameter_desc.SemanticName);

        D3D12_INPUT_ELEMENT_DESC input_element = {};
        input_element.SemanticName = input_element_semantic_names.back().c_str();
        input_element.SemanticIndex = parameter_desc.SemanticIndex;
        input_element.InputSlot = 0;
        input_element.AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
        input_element.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
        input_element.InstanceDataStepRate = 0;

        if (parameter_desc.Mask == 1) {
            if (parameter_desc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) input_element.Format = DXGI_FORMAT_R32_UINT;
            else if (parameter_desc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) input_element.Format = DXGI_FORMAT_R32_SINT;
            else if (parameter_desc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) input_element.Format = DXGI_FORMAT_R32_FLOAT;
        } else if (parameter_desc.Mask <= 3) {
            if (parameter_desc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) input_element.Format = DXGI_FORMAT_R32G32_UINT;
            else if (parameter_desc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) input_element.Format = DXGI_FORMAT_R32G32_SINT;
            else if (parameter_desc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) input_element.Format = DXGI_FORMAT_R32G32_FLOAT;
        } else if (parameter_desc.Mask <= 7) {
            if (parameter_desc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) input_element.Format = DXGI_FORMAT_R32G32B32_UINT;
            else if (parameter_desc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) input_element.Format = DXGI_FORMAT_R32G32B32_SINT;
            else if (parameter_desc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) input_element.Format = DXGI_FORMAT_R32G32B32_FLOAT;
        } else if (parameter_desc.Mask <= 15) {
            if (parameter_desc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) input_element.Format = DXGI_FORMAT_R32G32B32A32_UINT;
            else if (parameter_desc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) input_element.Format = DXGI_FORMAT_R32G32B32A32_SINT;
            else if (parameter_desc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) input_element.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        }

        input_element_descs.push_back(input_element);
    }
    pipeline_desc.InputLayout.pInputElementDescs = input_element_descs.data();
    pipeline_desc.InputLayout.NumElements = static_cast<uint32_t>(input_element_descs.size());

    HRESULT result = video.device->CreateGraphicsPipelineState(&pipeline_desc, IID_PPV_ARGS(&pipeline->state));
    if (FAILED(result))
        throw_error("Failed to create graphics pipeline");
}

void graphics_pipeline_free(graphics_pipeline *pipeline)
{
    SAFE_RELEASE(pipeline->state);
}

/// @note(ame): compute shaders

void compute_pipeline_init(compute_pipeline *pipeline, compiled_shader *shader, root_signature *signature)
{
    pipeline->signature = signature;

    D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {};
    desc.CS.BytecodeLength = shader->bytes.size();
    desc.CS.pShaderBytecode = shader->bytes.data();
    desc.pRootSignature = signature->signature;

    HRESULT result = video.device->CreateComputePipelineState(&desc, IID_PPV_ARGS(&pipeline->state));
    if (FAILED(result))
        throw_error("Failed to create compute pipeline");
}

void compute_pipeline_free(compute_pipeline *pipeline)
{
    SAFE_RELEASE(pipeline->state);
}

/// @note(ame): buffers

void buffer_init(buffer *buf, u64 size, u64 stride, buffer_type type, bool readback, const std::string& name)
{
    buf->size = size;
    buf->stride = stride;
    buf->type = type;

    D3D12_HEAP_PROPERTIES properties = {};
    switch (type) {
        case BufferType_Copy:
        case BufferType_Constant:
            properties.Type = D3D12_HEAP_TYPE_UPLOAD;
            break;
        default:
            if (readback)
                properties.Type = D3D12_HEAP_TYPE_READBACK;
            else
                properties.Type = D3D12_HEAP_TYPE_DEFAULT;
    }

    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    desc.Width = size;
    desc.Height = 1;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.Format = DXGI_FORMAT_UNKNOWN;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    desc.Flags = D3D12_RESOURCE_FLAG_NONE;
    if (type == BufferType_Storage) {
        desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    }

    switch (type) {
        case BufferType_Constant: {
            buf->state = D3D12_RESOURCE_STATE_GENERIC_READ;
            break;
        }
        case BufferType_AccelerationStructure: {
            buf->state = D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
            break;
        }
        default: {
            buf->state = D3D12_RESOURCE_STATE_COMMON;
            break;
        }
    }

    gpu_resource_alloc(&buf->resource, &desc, &properties, buf->state, name);

    switch (type) {
        case BufferType_Vertex: {
            buf->vbv.BufferLocation = buf->resource.resource->GetGPUVirtualAddress();
            buf->vbv.SizeInBytes = size;
            buf->vbv.StrideInBytes = stride;
            break;
        }
        case BufferType_Index: {
            buf->ibv.BufferLocation = buf->resource.resource->GetGPUVirtualAddress();
            buf->ibv.SizeInBytes = size;
            buf->ibv.Format = DXGI_FORMAT_R32_UINT;
            break;
        }
    }
}

void buffer_build_constant(buffer *buf)
{
    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvd = {};
    cbvd.BufferLocation = buf->resource.resource->GetGPUVirtualAddress();
    cbvd.SizeInBytes = buf->size;
    if (buf->cbv.valid == false)
        buf->cbv = descriptor_heap_alloc(&video.shader_heap);
    video.device->CreateConstantBufferView(&cbvd, buf->cbv.cpu);
}

void buffer_build_storage(buffer *buf)
{
    D3D12_UNORDERED_ACCESS_VIEW_DESC uavd = {};
    uavd.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    uavd.Format = DXGI_FORMAT_UNKNOWN;
    uavd.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
    uavd.Buffer.FirstElement = 0;
    uavd.Buffer.NumElements = buf->size;
    uavd.Buffer.StructureByteStride = 1;
    uavd.Buffer.CounterOffsetInBytes = 0;
    if (buf->uav.valid == false)
        buf->uav = descriptor_heap_alloc(&video.shader_heap);
    video.device->CreateUnorderedAccessView(buf->resource, nullptr, &uavd, buf->uav.cpu);
}

void buffer_build_shader_resource(buffer *buf)
{
    D3D12_SHADER_RESOURCE_VIEW_DESC srvd = {};
    srvd.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    srvd.Format = DXGI_FORMAT_UNKNOWN;
    srvd.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvd.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
    srvd.Buffer.FirstElement = 0;
    srvd.Buffer.NumElements = buf->size / buf->stride;
    srvd.Buffer.StructureByteStride = buf->stride;
    if (buf->srv.valid == false)
        buf->srv = descriptor_heap_alloc(&video.shader_heap);
    video.device->CreateShaderResourceView(buf->resource, &srvd, buf->srv.cpu);
}

void buffer_map(buffer *buf, i32 start, i32 end, void **data)
{
    D3D12_RANGE range;
    range.Begin = start;
    range.End = end;

    HRESULT result = 0;
    if (range.Begin != range.End) {
        result = buf->resource.resource->Map(0, &range, data);
    } else {
        result = buf->resource.resource->Map(0, nullptr, data);
    }
    if (FAILED(result)) {
        throw_error("Failed to map D3D12 buffer!");
    }
}

void buffer_unmap(buffer *buf)
{
    buf->resource.resource->Unmap(0, nullptr);
}

void buffer_free(buffer *buf)
{
    descriptor_free(&buf->cbv);
    descriptor_free(&buf->srv);
    descriptor_free(&buf->uav);
    gpu_resource_free(&buf->resource);
}

/// @note(ame): root signature

void root_signature_init(root_signature *signature, const std::vector<root_signature_entry>& entries, u64 push_constant_size, bool bindless)
{
    std::vector<D3D12_ROOT_PARAMETER> parameters(entries.size());
    std::vector<D3D12_DESCRIPTOR_RANGE> ranges(entries.size());
    
    for (int i = 0; i < entries.size(); i++) {
        D3D12_DESCRIPTOR_RANGE& descriptor_range = ranges[i];
        descriptor_range.NumDescriptors = 1;
        descriptor_range.BaseShaderRegister = i;
        descriptor_range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE(entries[i]);
        descriptor_range.RegisterSpace = 0;
        
        D3D12_ROOT_PARAMETER& parameter = parameters[i];
        if (entries[i] == RootSignatureEntry_PushConstants) {
            parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
            parameter.Constants.ShaderRegister = i;
            parameter.Constants.RegisterSpace = 0;
            parameter.Constants.Num32BitValues = push_constant_size / 4;
        } else {
            parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            parameter.DescriptorTable.NumDescriptorRanges = 1;
            parameter.DescriptorTable.pDescriptorRanges = &descriptor_range;
            parameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        }
    }

    D3D12_ROOT_SIGNATURE_DESC root_signature_desc = {};
    root_signature_desc.NumParameters = parameters.size();
    root_signature_desc.pParameters = parameters.data();
    root_signature_desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    if (bindless) {
        root_signature_desc.Flags |= D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED | D3D12_ROOT_SIGNATURE_FLAG_SAMPLER_HEAP_DIRECTLY_INDEXED;
    }

    ID3DBlob* root_signature_blob;
    ID3DBlob* error_blob;
    D3D12SerializeRootSignature(&root_signature_desc, D3D_ROOT_SIGNATURE_VERSION_1_0, &root_signature_blob, &error_blob);
    if (error_blob) {
        log("D3D12 Root Signature error! %s", error_blob->GetBufferPointer());
        throw_error("Failed to create root signature");
    }
    HRESULT Result = video.device->CreateRootSignature(0, root_signature_blob->GetBufferPointer(), root_signature_blob->GetBufferSize(), IID_PPV_ARGS(&signature->signature));
    if (FAILED(Result)) {
        throw_error("Failed to create root signature!");
    }
    root_signature_blob->Release();
}

void root_signature_free(root_signature *signature)
{
    SAFE_RELEASE(signature->signature);
}

ID3D12ShaderReflection* shader_get_reflection(compiled_shader *shader, D3D12_SHADER_DESC *desc)
{
    IDxcUtils* utils;
    DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&utils));

    DxcBuffer shader_buffer = {};
    shader_buffer.Ptr = shader->bytes.data();
    shader_buffer.Size = shader->bytes.size();

    ID3D12ShaderReflection *reflection;
    HRESULT result = utils->CreateReflection(&shader_buffer, IID_PPV_ARGS(&reflection));
    if (FAILED(result)) {
        throw_error("Failed to get shader reflection!");
    }
    reflection->GetDesc(desc);
    utils->Release();
    return reflection;
}

void sampler_init(sampler *s, sampler_address address, sampler_filter filter, bool mips)
{
    s->handle = descriptor_heap_alloc(&video.sampler_heap);

    D3D12_SAMPLER_DESC desc = {};
    desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE(address);
    desc.AddressV = desc.AddressU;
    desc.AddressW = desc.AddressV;
    desc.Filter = D3D12_FILTER(filter);
    desc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    desc.MinLOD = 0.0f;
    if (mips) {
        desc.MaxLOD = D3D12_FLOAT32_MAX;
    }
    desc.MipLODBias = 0.0f;
    desc.BorderColor[0] = 1.0f;
    desc.BorderColor[1] = 1.0f;
    desc.BorderColor[2] = 1.0f;
    desc.BorderColor[3] = 1.0f;

    video.device->CreateSampler(&desc, s->handle.cpu);
}

void sampler_free(sampler *s)
{
    descriptor_free(&s->handle);
}

void hot_pipeline_add_shader(hot_pipeline* pipeline, const std::string& path, shader_type type)
{
    hot_pipeline::shader_watch watch;
    watch.path = path;
    file_watch_start(&watch.watch, path);
    watch.shader = shader_compile(path, type);

    pipeline->watches[type] = watch;
}

void hot_pipeline_build(hot_pipeline* pipeline)
{
    if (pipeline->sig.signature == nullptr) {
        root_signature_init(&pipeline->sig, pipeline->entries, pipeline->push_constant_size);
    }

    if (pipeline->watches.count(ShaderType_Compute) == 1) {
        compute_pipeline_init(&pipeline->compute, &pipeline->watches[ShaderType_Compute].shader, &pipeline->sig);
    } else {
        pipeline->specs.signature = &pipeline->sig;
        pipeline->specs.shaders[ShaderType_Vertex] = pipeline->watches[ShaderType_Vertex].shader;
        pipeline->specs.shaders[ShaderType_Pixel] = pipeline->watches[ShaderType_Pixel].shader;

        graphics_pipeline_init(&pipeline->graphics, &pipeline->specs);
    }
}

void hot_pipeline_rebuild(hot_pipeline* pipeline)
{
    if (pipeline->watches.count(ShaderType_Compute) == 1) {
        if (file_watch_check(&pipeline->watches[ShaderType_Compute].watch)) {
            compiled_shader shader = shader_compile(pipeline->watches[ShaderType_Compute].path, ShaderType_Compute);
            if (!shader.errors) {
                compute_pipeline_free(&pipeline->compute);
                pipeline->watches[ShaderType_Compute].shader = shader;
                hot_pipeline_build(pipeline);
            }
        }
    } else {
        for (auto& watch : pipeline->watches) {
            if (file_watch_check(&watch.second.watch)) {
                compiled_shader shader = shader_compile(watch.second.path, watch.first);
                if (!shader.errors) {
                    watch.second.shader = shader;
                    graphics_pipeline_free(&pipeline->graphics);
                    hot_pipeline_build(pipeline);
                }
            }
        }
    }
}

void hot_pipeline_free(hot_pipeline* pipeline)
{
    graphics_pipeline_free(&pipeline->graphics);
    compute_pipeline_free(&pipeline->compute);
    root_signature_free(&pipeline->sig);
    pipeline->watches.clear();
}

tvc_entry *tvc_get_entry(texture *t, texture_view_type view, u32 mip)
{
    if (tvc.entries.count(t->uuid) > 0) {
        for (auto& entry : tvc.entries[t->uuid]) {
            if (entry->type == view && entry->mip == mip) {
                return entry;
            }
        }
    }
    
    tvc_entry* entry = new tvc_entry;
    entry->parent = t;
    entry->type = view;
    entry->mip = mip;
    entry->ref_count++;

    texture_view_init(&entry->view, entry->parent, view, mip);
    tvc.entries[t->uuid].push_back(entry);
    return entry;
}

void tvc_release_view(tvc_entry *entry)
{
    entry->ref_count--;
    if (entry->ref_count == 0) {
        texture_view_free(&entry->view);
        tvc.entries[entry->parent->uuid].erase(std::find(tvc.entries[entry->parent->uuid].begin(), tvc.entries[entry->parent->uuid].end(), entry));
        delete entry;
    }
}

/// @note(ame): resource tracker

void gpu_resource_alloc(gpu_resource *res, D3D12_RESOURCE_DESC *res_desc, D3D12_HEAP_PROPERTIES *heap_props, D3D12_RESOURCE_STATES state, const std::string& name)
{
    res->name = name;
    res->uuid = wn_uuid();

    HRESULT result = video.device->CreateCommittedResource(heap_props, D3D12_HEAP_FLAG_NONE, res_desc, state, nullptr, IID_PPV_ARGS(&res->resource));
    if (FAILED(result)) {
        log("[d3d12] Failed to create resource %s", name.c_str());
    }

    std::wstring resource_name = std::wstring(name.begin(), name.end());
    res->resource->SetName(resource_name.c_str());

    gpu_tracker.tracked_allocations.push_back(res);
}

void gpu_resource_free(gpu_resource *res)
{
    SAFE_RELEASE(res->resource);
    for (u64 i = 0; i < gpu_tracker.tracked_allocations.size(); i++) {
        if (res->uuid == gpu_tracker.tracked_allocations[i]->uuid) {
            gpu_tracker.tracked_allocations.erase(gpu_tracker.tracked_allocations.begin() + i);
        }
    }
}

void gpu_resource_tracker_report()
{
    for (auto& resource : gpu_tracker.tracked_allocations) {
        log("[d3d12] report resource %s", resource->name.c_str());
    }
}

//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-10-26 13:47:12
//

#pragma once

#include <d3d12.h>
#include <d3d12shader.h>
#include <dxgi1_6.h>
#include <vector>
#include <unordered_map>

#include "wn_common.h"
#include "wn_shader.h"
#include "wn_filesystem.h"

#define FRAMES_IN_FLIGHT 2
#define SAFE_RELEASE(object) if (object != nullptr) object->Release()

/// @note(ame): descriptor heap
 
struct descriptor_heap
{
    ID3D12DescriptorHeap *heap;
    D3D12_DESCRIPTOR_HEAP_TYPE type;
    u32 heap_size;
    i32 increment_size;
    std::vector<bool> descriptors;
    bool shader_visible;
};

struct descriptor
{
    bool valid;
    i32 heap_index;

    D3D12_CPU_DESCRIPTOR_HANDLE cpu;
    D3D12_GPU_DESCRIPTOR_HANDLE gpu;
    descriptor_heap *parent;
};

void descriptor_heap_init(descriptor_heap *heap, D3D12_DESCRIPTOR_HEAP_TYPE type, u32 size);
descriptor descriptor_heap_alloc(descriptor_heap *heap);
void descriptor_heap_free(descriptor_heap *heap);

/// @note(ame): don't need to take the descriptor heap as it's already stored in the descriptor struct
void descriptor_free(descriptor *descriptor);

/// @note(ame): fence

struct fence
{
    ID3D12Fence *fence;
    u64 value;
};

void fence_init(fence *fence);
void fence_free(fence *fence);
void fence_wait(fence *fence, u64 value);
u64 fence_completed_value(fence *fence);

/// @note(ame): command queue

struct command_queue
{
    ID3D12CommandQueue *queue;
    D3D12_COMMAND_LIST_TYPE type;  
};

void command_queue_init(command_queue *queue, D3D12_COMMAND_LIST_TYPE type);
void command_queue_wait(command_queue *queue, fence *fence, u64 value);
void command_queue_signal(command_queue *queue, fence *fence, u64 value);
void command_queue_free(command_queue *queue);
u64 fence_signal(fence *fence, command_queue *queue);

/// @note(ame): GPU tracker
struct gpu_resource
{
    u64 uuid;
    std::string name;
    ID3D12Resource* resource;

    operator ID3D12Resource*()
    {
        return resource;
    }
};

struct gpu_resource_tracker
{
    std::vector<gpu_resource*> tracked_allocations;
};

void gpu_resource_alloc(gpu_resource *res, D3D12_RESOURCE_DESC *res_desc, D3D12_HEAP_PROPERTIES *heap_props, D3D12_RESOURCE_STATES state, const std::string& name = "Resource");
void gpu_resource_free(gpu_resource *res);
void gpu_resource_tracker_report();

/// @note(ame): texture

#define TEXTURE_ALL_MIPS 0xFFFF

#define TEXTURE_RTV D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
#define TEXTURE_DSV D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL
#define TEXTURE_UAV D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS

#define LAYOUT_COMMON D3D12_RESOURCE_STATE_COMMON
#define LAYOUT_SHADER D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE
#define LAYOUT_STORAGE D3D12_RESOURCE_STATE_UNORDERED_ACCESS
#define LAYOUT_DEPTH D3D12_RESOURCE_STATE_DEPTH_WRITE
#define LAYOUT_RENDER D3D12_RESOURCE_STATE_RENDER_TARGET
#define LAYOUT_COPYSRC D3D12_RESOURCE_STATE_COPY_SOURCE
#define LAYOUT_COPYDST D3D12_RESOURCE_STATE_COPY_DEST
#define LAYOUT_PRESENT D3D12_RESOURCE_STATE_PRESENT

struct texture
{
    gpu_resource resource;
    u64 uuid;

    u32 width;
    u32 height;
    u32 levels;
    DXGI_FORMAT format;
    D3D12_RESOURCE_STATES state;
};

void texture_init(texture *tex, u32 width, u32 height, DXGI_FORMAT format, u32 flags = 0, u32 levels = 1, bool copy = false, const std::string& name = "Texture");
u64 texture_get_size(texture *tex, u32 mip = TEXTURE_ALL_MIPS);
void texture_free(texture *tex);

/// @note(ame): texture view

enum texture_view_type
{
    TextureViewType_RenderTarget,
    TextureViewType_DepthTarget,
    TextureViewType_ShaderResource,
    TextureViewType_Storage
};

struct texture_view
{
    texture *parent_texture;
    texture_view_type type;
    descriptor handle = {};
};

void texture_view_init(texture_view *view, texture *tex, texture_view_type type, u32 mip = TEXTURE_ALL_MIPS);
void texture_view_free(texture_view *view);

/// @note(ame): swapchain

struct swapchain
{
    IDXGISwapChain4* swapchain;
    HWND hwnd;
    ID3D12Resource* resources[FRAMES_IN_FLIGHT];
    descriptor descriptors[FRAMES_IN_FLIGHT];
    texture wrappers[FRAMES_IN_FLIGHT];
    texture_view views[FRAMES_IN_FLIGHT];
    i32 width;
    i32 height;
};

void swapchain_init(swapchain *swap, HWND window);
void swapchain_resize(swapchain *swap, u32 width, u32 height);
void swapchain_present(swapchain *swap, bool vsync);
void swapchain_free(swapchain *swap);

/// @note(ame): root signature

enum root_signature_entry
{
    RootSignatureEntry_PushConstants = 999,
    RootSignatureEntry_CBV = D3D12_DESCRIPTOR_RANGE_TYPE_CBV,
    RootSignatureEntry_SRV = D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
    RootSignatureEntry_UAV = D3D12_DESCRIPTOR_RANGE_TYPE_UAV,
    RootSignatureEntry_Sampler = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER
};

struct root_signature
{
    ID3D12RootSignature *signature;
};

void root_signature_init(root_signature *signature, const std::vector<root_signature_entry>& entries, u64 push_constant_size = 0, bool bindless = false);
void root_signature_free(root_signature *signature);
ID3D12ShaderReflection* shader_get_reflection(compiled_shader *shader, D3D12_SHADER_DESC *desc);

/// @note(ame): graphics pipeline

enum depth_op
{
    DepthOp_Less = D3D12_COMPARISON_FUNC_LESS,
    DepthOp_None = D3D12_COMPARISON_FUNC_NEVER
};

struct pipeline_desc
{
    bool line = false;
    bool depth = false;
    DXGI_FORMAT depth_format;
    depth_op op;
    bool wireframe = false;

    root_signature *signature;
    std::vector<DXGI_FORMAT> formats;
    std::unordered_map<shader_type, compiled_shader> shaders;
};

struct graphics_pipeline
{
    ID3D12PipelineState *state;
    root_signature *signature;
};

void graphics_pipeline_init(graphics_pipeline *pipeline, pipeline_desc *desc);
void graphics_pipeline_free(graphics_pipeline *pipeline);

/// @note(ame): compute pipeline
typedef graphics_pipeline compute_pipeline;

void compute_pipeline_init(compute_pipeline *pipeline, compiled_shader *shader, root_signature *signature);
void compute_pipeline_free(compute_pipeline *pipeline);

/// @note(ame): buffer

enum buffer_type
{
    BufferType_Vertex,
    BufferType_Index,
    BufferType_Constant,
    BufferType_Storage,
    BufferType_Copy,
    BufferType_AccelerationStructure /// @note(ame): because we might want hardware RT at some point
};

struct buffer
{
    gpu_resource resource = {};
    D3D12_RESOURCE_STATES state;
    buffer_type type;

    u64 size = 0;
    u64 stride = 0;

    descriptor srv = {};
    descriptor uav = {};
    descriptor cbv = {};

    D3D12_VERTEX_BUFFER_VIEW vbv = {};
    D3D12_INDEX_BUFFER_VIEW ibv = {};
};

void buffer_init(buffer *buf, u64 size, u64 stride, buffer_type type, bool readback = false, const std::string& name = "Buffer");
void buffer_build_constant(buffer *buf);
void buffer_build_storage(buffer *buf);
void buffer_build_shader_resource(buffer *buf);
void buffer_map(buffer *buf, i32 start, i32 end, void **data);
void buffer_unmap(buffer *buf);
void buffer_free(buffer *buf);

/// @note(ame): Samplers

enum sampler_address
{
    SamplerAddress_Wrap = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
    SamplerAddress_Mirror = D3D12_TEXTURE_ADDRESS_MODE_MIRROR,
    SamplerAddress_Clamp = D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
    SamplerAddress_Border = D3D12_TEXTURE_ADDRESS_MODE_BORDER
};

enum sampler_filter
{
    SamplerFilter_Linear = D3D12_FILTER_MIN_MAG_MIP_LINEAR,
    SamplerFilter_Nearest = D3D12_FILTER_MIN_MAG_MIP_POINT,
    SamplerFilter_Anisotropic = D3D12_FILTER_ANISOTROPIC 
};

struct sampler
{
    descriptor handle;  
};

void sampler_init(sampler *s, sampler_address address, sampler_filter filter, bool mips = true);
void sampler_free(sampler *s);

/// @note(ame): command buffer

struct command_buffer
{
    D3D12_COMMAND_LIST_TYPE type;
    ID3D12GraphicsCommandList6 *list;
    ID3D12CommandAllocator *allocator;
};

enum geom_topology
{
    GeomTopology_Triangles = D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
    GeomTopology_Lines = D3D10_PRIMITIVE_TOPOLOGY_LINELIST
};

void command_buffer_init(command_buffer *buf, D3D12_COMMAND_LIST_TYPE type, bool close = true);
void command_buffer_begin(command_buffer *buf, bool reset = true);
void command_buffer_end(command_buffer *buf);
void command_buffer_free(command_buffer *buf);
void command_queue_submit(command_queue *queue, const std::vector<command_buffer*> buffers);

// command buffer wrappers
void command_buffer_viewport(command_buffer *buf, u32 width, u32 height);
void command_buffer_buffer_barrier(command_buffer *buf, buffer* b, D3D12_RESOURCE_STATES state);
void command_buffer_image_barrier(command_buffer *buf, texture *tex, D3D12_RESOURCE_STATES state, i32 mip = TEXTURE_ALL_MIPS);
void command_buffer_set_render_targets(command_buffer *buf, const std::vector<texture_view*>& views, texture_view *depth);
void command_buffer_clear_render_target(command_buffer *buf, texture_view *view, f32 r, f32 g, f32 b);
void command_buffer_clear_depth_target(command_buffer *buf, texture_view *view);
void command_buffer_set_vertex_buffer(command_buffer *buf, buffer *v);
void command_buffer_set_index_buffer(command_buffer *buf, buffer *i);
void command_buffer_set_topology(command_buffer *buf, geom_topology top);
void command_buffer_draw(command_buffer *buf, u32 count);
void command_buffer_draw_indexed(command_buffer *buf, u32 count);
void command_buffer_dispatch(command_buffer *buf, u32 x, u32 y, u32 z);

// command buffer graphics pipeline
void command_buffer_set_graphics_pipeline(command_buffer *buf, graphics_pipeline *pipeline);
void command_buffer_set_graphics_srv(command_buffer *buf, texture_view *view, i32 index);
void command_buffer_set_graphics_cbv(command_buffer *buf, buffer *view, i32 index);
void command_buffer_set_graphics_sampler(command_buffer *buf, sampler *s, i32 index);
void command_buffer_set_graphics_push_constants(command_buffer *buf, const void *data, u64 size, i32 index);

// command buffer compute pipeline
void command_buffer_set_compute_pipeline(command_buffer *buf, compute_pipeline *pipeline);
void command_buffer_set_compute_srv(command_buffer *buf, texture_view *view, i32 index);
void command_buffer_set_compute_cbv(command_buffer *buf, buffer *view, i32 index);
void command_buffer_set_compute_uav(command_buffer *buf, texture_view *view, i32 index);
void command_buffer_set_compute_sampler(command_buffer *buf, sampler *s, i32 index);
void command_buffer_set_compute_push_constants(command_buffer *buf, const void *data, u64 size, i32 index);

// command buffer imgui
void command_buffer_begin_gui(command_buffer *buf, u32 width, u32 height);
void command_buffer_end_gui(command_buffer *buf);

// command buffer copy
void command_buffer_copy_texture_to_texture(command_buffer *buf, texture *dst, texture *src);
void command_buffer_copy_buffer_to_texture(command_buffer *buf, texture *dst, buffer *src, u32 mip_count = 1);
void command_buffer_copy_buffer_to_buffer(command_buffer *buf, buffer *dst, buffer *src);

/// @note(ame): hot reloadable pipeline

enum hot_pipeline_type
{
    HotPipelineType_Graphics,
    HotPipelineType_Compute
};

struct hot_pipeline
{
    pipeline_desc specs;

    std::vector<root_signature_entry> entries;
    u64 push_constant_size;
    root_signature sig;

    graphics_pipeline graphics;
    compute_pipeline compute;

    struct shader_watch
    {
        file_watch watch;
        compiled_shader shader;
        std::string path;
    };
    
    std::unordered_map<shader_type, shader_watch> watches;
};

void hot_pipeline_add_shader(hot_pipeline* pipeline, const std::string& path, shader_type type);
void hot_pipeline_build(hot_pipeline* pipeline);
void hot_pipeline_rebuild(hot_pipeline* pipeline);
void hot_pipeline_free(hot_pipeline* pipeline);

/// @note(ame): texture_view cache
struct tvc_entry
{
    texture *parent;
    texture_view view;
    texture_view_type type;
    u32 mip;
    u32 ref_count = 0;
};

struct texture_view_cache
{
    std::unordered_map<u64, std::vector<tvc_entry*>> entries;
};

tvc_entry *tvc_get_entry(texture *t, texture_view_type view, u32 mip = TEXTURE_ALL_MIPS);
void tvc_release_view(tvc_entry *entry);

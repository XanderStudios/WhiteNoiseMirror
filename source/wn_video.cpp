//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-10-26 12:54:58
//

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_dx12.h>
#include <locale>
#include <codecvt>

#include "wn_video.h"
#include "wn_output.h"

extern "C" __declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
extern "C" __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
extern "C" __declspec(dllexport) extern const u32 D3D12SDKVersion = 613;
extern "C" __declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12\\";

video_device video;

void imgui_style();

void get_adapter(IDXGIFactory7 *factory, IDXGIAdapter1 **ret_adapter, bool high_perf)
{
    *ret_adapter = nullptr;
    IDXGIAdapter1* adapter = 0;
    IDXGIFactory6* factory6;
    
    if (SUCCEEDED(factory->QueryInterface(IID_PPV_ARGS(&factory6))))  {
        for (u32 AdapterIndex = 0; SUCCEEDED(factory6->EnumAdapterByGpuPreference(AdapterIndex, high_perf == true ? DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE : DXGI_GPU_PREFERENCE_UNSPECIFIED, IID_PPV_ARGS(&adapter))); ++AdapterIndex) {
            DXGI_ADAPTER_DESC1 desc;
            adapter->GetDesc1(&desc);

            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
                continue;

            if (SUCCEEDED(D3D12CreateDevice((IUnknown*)adapter, D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device*), nullptr)))
                break;
        }
    }
    
    if (adapter == nullptr) {
        for (u32 AdapterIndex = 0; SUCCEEDED(factory->EnumAdapters1(AdapterIndex, &adapter)); ++AdapterIndex)  {
            DXGI_ADAPTER_DESC1 desc;
            adapter->GetDesc1(&desc);

            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
                continue;

            if (SUCCEEDED(D3D12CreateDevice((IUnknown*)adapter, D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device*), nullptr)))
                break;
        }
    }
    
    *ret_adapter = adapter;
}

void video_init(SDL_Window* window, bool use_debug)
{
    /// @note(ame): initialize device
    HRESULT result = CreateDXGIFactory(IID_PPV_ARGS(&video.factory));
    if (FAILED(result)) {
        throw_error("Failed to create DXGI factory");
    }
    get_adapter(video.factory, &video.adapter, true);

    if (use_debug) {
        ID3D12Debug* debug;
        result = D3D12GetDebugInterface(IID_PPV_ARGS(&debug));
        if (FAILED(result))
            throw_error("[D3D12] Failed to get debug interface!");

        debug->QueryInterface(IID_PPV_ARGS(&video.debug));
        debug->Release();

        video.debug->EnableDebugLayer();
        video.debug->SetEnableGPUBasedValidation(true);
    }

    result = D3D12CreateDevice(video.adapter, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&video.device));
    if (FAILED(result)) {
        throw_error("Failed to create D3D12 device");
    }

    if (use_debug) {
        result = video.device->QueryInterface(IID_PPV_ARGS(&video.debug_device));
        if (FAILED(result))
            throw_error("[D3D12] Failed to query debug device!");

        ID3D12InfoQueue* info_queue = 0;
        video.device->QueryInterface(IID_PPV_ARGS(&info_queue));

        info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
        info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);

        D3D12_MESSAGE_SEVERITY suppress_severities[] = {
            D3D12_MESSAGE_SEVERITY_INFO
        };

        D3D12_MESSAGE_ID suppress_ids[] = {
            D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,
            D3D12_MESSAGE_ID_CLEARDEPTHSTENCILVIEW_MISMATCHINGCLEARVALUE,
            D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
            D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE
        };

        D3D12_INFO_QUEUE_FILTER filter = {0};
        filter.DenyList.NumSeverities = ARRAYSIZE(suppress_severities);
        filter.DenyList.pSeverityList = suppress_severities;
        filter.DenyList.NumIDs = ARRAYSIZE(suppress_ids);
        filter.DenyList.pIDList = suppress_ids;

        info_queue->PushStorageFilter(&filter);
        info_queue->Release();
    }

    DXGI_ADAPTER_DESC desc;
    video.adapter->GetDesc(&desc);

    std::wstring string_to_convert;
    using convert_type = std::codecvt_utf8<wchar_t>;
    std::wstring_convert<convert_type, wchar_t> converter;
    std::string device_name = converter.to_bytes(desc.Description);

    log("[video] Using GPU %s", device_name.c_str());

    /// @note(ame): initialize descriptor heaps
    descriptor_heap_init(&video.rtv_heap, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 2048);
    descriptor_heap_init(&video.dsv_heap, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 2048);
    descriptor_heap_init(&video.shader_heap, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1'000'000);
    descriptor_heap_init(&video.sampler_heap, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 1024);

    /// @note(ame): initialize command queue
    command_queue_init(&video.graphics_queue, D3D12_COMMAND_LIST_TYPE_DIRECT);

    /// @note(ame): initialize swapchain
    HWND hwnd = (HWND)SDL_GetPointerProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
    swapchain_init(&video.swap, hwnd);

    /// @note(ame): initialize sync
    fence_init(&video.graphics_fence);
    for (int i = 0; i < FRAMES_IN_FLIGHT; i++) {
        video.frame_values[i] = 0;
    }
    video.frame_index = 0;

    /// @note(ame): initialize command buffers
    for (int i = 0; i < FRAMES_IN_FLIGHT; i++) {
        command_buffer_init(&video.buffers[i], D3D12_COMMAND_LIST_TYPE_DIRECT);
    }

    /// @note(ame): Init ImGui

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& IO = ImGui::GetIO();
    IO.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    IO.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    IO.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
    
    // @note(ame): Probably shouldn't do that
    //IO.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    ImGui::StyleColorsDark();
    ImGuiStyle& Style = ImGui::GetStyle();

    ImGui::StyleColorsClassic();

    video.font_descriptor = descriptor_heap_alloc(&video.shader_heap);

    ImGui_ImplDX12_Init(video.device, FRAMES_IN_FLIGHT, DXGI_FORMAT_R8G8B8A8_UNORM, video.shader_heap.heap, video.font_descriptor.cpu, video.font_descriptor.gpu);
    ImGui_ImplSDL3_InitForD3D(window); 

    video_wait();
}

void video_resize(u32 width, u32 height)
{
    swapchain_resize(&video.swap, width, height);
    log("[video] Resized window (%d, %d)", width, height);
}

video_frame video_begin()
{
    video.frame_index = video.swap.swapchain->GetCurrentBackBufferIndex();

    video_frame frame;
    frame.frame_index = video.frame_index;
    frame.cmd_buffer = &video.buffers[video.frame_index];
    frame.backbuffer = &video.swap.wrappers[video.frame_index];
    frame.backbuffer_view = &video.swap.views[video.frame_index];
    return frame;
}

void video_end(video_frame *frame)
{
    command_queue_submit(&video.graphics_queue, { frame->cmd_buffer });

    const UINT64 fence_value = video.frame_values[video.frame_index];
    command_queue_signal(&video.graphics_queue, &video.graphics_fence, fence_value);

    if (fence_completed_value(&video.graphics_fence) < video.frame_values[video.frame_index]) {
        fence_wait(&video.graphics_fence, video.frame_values[video.frame_index]);
    }

    video.frame_values[video.frame_index] = fence_value + 1;
}

void video_present(bool vsync)
{
    swapchain_present(&video.swap, vsync);
}

void video_wait()
{
    command_queue_signal(&video.graphics_queue, &video.graphics_fence, video.frame_values[video.frame_index]);
    fence_wait(&video.graphics_fence, video.frame_values[video.frame_index]);
    video.frame_values[video.frame_index]++;
}

void video_exit()
{
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    descriptor_free(&video.font_descriptor);

    for (int i = 0; i < FRAMES_IN_FLIGHT; i++) {
        command_buffer_free(&video.buffers[i]);
    }
    swapchain_free(&video.swap);

    command_queue_free(&video.graphics_queue);

    descriptor_heap_free(&video.dsv_heap);
    descriptor_heap_free(&video.sampler_heap);
    descriptor_heap_free(&video.shader_heap);
    descriptor_heap_free(&video.rtv_heap);

    SAFE_RELEASE(video.debug_device);
    SAFE_RELEASE(video.debug);
    SAFE_RELEASE(video.device);
    SAFE_RELEASE(video.adapter);
    SAFE_RELEASE(video.factory);
}

// courtesy of https://gist.github.com/enemymouse/c8aa24e247a1d7b9fc33d45091cbb8f0
void imgui_style()
{
    ImGuiStyle& style = ImGui::GetStyle();
    style.Alpha = 1.0;
    style.WindowRounding = 3;
    style.GrabRounding = 1;
    style.GrabMinSize = 20;
    style.FrameRounding = 3;
    style.FrameRounding = 12;
    style.ScrollbarRounding = 12;

    style.Colors[ImGuiCol_Text] = ImVec4(0.00f, 1.00f, 1.00f, 1.00f);
    style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.00f, 0.40f, 0.41f, 1.00f);
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_Border] = ImVec4(0.00f, 1.00f, 1.00f, 0.65f);
    style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    style.Colors[ImGuiCol_FrameBg] = ImVec4(0.44f, 0.80f, 0.80f, 0.18f);
    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.44f, 0.80f, 0.80f, 0.27f);
    style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.44f, 0.81f, 0.86f, 0.66f);
    style.Colors[ImGuiCol_TitleBg] = ImVec4(0.14f, 0.18f, 0.21f, 0.73f);
    style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.54f);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.00f, 1.00f, 1.00f, 0.27f);
    style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.20f);
    style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.22f, 0.29f, 0.30f, 0.71f);
    style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.00f, 1.00f, 1.00f, 0.44f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.00f, 1.00f, 1.00f, 0.74f);
    style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.00f, 1.00f, 1.00f, 1.00f);
    style.Colors[ImGuiCol_CheckMark] = ImVec4(0.00f, 1.00f, 1.00f, 0.68f);
    style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.00f, 1.00f, 1.00f, 0.36f);
    style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.00f, 1.00f, 1.00f, 0.76f);
    style.Colors[ImGuiCol_Button] = ImVec4(0.00f, 0.65f, 0.65f, 0.46f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.01f, 1.00f, 1.00f, 0.43f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.00f, 1.00f, 1.00f, 0.62f);
    style.Colors[ImGuiCol_Header] = ImVec4(0.00f, 1.00f, 1.00f, 0.33f);
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.00f, 1.00f, 1.00f, 0.42f);
    style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.00f, 1.00f, 1.00f, 0.54f);
    style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.00f, 1.00f, 1.00f, 0.54f);
    style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.00f, 1.00f, 1.00f, 0.74f);
    style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.00f, 1.00f, 1.00f, 1.00f);
    style.Colors[ImGuiCol_PlotLines] = ImVec4(0.00f, 1.00f, 1.00f, 1.00f);
    style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.00f, 1.00f, 1.00f, 1.00f);
    style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.00f, 1.00f, 1.00f, 1.00f);
    style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.00f, 1.00f, 1.00f, 1.00f);
    style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.00f, 1.00f, 1.00f, 0.22f);
}

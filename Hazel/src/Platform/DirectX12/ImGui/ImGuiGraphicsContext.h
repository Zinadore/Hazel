#pragma once

#include "Platform/DirectX12/ImGui/ImguiDeviceResources.h"
#include "Platform/DirectX12/ComPtr.h"

#include "imgui.h"
#include <memory>

struct ImGuiD3D12FrameResource 
{
    ImGuiD3D12FrameResource(Hazel::TComPtr<ID3D12Device> device, UINT passCount);
    ImGuiD3D12FrameResource(const ImGuiD3D12FrameResource& rhs) = delete;
    ImGuiD3D12FrameResource& operator=(const ImGuiD3D12FrameResource& rhs) = delete;
    ~ImGuiD3D12FrameResource();

    // We cannot reset the allocator until the GPU is done processing the commands.
    // So each frame needs their own allocator.
    Hazel::TComPtr<ID3D12CommandAllocator> CommandAllocator;

    // Fence value to mark commands up to this fence point.  This lets us
    // check if these frame resources are still in use by the GPU.
    UINT64 FenceValue = 0;
    UINT64 VertexCount = 0;
    UINT64 IndexCount = 0;

    // Upload buffer for vertices
    Hazel::TComPtr<ID3D12Resource> VertexBufferUploader;
    Hazel::TComPtr<ID3D12Resource> VertexBuffer;

    // Upload buffer for indices
    Hazel::TComPtr<ID3D12Resource> IndexBufferUploader;
    Hazel::TComPtr<ID3D12Resource> IndexBuffer;
};

class ImGuiGraphicsContext
{
public:
    ImGuiGraphicsContext(HWND hwnd);
    ~ImGuiGraphicsContext();

    void Init(ImGuiViewport* viewport, ID3D12Device* device);
    void SetVSync(bool enabled);
    void NewFrame();
    void NewFrame(ID3D12GraphicsCommandList* commandList);
    void Render(ImDrawData* drawData, ID3D12PipelineState* pipelineState, ID3D12RootSignature* rootSignature, ID3D12DescriptorHeap* srvHeap);
    void SwapBuffers();
    void SwapBuffers(ID3D12GraphicsCommandList* commandList);
    void Clear(ImVec4& clearColor);

    void CreateRenderTargetViews();
    void CleanupRenderTargetViews();
    void Flush();
    void ResizeSwapChain(ImVec2& size);
    void ResizeSwapChain(ID3D12GraphicsCommandList* commandList, ImVec2& size);
    void ExecuteQueue();
    D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView() const;

    inline HWND GetNativeHandle() { return m_NativeHandle; }

public:
    ImGuiDeviceResources* DeviceResources;
    std::vector<std::unique_ptr<ImGuiD3D12FrameResource>> FrameResources;
    D3D12_VIEWPORT m_Viewport;

private:
    ImGuiD3D12FrameResource* m_CurrentFrameResource;
    HWND m_NativeHandle;

    bool m_TearingSupported;
    bool m_VSyncEnabled;
    UINT m_RTVDescriptorSize;
    UINT m_CurrentBackbufferIndex;

    // Sync
    uint64_t    m_FenceValue = 0;

    void NextFrameResource();
    void BuildFrameResources();
};

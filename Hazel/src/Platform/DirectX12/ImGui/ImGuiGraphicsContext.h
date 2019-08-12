#pragma once

#include "Platform/DirectX12/ImGui/ImguiDeviceResources.h"
#include "Platform/DirectX12/D3D12FrameResource.h"

#include "imgui.h"
#include <memory>


    class ImGuiGraphicsContext
    {
    public:
        ImGuiGraphicsContext(HWND hwnd);
        ~ImGuiGraphicsContext();

        void Init(ImGuiViewport* viewport, ID3D12Device* device);
        void SetVSync(bool enabled);
        void NewFrame();
        void NewFrame(ID3D12GraphicsCommandList* commandList);
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
        std::vector<std::unique_ptr<Hazel::D3D12FrameResource>> FrameResources;
        D3D12_VIEWPORT m_Viewport;
    private: 
        Hazel::D3D12FrameResource* m_CurrentFrameResource;
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

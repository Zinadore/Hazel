#pragma once

#include "Hazel/Renderer/GraphicsContext.h"
#include "Hazel/Core/Window.h"
#include "Platform/DirectX12/D3D12DeviceResources.h"
#include "Platform/DirectX12/D3D12FrameResource.h"

#include <memory>

namespace Hazel {
    class D3D12Context : public GraphicsContext 
    {
    public:
        D3D12Context(Window* window);
        ~D3D12Context();

        virtual void Init() override;
        virtual void SetVSync(bool enabled) override;
        virtual void NewFrame() override;
        virtual void SwapBuffers() override;

        void Clear(float* color);

        void CreateRenderTargetViews();
        void CleanupRenderTargetViews();
        void CreateDepthStencil();
        void Flush();
        void ResizeSwapChain();
        D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView() const;
        D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView() const;

        inline HWND GetNativeHandle() { return m_NativeHandle; }
        inline uint64_t GetWidth() { return m_Window->GetWidth(); }
        inline uint64_t GetHeight() { return m_Window->GetHeight(); }
    public:
        D3D12DeviceResources* DeviceResources;
        std::vector<std::unique_ptr<D3D12FrameResource>> FrameResources;
    private: 
        D3D12FrameResource* m_CurrentFrameResource;
        HWND m_NativeHandle;
        D3D12_VIEWPORT m_Viewport;

        UINT m_RTVDescriptorSize;
        UINT m_CurrentBackbufferIndex;

        // Sync
        uint64_t    m_FenceValue = 0;

        void PerformInitializationTransitions();   
        void NextFrameResource();
        void BuildFrameResources();
    };
}

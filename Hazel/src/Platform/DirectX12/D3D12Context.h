#pragma once
#include <wrl.h>
#include <chrono>

// DirectX 12 specific headers.
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>

#define NUM_FRAMES 3

// D3D12 extension library.
#include "d3dx12.h"

#include "Hazel/Renderer/GraphicsContext.h"
#include "Hazel/Window.h"

namespace Hazel {
    class D3D12Context : public GraphicsContext 
    {
    public:
        D3D12Context(Window* window);

        virtual void Init() override;
        virtual void SetVSync(bool enabled) override;
        virtual void SwapBuffers() override;

    public:
        HWND m_NativeHandle;
        RECT m_WindowRect;
        uint8_t m_NumFrames;
        bool m_IsTearingEnabled;
        UINT m_RTVDescriptorSize;
        UINT m_CurrentBackbufferIndex;

        // DirectX 12 Objects
        Microsoft::WRL::ComPtr<ID3D12Device2>               m_Device;
        Microsoft::WRL::ComPtr<ID3D12CommandQueue>          m_CommandQueue;
        Microsoft::WRL::ComPtr<IDXGISwapChain4>             m_SwapChain;
        Microsoft::WRL::ComPtr<ID3D12Resource>              m_BackBuffers[NUM_FRAMES];
        Microsoft::WRL::ComPtr<ID3D12Resource>              m_DepthStencilBuffer;
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>   m_CommandList;
        Microsoft::WRL::ComPtr<ID3D12CommandAllocator>      m_CommandAllocators[NUM_FRAMES];
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>        m_RTVDescriptorHeap;
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>        m_SRVDescriptorHeap;
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>        m_DSVDescriptorHeap;
        Microsoft::WRL::ComPtr<ID3D12Fence>                 m_Fence;
        // Sync
        HANDLE      m_FenceEvent;
        uint64_t    m_FenceValue = 0;
        uint64_t    m_FrameFenceValues[NUM_FRAMES] = {};

        void EnableDebugLayer();

        Microsoft::WRL::ComPtr<IDXGIAdapter4> GetAdapter(bool useWarp);
        
        Microsoft::WRL::ComPtr<ID3D12Device2> CreateDevice(Microsoft::WRL::ComPtr<IDXGIAdapter4> adapter);
        
        Microsoft::WRL::ComPtr<ID3D12CommandQueue> CreateCommandQueue(Microsoft::WRL::ComPtr<ID3D12Device2> device, D3D12_COMMAND_LIST_TYPE type);
        
        Microsoft::WRL::ComPtr<IDXGISwapChain4> CreateSwapChain(HWND hWnd, Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue,
            uint32_t width, uint32_t height, uint32_t bufferCount);
        
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(Microsoft::WRL::ComPtr<ID3D12Device2> device,
            D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors, D3D12_DESCRIPTOR_HEAP_FLAGS flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE);

        void UpdateRenderTargetViews(Microsoft::WRL::ComPtr<ID3D12Device2> device,
            Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain, Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap);
        
        void CreateDepthStencil();
        
            Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CreateCommandAllocator(Microsoft::WRL::ComPtr<ID3D12Device2> device, D3D12_COMMAND_LIST_TYPE type);
        
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> CreateCommandList(Microsoft::WRL::ComPtr<ID3D12Device2> device,
            Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator, D3D12_COMMAND_LIST_TYPE type);

        Microsoft::WRL::ComPtr<ID3D12Fence> CreateFence(Microsoft::WRL::ComPtr<ID3D12Device2> device);

        HANDLE CreateEventHandle();

        uint64_t Signal(Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue, Microsoft::WRL::ComPtr<ID3D12Fence> fence, uint64_t& fenceValue);

        void WaitForFenceValue(Microsoft::WRL::ComPtr<ID3D12Fence> fence, uint64_t fenceValue, HANDLE fenceEvent,
            std::chrono::milliseconds duration = std::chrono::milliseconds::max());

        void Flush();

        void CleanupRenderTargetViews();

        void ResizeSwapChain(UINT width, UINT height);

        D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView() const;

        D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView() const;

        
    };
}

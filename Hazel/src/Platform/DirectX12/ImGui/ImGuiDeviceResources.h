#pragma once

#include <vector>

// DirectX 12 specific headers.
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
// D3D12 extension library.
#include "Platform/DirectX12/d3dx12.h"
#include "Platform/DirectX12/ComPtr.h"

struct SwapChainCreationOptions {
    UINT Width;
    UINT Height;
    bool TearingSupported;
    UINT BufferCount;
    HWND Handle;
};

class ImGuiDeviceResources {
public:

    ImGuiDeviceResources(UINT bufferCount);

    Hazel::TComPtr<ID3D12Device2>                   Device;
    Hazel::TComPtr<ID3D12CommandQueue>             CommandQueue;
    Hazel::TComPtr<IDXGISwapChain4>                SwapChain;
    std::vector<Hazel::TComPtr<ID3D12Resource>>    BackBuffers;
    //Hazel::TComPtr<ID3D12Resource>                 DepthStencilBuffer;
    Hazel::TComPtr<ID3D12GraphicsCommandList>      CommandList;
    Hazel::TComPtr<ID3D12CommandAllocator>         CommandAllocator;
    Hazel::TComPtr<ID3D12DescriptorHeap>           RTVDescriptorHeap;
    Hazel::TComPtr<ID3D12DescriptorHeap>           SRVDescriptorHeap;
    //Hazel::TComPtr<ID3D12DescriptorHeap>           DSVDescriptorHeap;
    Hazel::TComPtr<ID3D12Fence>                    Fence;

    int SwapChainBufferCount;

    void EnableDebugLayer();

    Hazel::TComPtr<IDXGIAdapter4> GetAdapter(bool useWarp);

    Hazel::TComPtr<ID3D12Device2> CreateDevice(Hazel::TComPtr<IDXGIAdapter4> adapter);

    Hazel::TComPtr<ID3D12CommandQueue> CreateCommandQueue(
        Hazel::TComPtr<ID3D12Device2> device,
        D3D12_COMMAND_LIST_TYPE type);

    Hazel::TComPtr<ID3D12CommandAllocator> CreateCommandAllocator(
        Hazel::TComPtr<ID3D12Device2> device,
        D3D12_COMMAND_LIST_TYPE type);

    Hazel::TComPtr<ID3D12GraphicsCommandList> CreateCommandList(
        Hazel::TComPtr<ID3D12Device2> device,
        Hazel::TComPtr<ID3D12CommandAllocator> commandAllocator,
        D3D12_COMMAND_LIST_TYPE type,
        bool closeList = true);

    Hazel::TComPtr<IDXGISwapChain4> CreateSwapChain(
        SwapChainCreationOptions& opts,
        Hazel::TComPtr<ID3D12CommandQueue> commandQueue);

    Hazel::TComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(
        Hazel::TComPtr<ID3D12Device2> device,
        D3D12_DESCRIPTOR_HEAP_TYPE type,
        uint32_t numDescriptors,
        D3D12_DESCRIPTOR_HEAP_FLAGS flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE);

    Hazel::TComPtr<ID3D12Fence> CreateFence(Hazel::TComPtr<ID3D12Device2> device);

    uint64_t Signal(
        Hazel::TComPtr<ID3D12CommandQueue> commandQueue,
        Hazel::TComPtr<ID3D12Fence> fence,
        uint64_t fenceValue);

    void WaitForFenceValue(
        Hazel::TComPtr<ID3D12Fence> fence,
        uint64_t fenceValue,
        UINT duration = INFINITE);
};
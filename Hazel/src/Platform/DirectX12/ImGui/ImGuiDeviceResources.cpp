#include "hzpch.h"
#include "ImGuiDeviceResources.h"
#include "Platform/DirectX12/D3D12Helpers.h"

ImGuiDeviceResources::ImGuiDeviceResources(UINT bufferCount)
{
    if (bufferCount < 2) {
        HZ_CORE_ERROR("Buffer count cannot be less than 2. Was give {0}", bufferCount);
        bufferCount = 2;
    }

    SwapChainBufferCount = bufferCount;
    BackBuffers.resize(SwapChainBufferCount);
}

void ImGuiDeviceResources::EnableDebugLayer()
{
#if defined(HZ_DEBUG)
    Hazel::TComPtr<ID3D12Debug> debugInterface;
    ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface)));
    debugInterface->EnableDebugLayer();
#endif
}

Hazel::TComPtr<IDXGIAdapter4> ImGuiDeviceResources::GetAdapter(bool useWarp)
{
    Hazel::TComPtr<IDXGIFactory4> dxgiFactory;
    UINT factoryFlags = 0;

#if defined(HZ_DEBUG)
    factoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif

    ThrowIfFailed(CreateDXGIFactory2(
        factoryFlags,
        IID_PPV_ARGS(&dxgiFactory)
    ));

    Hazel::TComPtr<IDXGIAdapter1> dxgiAdapter1;
    Hazel::TComPtr<IDXGIAdapter4> dxgiAdapter4;

    if (useWarp)
    {
        ThrowIfFailed(dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&dxgiAdapter1)));
        ThrowIfFailed(dxgiAdapter1.As(&dxgiAdapter4));
    }
    else
    {
        // We grab the adapter with the highest VRAM. It "should" be the most performant one.
        SIZE_T maxDedicatedVideoMemory = 0;
        for (UINT i = 0; DXGI_ERROR_NOT_FOUND != dxgiFactory->EnumAdapters1(i, &dxgiAdapter1); ++i)
        {
            DXGI_ADAPTER_DESC1 dxgiAdapterDesc1;
            dxgiAdapter1->GetDesc1(&dxgiAdapterDesc1);

            if ((dxgiAdapterDesc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0
                && SUCCEEDED(D3D12CreateDevice(dxgiAdapter1.Get(), D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr))
                && dxgiAdapterDesc1.DedicatedVideoMemory > maxDedicatedVideoMemory)
            {
                maxDedicatedVideoMemory = dxgiAdapterDesc1.DedicatedVideoMemory;
                ThrowIfFailed(dxgiAdapter1.As(&dxgiAdapter4));
            }
        }
    }
    return dxgiAdapter4;
}

Hazel::TComPtr<ID3D12Device2> ImGuiDeviceResources::CreateDevice(Hazel::TComPtr<IDXGIAdapter4> adapter)
{
    Hazel::TComPtr<ID3D12Device2> d3d12Device2;
    ThrowIfFailed(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&d3d12Device2)));

#if defined(HZ_DEBUG)
    // Add some message suppression in debug mode
    Hazel::TComPtr<ID3D12InfoQueue> pInfoQueue;
    if (SUCCEEDED(d3d12Device2.As(&pInfoQueue)))
    {
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);

        // Suppress whole categories of messages
        //D3D12_MESSAGE_CATEGORY Categories[] = {};

        // Suppress messages based on their severity level
        D3D12_MESSAGE_SEVERITY Severities[] =
        {
            D3D12_MESSAGE_SEVERITY_INFO
        };

        // Suppress individual messages by their ID
        D3D12_MESSAGE_ID DenyIds[] = {
            D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,   // I'm really not sure how to avoid this message.
            D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,                         // This warning occurs when using capture frame while graphics debugging.
            D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,                       // This warning occurs when using capture frame while graphics debugging.
        };

        D3D12_INFO_QUEUE_FILTER NewFilter = {};
        //NewFilter.DenyList.NumCategories = _countof(Categories);
        //NewFilter.DenyList.pCategoryList = Categories;
        NewFilter.DenyList.NumSeverities = _countof(Severities);
        NewFilter.DenyList.pSeverityList = Severities;
        NewFilter.DenyList.NumIDs = _countof(DenyIds);
        NewFilter.DenyList.pIDList = DenyIds;

        ThrowIfFailed(pInfoQueue->PushStorageFilter(&NewFilter));
    }
#endif

    return d3d12Device2;
}

Hazel::TComPtr<ID3D12CommandQueue> ImGuiDeviceResources::CreateCommandQueue(Hazel::TComPtr<ID3D12Device2> device, D3D12_COMMAND_LIST_TYPE type)
{
    Hazel::TComPtr<ID3D12CommandQueue> d3d12CommandQueue;

    D3D12_COMMAND_QUEUE_DESC desc = {};
    desc.Type = type;
    desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    desc.NodeMask = 0;

    ThrowIfFailed(device->CreateCommandQueue(&desc, IID_PPV_ARGS(&d3d12CommandQueue)));

    return d3d12CommandQueue;
}

Hazel::TComPtr<ID3D12CommandAllocator> ImGuiDeviceResources::CreateCommandAllocator(Hazel::TComPtr<ID3D12Device2> device, D3D12_COMMAND_LIST_TYPE type)
{
    Hazel::TComPtr<ID3D12CommandAllocator> commandAllocator;
    ThrowIfFailed(device->CreateCommandAllocator(type, IID_PPV_ARGS(&commandAllocator)));

    return commandAllocator;
}

Hazel::TComPtr<ID3D12GraphicsCommandList> ImGuiDeviceResources::CreateCommandList(Hazel::TComPtr<ID3D12Device2> device, Hazel::TComPtr<ID3D12CommandAllocator> commandAllocator, D3D12_COMMAND_LIST_TYPE type, bool closeList)
{
    Hazel::TComPtr<ID3D12GraphicsCommandList> commandList;
    ThrowIfFailed(device->CreateCommandList(0, type, commandAllocator.Get(), nullptr, IID_PPV_ARGS(&commandList)));

    if (closeList) {
        ThrowIfFailed(commandList->Close());
    }

    return commandList;
}

Hazel::TComPtr<IDXGISwapChain4> ImGuiDeviceResources::CreateSwapChain(SwapChainCreationOptions& opts, Hazel::TComPtr<ID3D12CommandQueue> commandQueue)
{
    Hazel::TComPtr<IDXGISwapChain4> dxgiSwapChain4;
    Hazel::TComPtr<IDXGIFactory4> dxgiFactory4;
    UINT createFactoryFlags = 0;
#if defined(_DEBUG)
    createFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif

    ThrowIfFailed(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory4)));

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.Width = opts.Width;
    swapChainDesc.Height = opts.Height;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.Stereo = FALSE;
    swapChainDesc.SampleDesc = { 1, 0 };
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = opts.BufferCount;
    swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    // TODO: It is recommended to always allow tearing if tearing support is available.
    swapChainDesc.Flags = opts.TearingSupported ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

    Hazel::TComPtr<IDXGISwapChain1> swapChain1;
    ThrowIfFailed(dxgiFactory4->CreateSwapChainForHwnd(
        commandQueue.Get(),
        opts.Handle,
        &swapChainDesc,
        nullptr,
        nullptr,
        &swapChain1));

    // Disable the Alt+Enter fullscreen toggle feature. Switching to fullscreen
    // will be handled manually.
    ThrowIfFailed(dxgiFactory4->MakeWindowAssociation(opts.Handle, DXGI_MWA_NO_ALT_ENTER));

    ThrowIfFailed(swapChain1.As(&dxgiSwapChain4));

    return dxgiSwapChain4;
}

Hazel::TComPtr<ID3D12DescriptorHeap> ImGuiDeviceResources::CreateDescriptorHeap(Hazel::TComPtr<ID3D12Device2> device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors, D3D12_DESCRIPTOR_HEAP_FLAGS flags /*= D3D12_DESCRIPTOR_HEAP_FLAG_NONE*/)
{
    Hazel::TComPtr<ID3D12DescriptorHeap> descriptorHeap;

    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.NumDescriptors = numDescriptors;
    desc.Type = type;
    desc.Flags = flags;

    ThrowIfFailed(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&descriptorHeap)));

    return descriptorHeap;
}

Hazel::TComPtr<ID3D12Fence> ImGuiDeviceResources::CreateFence(Hazel::TComPtr<ID3D12Device2> device)
{
    Hazel::TComPtr<ID3D12Fence> fence;

    ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));

    return fence;
}

uint64_t ImGuiDeviceResources::Signal(Hazel::TComPtr<ID3D12CommandQueue> commandQueue, Hazel::TComPtr<ID3D12Fence> fence, uint64_t fenceValue)
{
    uint64_t val = ++fenceValue;
    ThrowIfFailed(commandQueue->Signal(fence.Get(), val));

    return val;
}

void ImGuiDeviceResources::WaitForFenceValue(Hazel::TComPtr<ID3D12Fence> fence, uint64_t fenceValue, UINT duration)
{
    if (fence->GetCompletedValue() < fenceValue)
    {
        HANDLE evt = ::CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);

        ThrowIfFailed(fence->SetEventOnCompletion(fenceValue, evt));

        ::WaitForSingleObject(evt, duration);

        ::CloseHandle(evt);
    }
}

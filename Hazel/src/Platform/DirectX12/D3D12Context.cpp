#include "hzpch.h"
#include "D3D12Context.h"
#include "D3D12Helpers.h"

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <string>
#include "Hazel/Log.h"


bool CheckTearingSupport()
{
    BOOL allowTearing = FALSE;

    // Rather than create the DXGI 1.5 factory interface directly, we create the
    // DXGI 1.4 interface and query for the 1.5 interface. This is to enable the 
    // graphics debugging tools which will not support the 1.5 factory interface 
    // until a future update.
    Microsoft::WRL::ComPtr<IDXGIFactory4> factory4;
    if (SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&factory4))))
    {
        Microsoft::WRL::ComPtr<IDXGIFactory5> factory5;
        if (SUCCEEDED(factory4.As(&factory5)))
        {
            if (FAILED(factory5->CheckFeatureSupport(
                DXGI_FEATURE_PRESENT_ALLOW_TEARING,
                &allowTearing, sizeof(allowTearing))))
            {
                allowTearing = FALSE;
            }
        }
    }

    return allowTearing == TRUE;
}

namespace Hazel {
    using namespace Microsoft::WRL;

    D3D12Context::D3D12Context(Window* window)
        : GraphicsContext(window), m_NumFrames(NUM_FRAMES)
    {
        auto wnd = (GLFWwindow *)window->GetNativeWindow();
        m_NativeHandle = glfwGetWin32Window(wnd);
        HZ_CORE_ASSERT(m_NativeHandle, "HWND is null!");
    }

    void D3D12Context::Init()
    {   
        auto width = m_Window->GetWidth();
        auto height = m_Window->GetHeight();

        EnableDebugLayer();
        m_TearingSupported = CheckTearingSupport();

        ::GetWindowRect(m_NativeHandle, &m_WindowRect);

        // The device
        ComPtr<IDXGIAdapter4> theAdapter = GetAdapter(false);
        m_Device = CreateDevice(theAdapter);
        NAME_D3D12_OBJECT(m_Device);
        
        // The command queue        
        m_CommandQueue = CreateCommandQueue(m_Device, D3D12_COMMAND_LIST_TYPE_DIRECT);
        NAME_D3D12_OBJECT(m_CommandQueue);
        
        // The Swap Chain
        m_SwapChain = CreateSwapChain(m_NativeHandle, m_CommandQueue, width, height, m_NumFrames);
        m_CurrentBackbufferIndex = m_SwapChain->GetCurrentBackBufferIndex();
        
        // Command Objects
        for (int i = 0; i < m_NumFrames; ++i)
        {
            m_CommandAllocators[i] = CreateCommandAllocator(m_Device, D3D12_COMMAND_LIST_TYPE_DIRECT);
            NAME_D3D12_OBJECT_INDEXED(m_CommandAllocators, i);
        }

        m_CommandList = CreateCommandList(m_Device, m_CommandAllocators[m_CurrentBackbufferIndex], D3D12_COMMAND_LIST_TYPE_DIRECT);
        NAME_D3D12_OBJECT(m_CommandList);
        // The Heaps
        m_RTVDescriptorHeap = CreateDescriptorHeap(m_Device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, m_NumFrames);
        NAME_D3D12_OBJECT(m_RTVDescriptorHeap);
        m_RTVDescriptorSize = m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        m_SRVDescriptorHeap = CreateDescriptorHeap(m_Device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
        NAME_D3D12_OBJECT(m_SRVDescriptorHeap);
        m_DSVDescriptorHeap = CreateDescriptorHeap(m_Device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1);
        NAME_D3D12_OBJECT(m_DSVDescriptorHeap);

        UpdateRenderTargetViews(m_Device, m_SwapChain, m_RTVDescriptorHeap);
        CreateDepthStencil();
        
        m_Viewport.TopLeftX = 0.0f;
        m_Viewport.TopLeftY = 0.0f;
        m_Viewport.Width = static_cast<float>(width);
        m_Viewport.Height = static_cast<float>(height);
        m_Viewport.MinDepth = 0.0f;
        m_Viewport.MaxDepth = 1.0f;

        // Sync
        m_Fence = CreateFence(m_Device);
        m_FenceEvent = CreateEventHandle();

        PerformInitializationTransitions();

        DXGI_ADAPTER_DESC3 desc;
        theAdapter->GetDesc3(&desc);

        std::wstring description(desc.Description);
        std::string str(description.begin(), description.end());
        HZ_CORE_INFO("DirectX 12 Info:");
        HZ_CORE_INFO("  Vendor: {0}", desc.VendorId);
        HZ_CORE_INFO("  Renderer: {0}", str);
    }

    void D3D12Context::SetVSync(bool enabled)
    {
        m_VSyncEnabled = enabled;
    }

    void D3D12Context::SwapBuffers()
    {
        auto backBuffer = m_BackBuffers[m_CurrentBackbufferIndex];

        CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            backBuffer.Get(),
            D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

        m_CommandList->ResourceBarrier(1, &barrier);

        ThrowIfFailed(m_CommandList->Close());

        ID3D12CommandList* const commandLists[] = {
            m_CommandList.Get()
        };
        m_CommandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

        UINT syncInterval = m_VSyncEnabled ? 1 : 0;
        UINT presentFlags = m_TearingSupported && !m_VSyncEnabled ? DXGI_PRESENT_ALLOW_TEARING : 0;
        ThrowIfFailed(m_SwapChain->Present(syncInterval, presentFlags));

        m_FrameFenceValues[m_CurrentBackbufferIndex] = Signal(m_CommandQueue, m_Fence, m_FenceValue);

        m_CurrentBackbufferIndex = m_SwapChain->GetCurrentBackBufferIndex();

        WaitForFenceValue(m_Fence, m_FrameFenceValues[m_CurrentBackbufferIndex], m_FenceEvent);
    }

    void D3D12Context::EnableDebugLayer()
    {
#if defined(HZ_DEBUG)
        ComPtr<ID3D12Debug> debugInterface;
        ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface)));
        debugInterface->EnableDebugLayer();
#endif
    }

    ComPtr<IDXGIAdapter4> D3D12Context::GetAdapter(bool useWarp)
    {
        ComPtr<IDXGIFactory4> dxgiFactory;
        UINT factoryFlags = 0;

#if defined(HZ_DEBUG)
        factoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif

        ThrowIfFailed(
            CreateDXGIFactory2(
                factoryFlags, 
                IID_PPV_ARGS(&dxgiFactory)
            )
        );
        
        ComPtr<IDXGIAdapter1> dxgiAdapter1;
        ComPtr<IDXGIAdapter4> dxgiAdapter4;

        if (useWarp)
        {
            ThrowIfFailed(dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&dxgiAdapter1)));
            ThrowIfFailed(dxgiAdapter1.As(&dxgiAdapter4));
        }
        else 
        {
            // We grab the adapter with the highest VRAM. It "should" be the most performant one.
            SIZE_T maxDedicatedVideoMemory = 0;
            for (UINT i = 0; dxgiFactory->EnumAdapters1(i, &dxgiAdapter1) != DXGI_ERROR_NOT_FOUND; ++i)
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

    ComPtr<ID3D12Device2> D3D12Context::CreateDevice(ComPtr<IDXGIAdapter4> adapter)
    {
        ComPtr<ID3D12Device2> d3d12Device2;
        ThrowIfFailed(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&d3d12Device2)));
        
#if defined(HZ_DEBUG)
        ComPtr<ID3D12InfoQueue> pInfoQueue;
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

    ComPtr<ID3D12CommandQueue> D3D12Context::CreateCommandQueue(ComPtr<ID3D12Device2> device, D3D12_COMMAND_LIST_TYPE type)
    {
        ComPtr<ID3D12CommandQueue> d3d12CommandQueue;

        D3D12_COMMAND_QUEUE_DESC desc = {};
        desc.Type = type;
        desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
        desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        desc.NodeMask = 0;

        ThrowIfFailed(device->CreateCommandQueue(&desc, IID_PPV_ARGS(&d3d12CommandQueue)));

        return d3d12CommandQueue;
    }

    ComPtr<IDXGISwapChain4> D3D12Context::CreateSwapChain(HWND hWnd, ComPtr<ID3D12CommandQueue> commandQueue,
         uint32_t width, uint32_t height, uint32_t bufferCount)
    {
        ComPtr<IDXGISwapChain4> dxgiSwapChain4;
        ComPtr<IDXGIFactory4> dxgiFactory4;
        UINT createFactoryFlags = 0;
#if defined(_DEBUG)
        createFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif

        ThrowIfFailed(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory4)));

        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
        swapChainDesc.Width = width;
        swapChainDesc.Height = height;
        swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapChainDesc.Stereo = FALSE;
        swapChainDesc.SampleDesc = { 1, 0 };
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.BufferCount = bufferCount;
        swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
        // TODO: It is recommended to always allow tearing if tearing support is available.
        swapChainDesc.Flags = CheckTearingSupport() ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;
       
        ComPtr<IDXGISwapChain1> swapChain1;
        ThrowIfFailed(dxgiFactory4->CreateSwapChainForHwnd(
            commandQueue.Get(),
            hWnd,
            &swapChainDesc,
            nullptr,
            nullptr,
            &swapChain1));

        // Disable the Alt+Enter fullscreen toggle feature. Switching to fullscreen
        // will be handled manually.
        ThrowIfFailed(dxgiFactory4->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER));

        ThrowIfFailed(swapChain1.As(&dxgiSwapChain4));

        return dxgiSwapChain4;
    }

    ComPtr<ID3D12DescriptorHeap> D3D12Context::CreateDescriptorHeap(ComPtr<ID3D12Device2> device,
        D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors, D3D12_DESCRIPTOR_HEAP_FLAGS flags)
    {
        ComPtr<ID3D12DescriptorHeap> descriptorHeap;

        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.NumDescriptors = numDescriptors;
        desc.Type = type;
        desc.Flags = flags;

        ThrowIfFailed(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&descriptorHeap)));

        return descriptorHeap;
    }

    void D3D12Context::UpdateRenderTargetViews(ComPtr<ID3D12Device2> device,
        ComPtr<IDXGISwapChain4> swapChain, ComPtr<ID3D12DescriptorHeap> descriptorHeap)
    {
        auto rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(descriptorHeap->GetCPUDescriptorHandleForHeapStart());

        for (int i = 0; i < m_NumFrames; ++i)
        {
            ComPtr<ID3D12Resource> backBuffer;
            ThrowIfFailed(swapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer)));

            device->CreateRenderTargetView(backBuffer.Get(), nullptr, rtvHandle);

            m_BackBuffers[i] = backBuffer;
            NAME_D3D12_OBJECT_INDEXED(m_BackBuffers, i);

            rtvHandle.Offset(1, rtvDescriptorSize);
        }

        m_CurrentBackbufferIndex = m_SwapChain->GetCurrentBackBufferIndex();
    }

   void D3D12Context::CreateDepthStencil()
    {
        D3D12_RESOURCE_DESC desc;

        desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        desc.Alignment = 0;
        desc.Width = m_Window->GetWidth();
        desc.Height = m_Window->GetHeight();
        desc.DepthOrArraySize = 1;
        desc.MipLevels = 1;
        desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        desc.SampleDesc = { 1, 0 };
        desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

        D3D12_CLEAR_VALUE clear;
        clear.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        clear.DepthStencil.Depth = 1.0f;
        clear.DepthStencil.Stencil = 0;

        ThrowIfFailed(m_Device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
            D3D12_HEAP_FLAG_NONE,
            &desc,
            D3D12_RESOURCE_STATE_COMMON,
            &clear,
            IID_PPV_ARGS(m_DepthStencilBuffer.GetAddressOf())
        ));

        m_Device->CreateDepthStencilView(
            m_DepthStencilBuffer.Get(),
            nullptr,
            DepthStencilView()
        );

        NAME_D3D12_OBJECT(m_DepthStencilBuffer);
    }

    ComPtr<ID3D12CommandAllocator> D3D12Context::CreateCommandAllocator(ComPtr<ID3D12Device2> device, D3D12_COMMAND_LIST_TYPE type)
    {
        ComPtr<ID3D12CommandAllocator> commandAllocator;
        ThrowIfFailed(device->CreateCommandAllocator(type, IID_PPV_ARGS(&commandAllocator)));

        return commandAllocator;
    }

    ComPtr<ID3D12GraphicsCommandList> D3D12Context::CreateCommandList(ComPtr<ID3D12Device2> device,
        ComPtr<ID3D12CommandAllocator> commandAllocator, D3D12_COMMAND_LIST_TYPE type)
    {
        ComPtr<ID3D12GraphicsCommandList> commandList;
        ThrowIfFailed(device->CreateCommandList(0, type, commandAllocator.Get(), nullptr, IID_PPV_ARGS(&commandList)));

        ThrowIfFailed(commandList->Close());

        return commandList;
    }

    ComPtr<ID3D12Fence> D3D12Context::CreateFence(ComPtr<ID3D12Device2> device)
    {
        ComPtr<ID3D12Fence> fence;

        ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));

        return fence;
    }

    HANDLE D3D12Context::CreateEventHandle()
    {
        HANDLE fenceEvent;

        fenceEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
        HZ_CORE_ASSERT(fenceEvent, "Failed to create fence event.");

        return fenceEvent;
    }

    uint64_t D3D12Context::Signal(ComPtr<ID3D12CommandQueue> commandQueue, ComPtr<ID3D12Fence> fence,
        uint64_t& fenceValue)
    {
        uint64_t val = ++fenceValue;
        ThrowIfFailed(commandQueue->Signal(fence.Get(), val));

        return val;
    }
    
    void D3D12Context::WaitForFenceValue(ComPtr<ID3D12Fence> fence, uint64_t fenceValue, HANDLE fenceEvent, std::chrono::milliseconds duration)
    {
        if (fence->GetCompletedValue() < fenceValue)
        {
            ThrowIfFailed(fence->SetEventOnCompletion(fenceValue, fenceEvent));
            ::WaitForSingleObject(fenceEvent, static_cast<DWORD>(duration.count()));
        }
    }
    
    void D3D12Context::Flush()
    {
        ThrowIfFailed(m_CommandQueue->Signal(m_Fence.Get(), m_FrameFenceValues[m_CurrentBackbufferIndex]));

        ThrowIfFailed(m_Fence->SetEventOnCompletion(m_FrameFenceValues[m_CurrentBackbufferIndex], m_FenceEvent));

        ::WaitForSingleObjectEx(m_FenceEvent, INFINITE, FALSE);

        m_FrameFenceValues[m_CurrentBackbufferIndex]++;
    }
    void D3D12Context::CleanupRenderTargetViews()
    {
        Flush();

        //m_CommandList->Close();

        //auto allocator = m_CommandAllocators[m_CurrentBackbufferIndex];

        //ThrowIfFailed(m_CommandList->Reset(allocator.Get(), nullptr));

        for (UINT i = 0; i < m_NumFrames; i++)
        {
            m_BackBuffers[i].Reset();
            m_FrameFenceValues[i] = m_FrameFenceValues[m_CurrentBackbufferIndex];
        }

        m_DepthStencilBuffer.Reset();
    }
    void D3D12Context::ResizeSwapChain()
    {
        auto width = m_Window->GetWidth();
        auto height = m_Window->GetHeight();

        DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
        ThrowIfFailed(m_SwapChain->GetDesc(&swapChainDesc));
        ThrowIfFailed(m_SwapChain->ResizeBuffers(m_NumFrames, width, height,
            swapChainDesc.BufferDesc.Format, swapChainDesc.Flags));

        m_CurrentBackbufferIndex = m_SwapChain->GetCurrentBackBufferIndex();
    }
    D3D12_CPU_DESCRIPTOR_HANDLE D3D12Context::CurrentBackBufferView() const
    {
        return CD3DX12_CPU_DESCRIPTOR_HANDLE(
            m_RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
            m_CurrentBackbufferIndex,
            m_RTVDescriptorSize
        );
    }
    D3D12_CPU_DESCRIPTOR_HANDLE D3D12Context::DepthStencilView() const
    {
        return m_DSVDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
    }
    void D3D12Context::PerformInitializationTransitions()
    {
        auto commandAllocator = m_CommandAllocators[m_CurrentBackbufferIndex];

        commandAllocator->Reset();
        m_CommandList->Reset(commandAllocator.Get(), nullptr);

        // Transitions go here
        {
            auto dsBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
                m_DepthStencilBuffer.Get(),
                D3D12_RESOURCE_STATE_COMMON,
                D3D12_RESOURCE_STATE_DEPTH_WRITE
            );

            m_CommandList->ResourceBarrier(1, &dsBarrier);
        }

        ThrowIfFailed(m_CommandList->Close());

        ID3D12CommandList* const commandLists[] = {
            m_CommandList.Get()
        };
        m_CommandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

        Flush();
    }
}
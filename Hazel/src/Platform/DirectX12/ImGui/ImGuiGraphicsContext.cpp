#include "hzpch.h"
#include "ImGuiGraphicsContext.h"
#include "Platform/DirectX12/D3D12Helpers.h"

#include <string>
#include "Hazel/Log.h"
#include "Platform/DirectX12/ComPtr.h"

#define NUM_FRAMES 2


bool CheckTearingSupport2()
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

ImGuiGraphicsContext::ImGuiGraphicsContext(HWND hwnd)
    :m_NativeHandle(hwnd)
{

    HZ_CORE_ASSERT(m_NativeHandle, "HWND is null!");
    DeviceResources = new ImGuiDeviceResources(NUM_FRAMES);
}

ImGuiGraphicsContext::~ImGuiGraphicsContext()
{
    Flush();
    delete DeviceResources;
}

void ImGuiGraphicsContext::Init(ImGuiViewport* viewport, ID3D12Device* device)
{
    auto width = viewport->Size.x;
    auto height = viewport->Size.y;
    m_TearingSupported = CheckTearingSupport2();


    // The device
    Hazel::TComPtr<ID3D12Device2> device2;
    device->QueryInterface(IID_PPV_ARGS(&device2));
    DeviceResources->Device = device2;
    device->Release();

    BuildFrameResources();

    // The command queue        
    DeviceResources->CommandQueue = DeviceResources->CreateCommandQueue(
        DeviceResources->Device,
        D3D12_COMMAND_LIST_TYPE_DIRECT
    );
    NAME_D3D12_OBJECT(DeviceResources->CommandQueue);

    // The Swap Chain
    SwapChainCreationOptions opts = { 0 };
    opts.Width = width;
    opts.Height = height;
    opts.BufferCount = DeviceResources->SwapChainBufferCount;
    opts.TearingSupported = m_TearingSupported;
    opts.Handle = m_NativeHandle;

    DeviceResources->SwapChain = DeviceResources->CreateSwapChain(
        opts,
        DeviceResources->CommandQueue
    );

    m_CurrentBackbufferIndex = DeviceResources
        ->SwapChain
        ->GetCurrentBackBufferIndex();

    // Command Objects
    DeviceResources->CommandAllocator = DeviceResources->CreateCommandAllocator(
        DeviceResources->Device,
        D3D12_COMMAND_LIST_TYPE_DIRECT
    );
    NAME_D3D12_OBJECT(DeviceResources->CommandAllocator);

    DeviceResources->CommandList = DeviceResources->CreateCommandList(
        DeviceResources->Device,
        DeviceResources->CommandAllocator,
        D3D12_COMMAND_LIST_TYPE_DIRECT
    );
    NAME_D3D12_OBJECT(DeviceResources->CommandList);

    // The Heaps
    DeviceResources->RTVDescriptorHeap = DeviceResources->CreateDescriptorHeap(
        DeviceResources->Device,
        D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
        DeviceResources->SwapChainBufferCount
    );
    NAME_D3D12_OBJECT(DeviceResources->RTVDescriptorHeap);

    m_RTVDescriptorSize = DeviceResources
        ->Device
        ->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    DeviceResources->SRVDescriptorHeap = DeviceResources->CreateDescriptorHeap(
        DeviceResources->Device,
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
        1,
        D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
    );
    NAME_D3D12_OBJECT(DeviceResources->SRVDescriptorHeap);

    CreateRenderTargetViews();

    m_Viewport.TopLeftX = 0.0f;
    m_Viewport.TopLeftY = 0.0f;
    m_Viewport.Width = static_cast<float>(width);
    m_Viewport.Height = static_cast<float>(height);
    m_Viewport.MinDepth = 0.0f;
    m_Viewport.MaxDepth = 1.0f;

    // Sync
    DeviceResources->Fence = DeviceResources->CreateFence(DeviceResources->Device);
}

void ImGuiGraphicsContext::SetVSync(bool enabled)
{
    m_VSyncEnabled = enabled;
}

void ImGuiGraphicsContext::NewFrame(ID3D12GraphicsCommandList * commandList)
{
    NextFrameResource();
    // Get from resource
    auto commandAllocator = m_CurrentFrameResource->CommandAllocator;

    ThrowIfFailed(commandAllocator->Reset());
    ThrowIfFailed(commandList->Reset(
        commandAllocator.Get(),
        nullptr)
    );

    commandList->RSSetViewports(1, &m_Viewport);
    commandList->OMSetRenderTargets(1, &CurrentBackBufferView(), false, nullptr);
    commandList->SetDescriptorHeaps(1, DeviceResources->SRVDescriptorHeap.GetAddressOf());

    auto backBuffer = DeviceResources->BackBuffers[m_CurrentBackbufferIndex];

    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        backBuffer.Get(),
        D3D12_RESOURCE_STATE_PRESENT,
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
        D3D12_RESOURCE_BARRIER_FLAG_NONE);

    commandList->ResourceBarrier(1, &barrier);
}

void ImGuiGraphicsContext::NewFrame()
{
    NewFrame(DeviceResources->CommandList.Get());
}

void ImGuiGraphicsContext::SwapBuffers(ID3D12GraphicsCommandList * commandList)
{
    auto backBuffer = DeviceResources->BackBuffers[m_CurrentBackbufferIndex];

    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        backBuffer.Get(),
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_PRESENT
    );

    commandList->ResourceBarrier(1, &barrier);

    ThrowIfFailed(commandList->Close());

    ID3D12CommandList* const commandLists[] = {
        commandList
    };
    DeviceResources->CommandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

    UINT syncInterval = m_VSyncEnabled ? 1 : 0;
    UINT presentFlags = m_TearingSupported && !m_VSyncEnabled ? DXGI_PRESENT_ALLOW_TEARING : 0;
    ThrowIfFailed(DeviceResources->SwapChain->Present(syncInterval, presentFlags));

    // Get new backbuffer index
    m_CurrentBackbufferIndex = DeviceResources->SwapChain->GetCurrentBackBufferIndex();

    // Signal the queue
    m_FenceValue = DeviceResources->Signal(
        DeviceResources->CommandQueue,
        DeviceResources->Fence,
        m_FenceValue
    );

    // Update the resource
    m_CurrentFrameResource->FenceValue = m_FenceValue;
}

void ImGuiGraphicsContext::SwapBuffers()
{
    SwapBuffers(DeviceResources->CommandList.Get());
}

void ImGuiGraphicsContext::Clear(ImVec4 & clearColor)
{
    D3D12_CPU_DESCRIPTOR_HANDLE rtv = CurrentBackBufferView();

    float color[] = { clearColor.x, clearColor.y, clearColor.z, clearColor.w };

    DeviceResources->CommandList->ClearRenderTargetView(rtv, color, 0, nullptr);
}

void ImGuiGraphicsContext::CreateRenderTargetViews()
{
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(
        DeviceResources
        ->RTVDescriptorHeap
        ->GetCPUDescriptorHandleForHeapStart()
    );

    for (int i = 0; i < DeviceResources->SwapChainBufferCount; ++i)
    {
        Hazel::TComPtr<ID3D12Resource> backBuffer;
        ThrowIfFailed(DeviceResources->SwapChain
            ->GetBuffer(i, IID_PPV_ARGS(&backBuffer))
        );

        DeviceResources->Device->CreateRenderTargetView(
            backBuffer.Get(), nullptr, rtvHandle);

        DeviceResources->BackBuffers[i] = backBuffer;
        NAME_D3D12_OBJECT_INDEXED(DeviceResources->BackBuffers, i);

        rtvHandle.Offset(1, m_RTVDescriptorSize);
    }

    m_CurrentBackbufferIndex = DeviceResources->SwapChain->GetCurrentBackBufferIndex();
}

void ImGuiGraphicsContext::Flush()
{
    m_FenceValue = DeviceResources->Signal(
        DeviceResources->CommandQueue,
        DeviceResources->Fence,
        m_FenceValue
    );

    DeviceResources->WaitForFenceValue(
        DeviceResources->Fence,
        m_FenceValue
    );
}

void ImGuiGraphicsContext::CleanupRenderTargetViews()
{
    ThrowIfFailed(DeviceResources->CommandList->Reset(
        DeviceResources->CommandAllocator.Get(),
        nullptr)
    );

    for (UINT i = 0; i < DeviceResources->SwapChainBufferCount; i++)
    {
        DeviceResources->BackBuffers[i].Reset();
    }
}

void ImGuiGraphicsContext::ResizeSwapChain(ID3D12GraphicsCommandList * commandList, ImVec2 & size)
{
    auto width = size.x;
    auto height = size.y;

    DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
    ThrowIfFailed(DeviceResources->SwapChain->GetDesc(&swapChainDesc));
    ThrowIfFailed(DeviceResources->SwapChain->ResizeBuffers(
        DeviceResources->SwapChainBufferCount,
        width,
        height,
        swapChainDesc.BufferDesc.Format,
        swapChainDesc.Flags)
    );

    m_CurrentBackbufferIndex = DeviceResources->SwapChain->GetCurrentBackBufferIndex();

    m_Viewport.Width = width;
    m_Viewport.Height = height;
}

void ImGuiGraphicsContext::ResizeSwapChain(ImVec2& size)
{
    ResizeSwapChain(DeviceResources->CommandList.Get(), size);
}

void ImGuiGraphicsContext::ExecuteQueue()
{
    // Execute all the resize magic
    ThrowIfFailed(DeviceResources->CommandList->Close());

    ID3D12CommandList* const commandLists[] = {
        DeviceResources->CommandList.Get()
    };
    DeviceResources->CommandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);
}

D3D12_CPU_DESCRIPTOR_HANDLE ImGuiGraphicsContext::CurrentBackBufferView() const
{
    return CD3DX12_CPU_DESCRIPTOR_HANDLE(
        DeviceResources->RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
        m_CurrentBackbufferIndex,
        m_RTVDescriptorSize
    );
}

void ImGuiGraphicsContext::NextFrameResource()
{
    m_CurrentBackbufferIndex = DeviceResources->SwapChain->GetCurrentBackBufferIndex();

    m_CurrentFrameResource = FrameResources[m_CurrentBackbufferIndex].get();

    if (m_CurrentFrameResource->FenceValue != 0) {
        DeviceResources->WaitForFenceValue(
            DeviceResources->Fence,
            m_CurrentFrameResource->FenceValue
        );
    }
}

void ImGuiGraphicsContext::BuildFrameResources()
{
    auto count = DeviceResources->SwapChainBufferCount;
    FrameResources.reserve(count);

    for (int i = 0; i < count; i++)
    {
        FrameResources.push_back(std::make_unique<Hazel::D3D12FrameResource>(
            DeviceResources->Device,
            1
            ));
    }
}
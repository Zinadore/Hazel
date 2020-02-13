#include "hzpch.h"
#include "ImGuiGraphicsContext.h"
#include "Platform/DirectX12/D3D12Helpers.h"

#include <string>
#include "Hazel/Core/Log.h"
#include "Platform/DirectX12/ComPtr.h"

#define NUM_FRAMES 2

extern struct VERTEX_CONSTANT_BUFFER
{
    float   mvp[4][4];
};

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

ImGuiD3D12FrameResource::ImGuiD3D12FrameResource(Hazel::TComPtr<ID3D12Device> device, UINT passCount)
{
    ThrowIfFailed(device->CreateCommandAllocator(
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        IID_PPV_ARGS(CommandAllocator.GetAddressOf())));
}

ImGuiD3D12FrameResource::~ImGuiD3D12FrameResource()
{
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

    /*DeviceResources->SRVDescriptorHeap = DeviceResources->CreateDescriptorHeap(
        DeviceResources->Device,
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
        1,
        D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
    );
    NAME_D3D12_OBJECT(DeviceResources->SRVDescriptorHeap);*/

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
    //commandList->SetDescriptorHeaps(1, DeviceResources->SRVDescriptorHeap.GetAddressOf());

    auto backBuffer = DeviceResources->BackBuffers[m_CurrentBackbufferIndex];

    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        backBuffer.Get(),
        D3D12_RESOURCE_STATE_PRESENT,
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
        D3D12_RESOURCE_BARRIER_FLAG_NONE);

    commandList->ResourceBarrier(1, &barrier);
}

void ImGuiGraphicsContext::Render(ImDrawData * drawData, ID3D12PipelineState* pipelineState, ID3D12RootSignature* rootSignature, ID3D12DescriptorHeap* srvHeap)
{
    // We are assuming that NewFrame has been called, therefore the queue has been flushed
    // So we are free to release the buffers IF needed.
    if (!m_CurrentFrameResource->VertexBuffer || m_CurrentFrameResource->VertexCount != drawData->TotalVtxCount)
    {
        if (m_CurrentFrameResource->VertexBuffer) { 
            m_CurrentFrameResource->VertexBufferUploader.Reset();
            m_CurrentFrameResource->VertexBuffer.Reset(); 
        }

        UINT64 bufferSize = drawData->TotalVtxCount * sizeof(ImDrawVert);
        m_CurrentFrameResource->VertexCount = drawData->TotalVtxCount;

        // Create the actual default buffer resource.
        ThrowIfFailed(DeviceResources->Device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(bufferSize),
            D3D12_RESOURCE_STATE_COMMON,
            nullptr,
            IID_PPV_ARGS(m_CurrentFrameResource->VertexBuffer.GetAddressOf())
        ));

        ThrowIfFailed(DeviceResources->Device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(bufferSize),
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(m_CurrentFrameResource->VertexBufferUploader.GetAddressOf())
        ));
    }

    if (!m_CurrentFrameResource->IndexBuffer || m_CurrentFrameResource->IndexCount != drawData->TotalIdxCount)
    {
        if (m_CurrentFrameResource->IndexBuffer) { 
            m_CurrentFrameResource->IndexBufferUploader.Reset();
            m_CurrentFrameResource->IndexBuffer.Reset(); 
        }

        UINT64 bufferSize = drawData->TotalIdxCount * sizeof(ImDrawIdx);

        m_CurrentFrameResource->IndexCount = drawData->TotalIdxCount;
        
        ThrowIfFailed(DeviceResources->Device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(bufferSize),
            D3D12_RESOURCE_STATE_COMMON,
            nullptr,
            IID_PPV_ARGS(m_CurrentFrameResource->IndexBuffer.GetAddressOf())
        ));

        ThrowIfFailed(DeviceResources->Device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(bufferSize),
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(m_CurrentFrameResource->IndexBufferUploader.GetAddressOf())
        ));
    }

    // Upload buffers have been created. Now to copy over the data

    // The *MappedData variables are ImDrawVert* and ImDrawIdx respetively.
    // We just treat them as bytes for now though.
    ImDrawVert* vertexMappedData;
    ImDrawIdx* indexMappedData;

    D3D12_RANGE range = { 0 };

    ThrowIfFailed(m_CurrentFrameResource->VertexBufferUploader->Map(0, nullptr, reinterpret_cast<void**>(&vertexMappedData)));
    ThrowIfFailed(m_CurrentFrameResource->IndexBufferUploader->Map(0, nullptr, reinterpret_cast<void**>(&indexMappedData)));

    ImDrawVert* vertexDestinationPtr = (ImDrawVert*)vertexMappedData;
    ImDrawIdx* indexDestinationPtr = (ImDrawIdx*)indexMappedData;
    for (int i = 0; i < drawData->CmdListsCount; ++i)
    {
        auto commandList = drawData->CmdLists[i];
        ::memcpy(vertexDestinationPtr, commandList->VtxBuffer.Data, commandList->VtxBuffer.Size * sizeof(ImDrawVert));
        ::memcpy(indexDestinationPtr, commandList->IdxBuffer.Data, commandList->IdxBuffer.Size * sizeof(ImDrawIdx));
        vertexDestinationPtr += commandList->VtxBuffer.Size;
        indexDestinationPtr += commandList->IdxBuffer.Size;
    }

    m_CurrentFrameResource->VertexBufferUploader->Unmap(0, &range);
    m_CurrentFrameResource->IndexBufferUploader->Unmap(0, &range);


    // Copy data from upload to default buffer
    {
        D3D12_RESOURCE_BARRIER barriersPre[2] = {
            CD3DX12_RESOURCE_BARRIER::Transition(
                m_CurrentFrameResource->VertexBuffer.Get(),
                D3D12_RESOURCE_STATE_COMMON,
                D3D12_RESOURCE_STATE_COPY_DEST
            ),
            CD3DX12_RESOURCE_BARRIER::Transition(
                m_CurrentFrameResource->IndexBuffer.Get(),
                D3D12_RESOURCE_STATE_COMMON,
                D3D12_RESOURCE_STATE_COPY_DEST
            )
        };

        DeviceResources->CommandList->ResourceBarrier(2, barriersPre);

        DeviceResources->CommandList->CopyResource(
            m_CurrentFrameResource->VertexBuffer.Get(),
            m_CurrentFrameResource->VertexBufferUploader.Get()
        );

        DeviceResources->CommandList->CopyResource(
            m_CurrentFrameResource->IndexBuffer.Get(),
            m_CurrentFrameResource->IndexBufferUploader.Get()
        );

        D3D12_RESOURCE_BARRIER barriersPost[2] = {
            CD3DX12_RESOURCE_BARRIER::Transition(
                m_CurrentFrameResource->VertexBuffer.Get(),
                D3D12_RESOURCE_STATE_COPY_DEST,
                D3D12_RESOURCE_STATE_GENERIC_READ
            ),
            CD3DX12_RESOURCE_BARRIER::Transition(
                m_CurrentFrameResource->IndexBuffer.Get(),
                D3D12_RESOURCE_STATE_COPY_DEST,
                D3D12_RESOURCE_STATE_GENERIC_READ
            )
        };

        DeviceResources->CommandList->ResourceBarrier(2, barriersPost);
    }
    // Setup orthographic projection matrix into our constant buffer
    // Our visible imgui space lies from draw_data->DisplayPos (top left) to draw_data->DisplayPos+data_data->DisplaySize (bottom right). DisplayMin is (0,0) for single viewport apps.
    VERTEX_CONSTANT_BUFFER vertex_constant_buffer;
    {
        VERTEX_CONSTANT_BUFFER* constant_buffer = &vertex_constant_buffer;
        float L = drawData->DisplayPos.x;
        float R = drawData->DisplayPos.x + drawData->DisplaySize.x;
        float T = drawData->DisplayPos.y;
        float B = drawData->DisplayPos.y + drawData->DisplaySize.y;
        float mvp[4][4] =
        {
            { 2.0f / (R - L),   0.0f,           0.0f,       0.0f },
            { 0.0f,         2.0f / (T - B),     0.0f,       0.0f },
            { 0.0f,         0.0f,           0.5f,       0.0f },
            { (R + L) / (L - R),  (T + B) / (B - T),    0.5f,       1.0f },
        };
        ::memcpy(&constant_buffer->mvp, mvp, sizeof(mvp));
    }

    // Set Viewport. Using the one that keeps getting resized by the resize callback
    // Perhaps we don't need to update its values here as well.
    {
        m_Viewport.Width = drawData->DisplaySize.x;
        m_Viewport.Height = drawData->DisplaySize.y;
        m_Viewport.MinDepth = 0.0f;
        m_Viewport.MaxDepth = 1.0f;
        m_Viewport.TopLeftX = m_Viewport.TopLeftY = 0.0f;

        DeviceResources->CommandList->RSSetViewports(1, &m_Viewport);
    }
    
    // Create buffer views and bind them
    {
        D3D12_VERTEX_BUFFER_VIEW vbv;
        memset(&vbv, 0, sizeof(D3D12_VERTEX_BUFFER_VIEW));
        vbv.BufferLocation = m_CurrentFrameResource
            ->VertexBuffer
            ->GetGPUVirtualAddress();
        vbv.SizeInBytes = m_CurrentFrameResource->VertexCount * sizeof(ImDrawVert);
        vbv.StrideInBytes = sizeof(ImDrawVert);
        DeviceResources->CommandList->IASetVertexBuffers(0, 1, &vbv);
        
        D3D12_INDEX_BUFFER_VIEW ibv;
        memset(&ibv, 0, sizeof(D3D12_INDEX_BUFFER_VIEW));
        ibv.BufferLocation = m_CurrentFrameResource
            ->IndexBuffer
            ->GetGPUVirtualAddress();
        ibv.SizeInBytes = m_CurrentFrameResource->IndexCount * sizeof(ImDrawIdx);
        ibv.Format = sizeof(ImDrawIdx) == 2 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
        DeviceResources->CommandList->IASetIndexBuffer(&ibv);
    }

    // Setup render state
    {
        DeviceResources->CommandList->SetDescriptorHeaps(1, &srvHeap);
        DeviceResources->CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        DeviceResources->CommandList->SetPipelineState(pipelineState);
        DeviceResources->CommandList->SetGraphicsRootSignature(rootSignature);
        DeviceResources->CommandList->SetGraphicsRoot32BitConstants(0, 16, &vertex_constant_buffer, 0);
        const float blend_factor[4] = { 0.f, 0.f, 0.f, 0.f };
        DeviceResources->CommandList->OMSetBlendFactor(blend_factor);
    }
    

    // Render command lists
    int vtx_offset = 0;
    int idx_offset = 0;
    ImVec2 pos = drawData->DisplayPos;
    for (int n = 0; n < drawData->CmdListsCount; n++)
    {
        const ImDrawList* cmd_list = drawData->CmdLists[n];
        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
        {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
            if (pcmd->UserCallback)
            {
                pcmd->UserCallback(cmd_list, pcmd);
            }
            else
            {
                const D3D12_RECT r = { (LONG)(pcmd->ClipRect.x - pos.x), (LONG)(pcmd->ClipRect.y - pos.y), (LONG)(pcmd->ClipRect.z - pos.x), (LONG)(pcmd->ClipRect.w - pos.y) };
                DeviceResources->CommandList->SetGraphicsRootDescriptorTable(1, *(D3D12_GPU_DESCRIPTOR_HANDLE*)&pcmd->TextureId);
                DeviceResources->CommandList->RSSetScissorRects(1, &r);
                DeviceResources->CommandList->DrawIndexedInstanced(pcmd->ElemCount, 1, idx_offset, vtx_offset, 0);
            }
            idx_offset += pcmd->ElemCount;
        }
        vtx_offset += cmd_list->VtxBuffer.Size;
    }
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
        FrameResources.push_back(std::make_unique<ImGuiD3D12FrameResource>(
            DeviceResources->Device,
            1
            ));
    }
}
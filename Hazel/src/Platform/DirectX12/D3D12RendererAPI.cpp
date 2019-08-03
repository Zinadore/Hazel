#include <hzpch.h>
#include "D3D12RendererAPI.h"

#include "Platform/DirectX12/D3D12Context.h"
#include "Platform/DirectX12/D3D12Helpers.h"

#include <glm/gtc/type_ptr.hpp>


namespace Hazel {
    D3D12RendererAPI::D3D12RendererAPI()
        : m_ClearColor(glm::vec4(0.0f))
    {
        
    }
    void D3D12RendererAPI::SetClearColor(const glm::vec4& color)
    {
        m_ClearColor = { color.r, color.g, color.b, color.a };
    }

    void D3D12RendererAPI::Clear()
    {
        D3D12_CPU_DESCRIPTOR_HANDLE rtv = ctx->CurrentBackBufferView();

        ctx->m_CommandList->ClearRenderTargetView(rtv, glm::value_ptr(m_ClearColor), 0, nullptr);
        ctx->m_CommandList->ClearDepthStencilView(ctx->DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
        
        ctx->m_CommandList->OMSetRenderTargets(1, &rtv, true, &ctx->DepthStencilView());

        ctx->m_CommandList->SetDescriptorHeaps(1, ctx->m_SRVDescriptorHeap.GetAddressOf());
    }

    void D3D12RendererAPI::DrawIndexed(const std::shared_ptr<VertexArray>& vertexArray)
    {

    }

    void D3D12RendererAPI::BeginFrame()
    {
        auto commandAllocator = ctx->m_CommandAllocators[ctx->m_CurrentBackbufferIndex];
        
        commandAllocator->Reset();
        ctx->m_CommandList->Reset(commandAllocator.Get(), nullptr);

        auto backBuffer = ctx->m_BackBuffers[ctx->m_CurrentBackbufferIndex];

        CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            backBuffer.Get(),
            D3D12_RESOURCE_STATE_PRESENT, 
            D3D12_RESOURCE_STATE_RENDER_TARGET,
            D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, 
            D3D12_RESOURCE_BARRIER_FLAG_NONE);

        ctx->m_CommandList->ResourceBarrier(1, &barrier);

        ctx->m_CommandList->RSSetViewports(1, &ctx->m_Viewport);
    }

    void D3D12RendererAPI::EndFrame()
    {
        m_Context->SwapBuffers();
    }

    // NOT: The context has access to the new window size through the window itself
    void D3D12RendererAPI::ResizeResources()
    {
        ctx->CleanupRenderTargetViews();
        ctx->ResizeSwapChain();
        ctx->UpdateRenderTargetViews(ctx->m_Device, ctx->m_SwapChain, ctx->m_RTVDescriptorHeap);
        ctx->CreateDepthStencil();
    }

    void D3D12RendererAPI::OnChangeContext()
    {
        ctx = dynamic_cast<D3D12Context*>(m_Context);
        HZ_CORE_ASSERT(ctx, "Context was of wrong type");
    }



}
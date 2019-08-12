#include "hzpch.h"
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
        ctx->Clear(glm::value_ptr(m_ClearColor));
    }

    void D3D12RendererAPI::DrawIndexed(const std::shared_ptr<VertexArray>& vertexArray)
    {

    }

    void D3D12RendererAPI::BeginFrame()
    {
        ctx->NewFrame();
    }

    void D3D12RendererAPI::EndFrame()
    {
        m_Context->SwapBuffers();
    }

    void D3D12RendererAPI::Flush()
    {
        ctx->Flush();
    }

    // NOT: The context has access to the new window size through the window itself
    void D3D12RendererAPI::ResizeResources()
    {
        auto r = ctx->DeviceResources;
        ctx->CleanupRenderTargetViews();
        ctx->ResizeSwapChain();
        ctx->CreateRenderTargetViews();
        ctx->CreateDepthStencil();

        // Transition the DepthStencilBuffer
        auto dsBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
            r->DepthStencilBuffer.Get(),
            D3D12_RESOURCE_STATE_COMMON,
            D3D12_RESOURCE_STATE_DEPTH_WRITE
        );

        r->CommandList->ResourceBarrier(1, &dsBarrier);

        // Execute all the resize magic
        ThrowIfFailed(r->CommandList->Close());

        ID3D12CommandList* const commandLists[] = {
            r->CommandList.Get()
        };
        r->CommandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);
    }

    void D3D12RendererAPI::OnChangeContext()
    {
        ctx = dynamic_cast<D3D12Context*>(m_Context);
        HZ_CORE_ASSERT(ctx, "Context was of wrong type");
    }



}
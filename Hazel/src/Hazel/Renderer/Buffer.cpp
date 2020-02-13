#include "hzpch.h"
#include "Buffer.h"

#include "Renderer.h"

#include "Hazel/Core/Application.h"

#include "Platform/OpenGL/OpenGLBuffer.h"
#include "Platform/DirectX12/D3D12Buffer.h"
#include "Platform/DirectX12/D3D12Context.h"

namespace Hazel {

	VertexBuffer* VertexBuffer::Create(float* vertices, uint32_t size)
	{
		switch (Renderer::GetAPI())
		{
			case RendererAPI::API::None:    HZ_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
			case RendererAPI::API::OpenGL:  return new OpenGLVertexBuffer(vertices, size);
            case RendererAPI::API::D3D12: {
                auto ctx = dynamic_cast<D3D12Context*>(Application::Get().GetWindow().GetContext());
                HZ_CORE_ASSERT(ctx, "Rendering context was not of type D3D12Context");

                return new D3D12VertexBuffer(ctx->DeviceResources->Device, vertices, size);
            }
		}

		HZ_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

	IndexBuffer* IndexBuffer::Create(uint32_t* indices, uint32_t size)
	{
		switch (Renderer::GetAPI())
		{
			case RendererAPI::API::None:    HZ_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
			case RendererAPI::API::OpenGL:  return new OpenGLIndexBuffer(indices, size);
            case RendererAPI::API::D3D12: {
                auto ctx = dynamic_cast<D3D12Context*>(Application::Get().GetWindow().GetContext());
                HZ_CORE_ASSERT(ctx, "Rendering context was not of type D3D12Context");

                return new D3D12IndexBuffer(ctx->DeviceResources->Device, indices, size);
            }
		}

		HZ_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

}
#include "hzpch.h"

#include "GraphicsContext.h"

#include "Hazel/Renderer/Renderer.h"
#include "Hazel/Renderer/RendererAPI.h"
#include "Hazel/Window.h"

#include "Platform/OpenGL/OpenGLContext.h"
#include "Platform/DirectX12/D3D12Context.h"

namespace Hazel {
    GraphicsContext::GraphicsContext(Window * window)
        :m_Window(window)
    {
    }
    GraphicsContext* GraphicsContext::Create(Window* window)
    {
        switch (Renderer::GetAPI())
        {
        case RendererAPI::API::None:    HZ_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
        case RendererAPI::API::OpenGL:  return new OpenGLContext(window);
        case RendererAPI::API::D3D12:   return new D3D12Context(window);
        }

        HZ_CORE_ASSERT(false, "Unknown RendererAPI!");
        return nullptr;
    }
}
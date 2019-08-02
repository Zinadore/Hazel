#include <hzpch.h>

#include "RendererAPI.h"

#include "Platform/OpenGL/OpenGLRendererAPI.h"
#include "Platform/DirectX12/D3D12RendererAPI.h"

#include "Hazel/Renderer/RenderCommand.h"

namespace Hazel {
    RendererAPI::API RendererAPI::s_API = API::None;
    RendererAPI* RendererAPI::s_Instance = nullptr;

    void RendererAPI::SetGraphicsContext(GraphicsContext * context)
    {
        HZ_CORE_ASSERT(context, "Grahics contex is null!!");
        s_Instance->m_Context = context;
        s_Instance->OnChangeContext();
    }

    void RendererAPI::SelectAPI(API api)
    {
        if (s_Instance != nullptr) {
            HZ_CORE_ASSERT(false, "The renderer API is already initialized!");
            return;
        }

        switch (api)
        {
        case API::None:    HZ_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); break;
        case API::OpenGL:  s_Instance = new OpenGLRendererAPI(); break;
        case API::D3D12:   s_Instance = new D3D12RendererAPI(); break;
        default: HZ_CORE_ASSERT(false, "Unknown RendererAPI!"); break;
        }


        s_API = api;
        RenderCommand::s_RendererAPI = s_Instance;
    }

}
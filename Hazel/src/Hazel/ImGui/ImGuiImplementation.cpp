#include "hzpch.h"
#include "ImGuiImplementation.h"
#include "imgui.h"

#include "Platform/OpenGL/ImGui/OpenGLImGuiImplementation.h"
#include "Platform/DirectX12/ImGui/D3D12ImGuiImplementation.h"

#include "Hazel/Renderer/Renderer.h"
#include "Hazel/Renderer/RendererAPI.h"

namespace Hazel {
    ImGuiImplementation* ImGuiImplementation::Create() 
    {
        switch (Renderer::GetAPI())
        {
        case RendererAPI::API::None:    HZ_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
        case RendererAPI::API::OpenGL:  return new OpenGLImGuiImplementation();
        case RendererAPI::API::D3D12:   return D3D12ImGuiImplementation::Create();
        }

        HZ_CORE_ASSERT(false, "Unknown RendererAPI!");
        return nullptr;
    }
}

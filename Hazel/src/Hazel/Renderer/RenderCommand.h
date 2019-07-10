#pragma once

#include "RendererAPI.h"

namespace Hazel {
    HAZEL_API class RenderCommand {
    public:

        inline static void Clear() { s_RendererAPI->Clear(); }

        inline static void SetClearColor(const glm::vec4& color) { s_RendererAPI->SetClearColor(color); }

        inline static void DrawIndexed(const std::shared_ptr<VertexArray>& vertexArray)
        {
            s_RendererAPI->DrawIndexed(vertexArray);
        }

        inline static void BeginFrame() { s_RendererAPI->BeginFrame(); }

        inline static void EndFrame() { s_RendererAPI->EndFrame();  }
    private:
        static RendererAPI* s_RendererAPI;

        friend void RendererAPI::SelectAPI(RendererAPI::API api);
    };
}
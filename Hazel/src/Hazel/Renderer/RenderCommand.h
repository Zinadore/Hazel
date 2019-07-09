#pragma once

#include "RendererAPI.h"

namespace Hazel {
    HAZEL_API class RenderCommand {
    public:

        static void Clear() { s_RendererAPI->Clear(); }

        static void SetClearColor(const glm::vec4& color) { s_RendererAPI->SetClearColor(color); }

        static void DrawIndexed(const std::shared_ptr<VertexArray>& vertexArray)
        {
            s_RendererAPI->DrawIndexed(vertexArray);
        }
    private:
        static RendererAPI* s_RendererAPI;

        friend void RendererAPI::SelectAPI(RendererAPI::API api);
    };
}
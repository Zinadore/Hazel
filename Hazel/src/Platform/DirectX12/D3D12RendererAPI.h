#pragma once
#include "Hazel/Renderer/RendererAPI.h"
#include <glm/vec4.hpp>

namespace Hazel {
    class D3D12RendererAPI : public RendererAPI
    {
    public:

        D3D12RendererAPI();
        virtual void SetClearColor(const glm::vec4& color) override;
        virtual void Clear() override;

        virtual void DrawIndexed(const std::shared_ptr<VertexArray>& vertexArray) override;

        virtual void BeginFrame() override;
        virtual void EndFrame() override;

    private:
        glm::vec4 m_ClearColor;
    };
}
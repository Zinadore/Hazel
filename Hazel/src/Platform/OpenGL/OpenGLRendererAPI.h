#pragma once

#include "Hazel/Renderer/RendererAPI.h"

namespace Hazel {

	class OpenGLRendererAPI : public RendererAPI
	{
	public:
        OpenGLRendererAPI();

		virtual void SetClearColor(const glm::vec4& color) override;
		virtual void Clear() override;

		virtual void DrawIndexed(const std::shared_ptr<VertexArray>& vertexArray) override;

        virtual void BeginFrame() override;
        virtual void EndFrame() override;

        virtual void ResizeResources() override;

    protected:
        virtual void OnChangeContext() override;
	};


}

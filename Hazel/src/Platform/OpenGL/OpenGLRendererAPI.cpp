#include "hzpch.h"
#include "OpenGLRendererAPI.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "Hazel/Core/Application.h"
namespace Hazel {
    OpenGLRendererAPI::OpenGLRendererAPI()
    {
    }

    void OpenGLRendererAPI::SetClearColor(const glm::vec4& color)
	{
		glClearColor(color.r, color.g, color.b, color.a);
	}

	void OpenGLRendererAPI::Clear()
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	void OpenGLRendererAPI::DrawIndexed(const std::shared_ptr<VertexArray>& vertexArray)
	{
		glDrawElements(GL_TRIANGLES, vertexArray->GetIndexBuffer()->GetCount(), GL_UNSIGNED_INT, nullptr);
	}

    void OpenGLRendererAPI::BeginFrame()
    {
    }

    void OpenGLRendererAPI::EndFrame()
    {
        glfwSwapBuffers((GLFWwindow*)Application::Get().GetWindow().GetNativeWindow());
    }

    void OpenGLRendererAPI::Flush()
    {
    }

    void OpenGLRendererAPI::ResizeResources()
    {
        auto& wnd = Application::Get().GetWindow();
        glViewport(0, 0, wnd.GetWidth(), wnd.GetHeight());
    }

    void OpenGLRendererAPI::OnChangeContext()
    {
    }

}
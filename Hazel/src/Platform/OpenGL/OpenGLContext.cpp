#include "hzpch.h"
#include "OpenGLContext.h"

#include "Hazel/Core/Window.h"

#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <GL/GL.h>

namespace Hazel {

	OpenGLContext::OpenGLContext(Window* window)
		: GraphicsContext(window)
	{
        HZ_CORE_ASSERT(window, "Window is null!");

        m_WindowHandle = (GLFWwindow*)window->GetNativeWindow();
        HZ_CORE_ASSERT(m_WindowHandle, "Window handle is null");
	}

	void OpenGLContext::Init()
	{
		glfwMakeContextCurrent(m_WindowHandle);
		int status = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
		HZ_CORE_ASSERT(status, "Failed to initialize Glad!");

		HZ_CORE_INFO("OpenGL Info:");
		HZ_CORE_INFO("  Vendor: {0}", glGetString(GL_VENDOR));
		HZ_CORE_INFO("  Renderer: {0}", glGetString(GL_RENDERER));
		HZ_CORE_INFO("  Version: {0}", glGetString(GL_VERSION));

	}

	void OpenGLContext::SwapBuffers()
	{
		glfwSwapBuffers(m_WindowHandle);
	}

    void OpenGLContext::SetVSync(bool enabled)
    {
        m_VSyncEnabled = enabled;
        if (enabled)
            glfwSwapInterval(1);
        else
            glfwSwapInterval(0);
    }

    void OpenGLContext::NewFrame()
    {
    }

}
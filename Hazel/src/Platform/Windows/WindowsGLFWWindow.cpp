#include "hzpch.h"
#include "WindowsGLFWWindow.h"
#ifdef HZ_PLATFORM_WINDOWS
#include "Win32Window.h"
#endif // HZ_PLATFORM_WINDOWS


#include "Hazel/Events/ApplicationEvent.h"
#include "Hazel/Events/MouseEvent.h"
#include "Hazel/Events/KeyEvent.h"
#include "Hazel/Renderer/Renderer.h"
#include "Hazel/Renderer/RendererAPI.h"




namespace Hazel {
	
	static bool s_GLFWInitialized = false;

	static void GLFWErrorCallback(int error, const char* description)
	{
		HZ_CORE_ERROR("GLFW Error ({0}): {1}", error, description);
	}
    // TODO: Move this method to some other cpp file
	Window* Window::Create(const WindowProps& props)
	{
        switch (Renderer::GetAPI())
        {
        case RendererAPI::API::None:    HZ_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
        case RendererAPI::API::OpenGL:  return new WindowsGLFWWindow(props);
        case RendererAPI::API::D3D12: {
#ifdef HZ_PLATFORM_WINDOWS
            return new Win32Window(props);
#else
            HZ_CORE_ASSERT(false, "Using D3D12 on a non-windows build is not supported!");
#endif
        }
        }

        HZ_CORE_ASSERT(false, "Unknown RendererAPI!");
        
        return nullptr;
	}

	WindowsGLFWWindow::WindowsGLFWWindow(const WindowProps& props)
	{
		Init(props);
	}

	WindowsGLFWWindow::~WindowsGLFWWindow()
	{
		Shutdown();
	}

	void WindowsGLFWWindow::Init(const WindowProps& props)
	{
		m_Data.Title = props.Title;
		m_Data.Width = props.Width;
		m_Data.Height = props.Height;

		HZ_CORE_INFO("Creating window {0} ({1}, {2})", props.Title, props.Width, props.Height);

		if (!s_GLFWInitialized)
		{
			// TODO: glfwTerminate on system shutdown
			int success = glfwInit();
			HZ_CORE_ASSERT(success, "Could not intialize GLFW!");
			glfwSetErrorCallback(GLFWErrorCallback);
			s_GLFWInitialized = true;
		}

        //// Disable rendering API if we are not using OpenGL
        //if (Renderer::GetAPI() != RendererAPI::API::OpenGL) 
        //{
        //    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        //}

		m_Window = glfwCreateWindow((int)props.Width, (int)props.Height, m_Data.Title.c_str(), nullptr, nullptr);

        m_Context = GraphicsContext::Create(this);
		m_Context->Init(props.Width, props.Height);
        RendererAPI::SetGraphicsContext(m_Context);

		glfwSetWindowUserPointer(m_Window, &m_Data);
		SetVSync(true);

		// Set GLFW callbacks
		glfwSetWindowSizeCallback(m_Window, [](GLFWwindow* window, int width, int height)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
			data.Width = width;
			data.Height = height;

			WindowResizeEvent event(width, height);
			data.EventCallback(event);
		});

		glfwSetWindowCloseCallback(m_Window, [](GLFWwindow* window)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
			WindowCloseEvent event;
			data.EventCallback(event);
		});

		glfwSetKeyCallback(m_Window, [](GLFWwindow* window, int key, int scancode, int action, int mods)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			switch (action)
			{
				case GLFW_PRESS:
				{
					KeyPressedEvent event(key, 0);
					data.EventCallback(event);
					break;
				}
				case GLFW_RELEASE:
				{
					KeyReleasedEvent event(key);
					data.EventCallback(event);
					break;
				}
				case GLFW_REPEAT:
				{
					KeyPressedEvent event(key, 1);
					data.EventCallback(event);
					break;
				}
			}
		});

		glfwSetCharCallback(m_Window, [](GLFWwindow* window, unsigned int keycode)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			KeyTypedEvent event(keycode);
			data.EventCallback(event);
		});

		glfwSetMouseButtonCallback(m_Window, [](GLFWwindow* window, int button, int action, int mods)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			switch (action)
			{
				case GLFW_PRESS:
				{
					MouseButtonPressedEvent event(button);
					data.EventCallback(event);
					break;
				}
				case GLFW_RELEASE:
				{
					MouseButtonReleasedEvent event(button);
					data.EventCallback(event);
					break;
				}
			}
		});

		glfwSetScrollCallback(m_Window, [](GLFWwindow* window, double xOffset, double yOffset)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			MouseScrolledEvent event((float)xOffset, (float)yOffset);
			data.EventCallback(event);
		});

		glfwSetCursorPosCallback(m_Window, [](GLFWwindow* window, double xPos, double yPos)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			MouseMovedEvent event((float)xPos, (float)yPos);
			data.EventCallback(event);
		});
	}

	void WindowsGLFWWindow::Shutdown()
	{
		glfwDestroyWindow(m_Window);
	}

	void WindowsGLFWWindow::OnUpdate()
	{
		glfwPollEvents();
	}

	void WindowsGLFWWindow::SetVSync(bool enabled)
	{
        m_Context->SetVSync(enabled);
		m_Data.VSync = enabled;
	}

	bool WindowsGLFWWindow::IsVSync() const
	{
		return m_Data.VSync;
	}

}

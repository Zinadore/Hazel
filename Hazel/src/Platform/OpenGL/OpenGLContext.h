#pragma once

#include "Hazel/Renderer/GraphicsContext.h"

class Hazel::Window;
struct GLFWwindow;

namespace Hazel {

	class OpenGLContext : public GraphicsContext
	{
	public:
		OpenGLContext(Window* window);

		virtual void Init() override;
		virtual void SwapBuffers() override;
        virtual void SetVSync(bool enabled) override;
	private:
		GLFWwindow* m_WindowHandle;
	};

}
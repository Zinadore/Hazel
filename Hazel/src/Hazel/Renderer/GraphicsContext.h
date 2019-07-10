#pragma once

struct GLFWwindow;

namespace Hazel {

	class GraphicsContext
	{
	public:
		virtual void Init(unsigned int width, unsigned int height) = 0;
		virtual void SwapBuffers() = 0;

        static GraphicsContext* Create(GLFWwindow* window);
	};

}
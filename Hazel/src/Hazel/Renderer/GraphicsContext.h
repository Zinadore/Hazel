#pragma once

struct GLFWwindow;

namespace Hazel {

	class GraphicsContext
	{
    protected:
        unsigned int m_ClientWidth;
        unsigned int m_ClientHeight;
        bool m_VSyncEnabled;
        bool m_TearingSupported = false;

	public:
		virtual void Init(unsigned int width, unsigned int height) = 0;
        virtual void SetVSync(bool enabled) = 0;
        virtual void SwapBuffers() = 0;


        static GraphicsContext* Create(GLFWwindow* window);
	};

}
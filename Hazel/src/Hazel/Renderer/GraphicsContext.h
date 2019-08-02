#pragma once


namespace Hazel {
    class Window;
	
    class GraphicsContext
	{
    protected:
        Window* m_Window;
        unsigned int m_ClientWidth;
        unsigned int m_ClientHeight;
        bool m_VSyncEnabled;
        bool m_TearingSupported = false;

        GraphicsContext(Window* window);

	public:
		virtual void Init(unsigned int width, unsigned int height) = 0;
        virtual void SetVSync(bool enabled) = 0;
        virtual void SwapBuffers() = 0;

        UINT inline GetWidth() const { return m_ClientWidth; }
        UINT inline GetHeight() const { return m_ClientHeight; }


        static GraphicsContext* Create(Window* window);
	};

}
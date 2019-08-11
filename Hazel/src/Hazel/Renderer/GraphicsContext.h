#pragma once


namespace Hazel {
    class Window;
	
    class GraphicsContext
	{
    protected:
        Window* m_Window;
        bool m_VSyncEnabled;
        bool m_TearingSupported = false;

        GraphicsContext(Window* window);

	public:
		virtual void Init() = 0;
        virtual void SetVSync(bool enabled) = 0;
        virtual void NewFrame() = 0;
        virtual void SwapBuffers() = 0;

        static GraphicsContext* Create(Window* window);
	};

}
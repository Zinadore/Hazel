#pragma once
#include <Hazel/Window.h>

#if defined(HZ_PLATFORM_WINDOWS)
#include <windows.h>
namespace Hazel {


    class Win32Window : public Window
    {
    public:
        Win32Window(const WindowProps& props);
        ~Win32Window();

        void OnUpdate() override;

        inline unsigned int GetWidth() const override { return m_Data.Width; }
        inline unsigned int GetHeight() const override { return m_Data.Height; }

        // Window attributes
        inline void SetEventCallback(const EventCallbackFn& callback) override { m_Data.EventCallback = callback; }
        void SetVSync(bool enabled) override;
        bool IsVSync() const override;

        inline virtual void* GetNativeWindow() const { return m_Window; }
        
        LRESULT CALLBACK MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    private:
        virtual void Init(const WindowProps& props);
        virtual void Shutdown();


    private:
        HWND m_Window;

        struct WindowData
        {
            std::string Title;
            unsigned int Width, Height;
            bool VSync;

            EventCallbackFn EventCallback;
        };

        WindowData m_Data;

        static HWND CreateWindowHelper(WindowData& data);

    };


}
#endif
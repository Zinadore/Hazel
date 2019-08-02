#include "hzpch.h"

#include "Win32Window.h"

#include "Hazel/Renderer/RendererAPI.h"

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace Hazel {
    static LRESULT WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        Win32Window* wnd = (Win32Window*)(::GetWindowLongPtr(hWnd, GWLP_USERDATA));
        if (!wnd) return ::DefWindowProc(hWnd, msg, wParam, lParam);

        return wnd->MsgProc(hWnd, msg, wParam, lParam);
    }

    HWND Win32Window::CreateWindowHelper(WindowData& data)
    {
        std::wstring className = std::wstring(data.Title.begin(), data.Title.end());
        HINSTANCE hInstance = GetModuleHandle(NULL);
        int screenWidth = GetSystemMetrics(SM_CXSCREEN);
        int screenHeight = GetSystemMetrics(SM_CYSCREEN);

        WNDCLASS WindowClass;
        WindowClass.style = CS_HREDRAW | CS_VREDRAW;
        WindowClass.lpfnWndProc = Hazel::WindowProc;
        WindowClass.cbClsExtra = 0;
        WindowClass.cbWndExtra = 0;
        WindowClass.hInstance = hInstance;
        WindowClass.hIcon = LoadIcon(0, IDI_APPLICATION);
        WindowClass.hCursor = LoadCursor(0, IDC_ARROW);
        WindowClass.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
        WindowClass.lpszMenuName = 0;
        WindowClass.lpszClassName = className.c_str();

        if (!RegisterClass(&WindowClass)) {
            HZ_CORE_ASSERT(false, "Could not register window class");
            return nullptr;
        }

        RECT rect = { 0, 0, data.Width, data.Height };
        ::AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, false);

        int width = rect.right - rect.left;
        int height = rect.bottom - rect.top;

        auto hwnd = CreateWindow(
            className.c_str(),
            className.c_str(),
            WS_OVERLAPPEDWINDOW,
            screenWidth / 2 - width / 2,
            screenHeight / 2 - height / 2,
            width,
            height,
            nullptr,
            nullptr,
            hInstance,
            0
        );

        if (!hwnd) {
            HZ_CORE_ASSERT(false, "Unable to create Win32 window");
            ::UnregisterClass(className.c_str(), hInstance);
            return nullptr;
        }

        return hwnd;
    }

    Win32Window::Win32Window(const WindowProps& props)
    {
        Init(props);
    }

    Win32Window::~Win32Window()
    {
        Shutdown();
    }

    void Win32Window::OnUpdate()
    {
        MSG msg = { 0 };
        while (true) 
        {
            if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
            else 
            {
                break;
            }
        }
    }

    void Win32Window::SetVSync(bool enabled)
    {
        m_Context->SetVSync(enabled);
        m_Data.VSync = enabled;
    }

    bool Win32Window::IsVSync() const
    {
        return m_Data.VSync;
    }

    LRESULT Win32Window::MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
            return true;

        static bool resizing = false;
        
        switch (msg)
        {
        case WM_SIZE: {
            m_Data.Width = LOWORD(lParam);
            m_Data.Height = HIWORD(lParam);

            if (!resizing) {
                OnResize();
            }

            return 0;
        }
        case WM_CLOSE: {
            Shutdown();
            return 0;
        }
        case WM_ENTERSIZEMOVE: {
            resizing = true;
            return 0;
        }

        case WM_EXITSIZEMOVE: {
            resizing = false;
            OnResize();
            return 0;
        }
        case WM_MENUCHAR:
            // Don't beep when we alt-enter.
            return MAKELRESULT(0, MNC_CLOSE);

        case WM_LBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_RBUTTONDOWN:
        }

        return ::DefWindowProc(hWnd, msg, wParam, lParam);
    }

    void Win32Window::Init(const WindowProps& props)
    {
        m_Data.Title = props.Title;
        m_Data.Width = props.Width;
        m_Data.Height = props.Height;
        

        HZ_CORE_INFO("Creating window {0} ({1}, {2})", props.Title, props.Width, props.Height);
    
        m_Window = CreateWindowHelper(m_Data);

        ::ShowWindow(m_Window, SW_SHOW);
        ::UpdateWindow(m_Window);

        m_Context = GraphicsContext::Create(this);
        m_Context->Init(props.Width, props.Height);
        RendererAPI::SetGraphicsContext(m_Context);

        ::SetWindowLongPtr(m_Window, GWLP_USERDATA, (LONG_PTR)this);
        SetVSync(false);
    }

    void Win32Window::Shutdown()
    {
        ::PostQuitMessage(0);
    }
}

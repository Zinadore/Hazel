#pragma once

#include "Hazel/Core/Core.h"
#include "Hazel/Core/Window.h"
#include "Hazel/Core/LayerStack.h"
#include "Hazel/Core/Timestep.h"
#include "Hazel/Events/Event.h"
#include "Hazel/Events/ApplicationEvent.h"


#include "Hazel/ImGui/ImGuiLayer.h"
#include "Hazel/Renderer/RendererAPI.h"


namespace Hazel {

    class HAZEL_API Application
    {
    public:
        Application(RendererAPI::API api = RendererAPI::API::None);
        virtual ~Application() = default;

        void Run();

        void OnEvent(Event& e);

        void PushLayer(Layer* layer);
        void PushOverlay(Layer* layer);

        inline Window& GetWindow() { return *m_Window; }

        inline static Application& Get() { return *s_Instance; }
    protected:
        void SetRendererAPI(RendererAPI::API api);

    private:
        bool OnWindowClose(WindowCloseEvent& e);
        bool OnWindowResize(WindowResizeEvent& e);

        std::unique_ptr<Window> m_Window;
        ImGuiLayer* m_ImGuiLayer;
        bool m_Running = true;
        LayerStack m_LayerStack;
        float m_LastFrameTime = 0.0f;

    private:
        static Application* s_Instance;
    };

    // To be defined in CLIENT
    Application* CreateApplication();

}
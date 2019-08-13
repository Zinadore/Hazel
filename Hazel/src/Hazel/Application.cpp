#include "hzpch.h"
#include "Application.h"

#include "Hazel/Log.h"

#include <Hazel/Renderer/Renderer.h>
#include <Hazel/Renderer/RenderCommand.h>
#include <Hazel/Input.h>

#include <Platform/DirectX12/D3D12Context.h>

#include <glfw/glfw3.h>

#define USE_IMGUI 1
namespace Hazel {

#define BIND_EVENT_FN(x) std::bind(&Application::x, this, std::placeholders::_1)

    Application* Application::s_Instance = nullptr;

    Application::Application(RendererAPI::API api)
    {
        HZ_CORE_ASSERT(!s_Instance, "Application already exists!");
        s_Instance = this;
        SetRendererAPI(api);
        m_Window = std::unique_ptr<Window>(Window::Create());
        m_Window->SetEventCallback(BIND_EVENT_FN(OnEvent));
#if USE_IMGUI
        m_ImGuiLayer = new ImGuiLayer();
        PushOverlay(m_ImGuiLayer);
#endif
    }

    void Application::PushLayer(Layer* layer)
    {
        m_LayerStack.PushLayer(layer);
    }

    void Application::PushOverlay(Layer* layer)
    {
        m_LayerStack.PushOverlay(layer);
    }

    void Application::OnEvent(Event& e)
    {
        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<WindowCloseEvent>(BIND_EVENT_FN(OnWindowClose));

        for (auto it = m_LayerStack.end(); it != m_LayerStack.begin(); )
        {
            (*--it)->OnEvent(e);
            if (e.Handled)
                break;
        }

        dispatcher.Dispatch<WindowResizeEvent>(BIND_EVENT_FN(OnWindowResize));
    }

    void Application::Run()
    {
        while (m_Running)
        {
            m_Window->OnUpdate();

            float time = (float)glfwGetTime();
            Timestep timestep = time - m_LastFrameTime;
            m_LastFrameTime = time;

            RenderCommand::BeginFrame();

            for (Layer* layer : m_LayerStack)
                layer->OnUpdate(timestep);
#if USE_IMGUI
            m_ImGuiLayer->Begin();
            for (Layer* layer : m_LayerStack)
                layer->OnImGuiRender();
            m_ImGuiLayer->End();
#endif
            RenderCommand::EndFrame();
#if USE_IMGUI
            m_ImGuiLayer->UpdateDockedWindows();
            m_ImGuiLayer->RenderDockedWindows();
#endif
        }
    }

    void Application::SetRendererAPI(RendererAPI::API api)
    {
        RendererAPI::SelectAPI(api);
    }

    bool Application::OnWindowClose(WindowCloseEvent& e)
    {
        m_Running = false;
        return true;
    }

    bool Application::OnWindowResize(WindowResizeEvent & e)
    {

        RenderCommand::Flush();
#if USE_IMGUI
        m_ImGuiLayer->OnResizeBegin();
#endif
        RenderCommand::ResizeResources();
#if USE_IMGUI
        m_ImGuiLayer->OnResizeEnd();
#endif
        return false;
    }

}
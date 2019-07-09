#pragma once
#include "Hazel/Renderer/RendererAPI.h"

#ifdef HZ_PLATFORM_WINDOWS

extern Hazel::Application* Hazel::CreateApplication();

int main(int argc, char** argv)
{
    Hazel::Log::Init();
    HZ_CORE_WARN("Initialized Log!");

    auto app = Hazel::CreateApplication();

    if (Hazel::RendererAPI::IsInitialized())
    {
        app->Run();
    }
    else
    {
        HZ_CORE_ERROR(false, "Application was created without initializing the renderer api!");
    }
    delete app;
}

#endif
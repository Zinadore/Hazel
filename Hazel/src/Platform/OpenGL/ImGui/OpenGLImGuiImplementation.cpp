#include "hzpch.h"

#include "imgui.h"
#define IMGUI_IMPL_OPENGL_LOADER_GLAD
#include "examples/imgui_impl_glfw.h"
#include "examples/imgui_impl_opengl3.h"
#include "examples/imgui_impl_glfw.cpp"
#include "examples/imgui_impl_opengl3.cpp"

#include <GLFW/glfw3.h>

#include "OpenGLImGuiImplementation.h"

void Hazel::OpenGLImGuiImplementation::Init(Window& window)
{
    auto w = static_cast<GLFWwindow*>(window.GetNativeWindow());
    // Setup Platform/Renderer bindings
    ImGui_ImplGlfw_InitForOpenGL(w, true);
    ImGui_ImplOpenGL3_Init("#version 410");
}

void Hazel::OpenGLImGuiImplementation::UpdateDockedWindows()
{
    GLFWwindow* backup_current_context = glfwGetCurrentContext();
    ImGui::UpdatePlatformWindows();
    ImGui::RenderPlatformWindowsDefault();
    glfwMakeContextCurrent(backup_current_context);
}

void Hazel::OpenGLImGuiImplementation::RenderDrawData(ImDrawData* drawData)
{
    ImGui_ImplOpenGL3_RenderDrawData(drawData);
}

void Hazel::OpenGLImGuiImplementation::NewFrame()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
}

void Hazel::OpenGLImGuiImplementation::Shutdown()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
}

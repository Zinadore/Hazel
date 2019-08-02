#pragma once

#include "Hazel/ImGui/ImGuiImplementation.h"

namespace Hazel {

    class OpenGLImGuiImplementation: public ImGuiImplementation
    {
    public:
        virtual void Init(Window& window) override;

        virtual void RenderDrawData(ImDrawData* drawData) override;

        virtual void NewFrame() override;

        virtual void UpdateDockedWindows() override;

        virtual void OnResize(unsigned int width, unsigned int height) override;

        virtual void Shutdown() override;


    };

}


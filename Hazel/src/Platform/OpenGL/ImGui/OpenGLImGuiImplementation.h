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

        virtual void Shutdown() override;

        virtual void RecreateResources() override;

        virtual void InvalidateResources() override;

    };

}


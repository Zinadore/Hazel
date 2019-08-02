#pragma once

#include "Hazel/Window.h"

struct ImDrawData;

namespace Hazel {

class ImGuiImplementation
{
public:
    virtual void Init(Window& window) = 0;
    virtual void RenderDrawData(ImDrawData* drawData) = 0;
    virtual void NewFrame() = 0;
    virtual void UpdateDockedWindows() = 0;
    virtual void Shutdown() = 0;
    virtual void OnResize(unsigned int width, unsigned int height) = 0;

    static ImGuiImplementation* Create();
};

}


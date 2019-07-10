#pragma once

#include "Hazel/ImGui/ImGuiImplementation.h"
#include "Platform/DirectX12/D3D12Context.h"

namespace Hazel {
    class D3D12ImGuiImplementation : public ImGuiImplementation
    {

    public:
        virtual void Init(Window& window) override;


        virtual void RenderDrawData(ImDrawData* drawData) override;


        virtual void NewFrame() override;


        virtual void Shutdown() override;


        virtual void UpdateDockedWindows() override;

    private:
        D3D12Context* ctx;

    };
}



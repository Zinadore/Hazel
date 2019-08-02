#pragma once

#include "Hazel/ImGui/ImGuiImplementation.h"
#include "Platform/DirectX12/D3D12Context.h"

#include <windows.h>

namespace Hazel {
    class D3D12ImGuiImplementation : public ImGuiImplementation
    {

    public:
        virtual void Init(Window& window) override;


        virtual void RenderDrawData(ImDrawData* drawData) override;


        virtual void NewFrame() override;


        virtual void Shutdown() override;


        virtual void UpdateDockedWindows() override;

        virtual void OnResize(unsigned int width, unsigned int height) override;
        
        static D3D12ImGuiImplementation* Create();

    private:
        D3D12Context* ctx;
        static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
        static D3D12ImGuiImplementation* sInstance;

    };
}



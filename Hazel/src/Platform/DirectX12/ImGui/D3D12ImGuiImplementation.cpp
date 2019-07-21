#include "hzpch.h"
#include "D3D12ImGuiImplementation.h"

#include "imgui.h"
#include "examples/imgui_impl_win32.h"
#include "examples/imgui_impl_dx12.h"
#include "examples/imgui_impl_win32.cpp"
#include "examples/imgui_impl_dx12.cpp"

#include "Platform/DirectX12/D3D12Context.h"

#include <GLFW/glfw3.h>


namespace Hazel {
    
    static LRESULT (*originalWindowProc)(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) = nullptr;

    LRESULT CALLBACK D3D12ImGuiImplementation::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        if (ImGui_ImplWin32_WndProcHandler(hwnd, uMsg, wParam, lParam))
            return true;

        switch (uMsg)
        {
            // NOTE : This might be super wrong. The ImGui layer should probably
            // handle the dispatched resize event. But there is no such event yet
        case WM_SIZE:

            if (sInstance == nullptr || wParam == SIZE_MINIMIZED) break;

            auto ctx = sInstance->ctx;
            ImGui_ImplDX12_InvalidateDeviceObjects();
            ctx->CleanupRenderTargetViews();
            ctx->ResizeSwapChain((UINT)LOWORD(lParam), (UINT)HIWORD(lParam));
            ctx->UpdateRenderTargetViews(ctx->m_Device, ctx->m_SwapChain, ctx->m_RTVDescriptorHeap);
            ImGui_ImplDX12_CreateDeviceObjects();

            return 0;
        }

        return ::CallWindowProc(originalWindowProc, hwnd, uMsg, wParam, lParam);
    }

    D3D12ImGuiImplementation* D3D12ImGuiImplementation::sInstance = nullptr;

    void D3D12ImGuiImplementation::Init(Window& window)
    {
        
        ctx = static_cast<D3D12Context*>(window.GetContext());
        HWND hwnd = (HWND)window.GetNativeWindow();
        originalWindowProc = (WNDPROC)::GetWindowLongPtr(hwnd, GWLP_WNDPROC);

        ::SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)Hazel::D3D12ImGuiImplementation::WindowProc);

        ImGui_ImplWin32_Init(ctx->m_NativeHandle);
        ImGui_ImplDX12_Init(ctx->m_Device.Get(), ctx->m_NumFrames,
            DXGI_FORMAT_R8G8B8A8_UNORM,
            ctx->m_SRVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
            ctx->m_SRVDescriptorHeap->GetGPUDescriptorHandleForHeapStart());


    }

    void D3D12ImGuiImplementation::RenderDrawData(ImDrawData* drawData)
    {
        ImGui_ImplDX12_RenderDrawData(drawData, ctx->m_CommandList.Get());
    }

    void D3D12ImGuiImplementation::NewFrame()
    {
        ImGui_ImplDX12_NewFrame();
        ImGui_ImplWin32_NewFrame();
    }

    void D3D12ImGuiImplementation::Shutdown()
    {
        ImGui_ImplDX12_Shutdown();
        ImGui_ImplWin32_Shutdown();
    }

    void D3D12ImGuiImplementation::UpdateDockedWindows()
    {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault(NULL, (void*)ctx->m_CommandList.Get());
    }

    D3D12ImGuiImplementation * D3D12ImGuiImplementation::Create()
    {
        if (sInstance == nullptr) {
            sInstance = new D3D12ImGuiImplementation();
        }

        return sInstance;
    }

}

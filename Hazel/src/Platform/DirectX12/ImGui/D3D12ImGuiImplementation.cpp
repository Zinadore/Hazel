#include "hzpch.h"
#include "D3D12ImGuiImplementation.h"

#include "imgui.h"
#include "imgui_internal.h"
#include "Platform/DirectX12/ImGui/imgui_impl_win32.h"
#include "Platform/DirectX12/ImGui/imgui_impl_dx12.h"
#include "Platform/DirectX12/ImGui/imgui_impl_win32.cpp"
#include "Platform/DirectX12/ImGui/imgui_impl_dx12.cpp"

#include "Platform/DirectX12/D3D12Context.h"

#include <GLFW/glfw3.h>


namespace Hazel {
    
    static LRESULT (*originalWindowProc)(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) = nullptr;

    LRESULT CALLBACK D3D12ImGuiImplementation::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        if (ImGui_ImplWin32_WndProcHandler(hwnd, uMsg, wParam, lParam))
            return true;

        return ::CallWindowProc(originalWindowProc, hwnd, uMsg, wParam, lParam);
    }

    D3D12ImGuiImplementation* D3D12ImGuiImplementation::sInstance = nullptr;

    void D3D12ImGuiImplementation::Init(Window& window)
    {
        
        ctx = static_cast<D3D12Context*>(window.GetContext());
        HWND hwnd = ctx->GetNativeHandle();
        originalWindowProc = (WNDPROC)::GetWindowLongPtr(hwnd, GWLP_WNDPROC);

        ::SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)Hazel::D3D12ImGuiImplementation::WindowProc);
        auto r = ctx->DeviceResources;

        ImGui_ImplWin32_Init(ctx->GetNativeHandle());
        ImGui_ImplDX12_Init(
            r->Device.Get(), 
            r->SwapChainBufferCount,
            DXGI_FORMAT_R8G8B8A8_UNORM,
            r->SRVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
            r->SRVDescriptorHeap->GetGPUDescriptorHandleForHeapStart()
        );
    }

    void D3D12ImGuiImplementation::RenderDrawData(ImDrawData* drawData)
    {
        auto r = ctx->DeviceResources;
        ImGui_ImplDX12_RenderDrawData(drawData, r->CommandList.Get());
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
        auto r = ctx->DeviceResources;
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault(NULL, (void*)r->CommandList.Get());
    }

    void D3D12ImGuiImplementation::RecreateResources()
    {
        ImGui_ImplDX12_CreateDeviceObjects();
    }

    void D3D12ImGuiImplementation::InvalidateResources()
    {
        ImGui_ImplDX12_InvalidateDeviceObjects();
    }

    D3D12ImGuiImplementation * D3D12ImGuiImplementation::Create()
    {
        if (sInstance == nullptr) {
            sInstance = new D3D12ImGuiImplementation();
        }

        return sInstance;
    }

}

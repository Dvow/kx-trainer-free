#include "internal/app.h"

#include "constants.h"
#include "gui_style.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_dx11.h"
#include "imgui/imgui_impl_win32.h"

#include <d3d11.h>
#include <dxgi1_4.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

namespace kx {
namespace {

constexpr int kMenuToggleVk = VK_PAUSE;

bool g_imguiReady = false;
bool g_showMenu = false;

HWND g_hwnd = nullptr;
WNDPROC g_origWndProc = nullptr;

ID3D11Device* g_device = nullptr;
ID3D11DeviceContext* g_context = nullptr;
ID3D11RenderTargetView* g_rtv = nullptr;
UINT g_rtvIndex = UINT_MAX;

void ShutdownImGui() {
    if (!ImGui::GetCurrentContext())
        return;
    ImGuiIO& io = ImGui::GetIO();
    if (io.BackendRendererUserData)
        ImGui_ImplDX11_Shutdown();
    if (io.BackendPlatformUserData)
        ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (g_showMenu && g_imguiReady) {
        ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam);
        const ImGuiIO& io = ImGui::GetIO();
        if (io.WantCaptureMouse && msg >= WM_MOUSEFIRST && msg <= WM_MOUSELAST)
            return 0;
        if (io.WantCaptureKeyboard && (msg == WM_KEYDOWN || msg == WM_KEYUP || msg == WM_CHAR))
            return 0;
    }
    return CallWindowProcW(g_origWndProc, hwnd, msg, wParam, lParam);
}

void HookWindow(HWND hwnd) {
    if (!hwnd || g_origWndProc)
        return;
    g_hwnd = hwnd;
    g_origWndProc = reinterpret_cast<WNDPROC>(
        SetWindowLongPtrW(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WndProc)));
}

void ReleaseRtv() {
    if (g_rtv) {
        g_rtv->Release();
        g_rtv = nullptr;
    }
    g_rtvIndex = UINT_MAX;
}

bool EnsureRtv(IDXGISwapChain* swapChain) {
    UINT idx = 0;
    IDXGISwapChain3* sc3 = nullptr;
    if (SUCCEEDED(swapChain->QueryInterface(IID_PPV_ARGS(&sc3)))) {
        idx = sc3->GetCurrentBackBufferIndex();
        sc3->Release();
    }

    if (g_rtv && g_rtvIndex == idx)
        return true;

    ReleaseRtv();

    ID3D11Texture2D* bb = nullptr;
    if (FAILED(swapChain->GetBuffer(idx, IID_PPV_ARGS(&bb)))) {
        if (FAILED(swapChain->GetBuffer(0, IID_PPV_ARGS(&bb))))
            return false;
    }

    if (FAILED(g_device->CreateRenderTargetView(bb, nullptr, &g_rtv))) {
        bb->Release();
        return false;
    }
    bb->Release();
    g_rtvIndex = idx;
    return true;
}

void PollMenuToggle() {
    static bool keyDown = false;
    const bool pressed = (GetAsyncKeyState(kMenuToggleVk) & 0x8000) != 0;
    if (pressed && !keyDown)
        g_showMenu = !g_showMenu;
    keyDown = pressed;
}

void RenderFrame(IDXGISwapChain* swapChain) {
    if (!EnsureRtv(swapChain))
        return;

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    PollMenuToggle();
    App_DrawUi(&g_showMenu);

    ImGui::Render();

    ID3D11RenderTargetView* prevRtv = nullptr;
    ID3D11DepthStencilView* prevDsv = nullptr;
    g_context->OMGetRenderTargets(1, &prevRtv, &prevDsv);
    g_context->OMSetRenderTargets(1, &g_rtv, nullptr);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    g_context->OMSetRenderTargets(1, prevRtv ? &prevRtv : nullptr, prevDsv);
    if (prevRtv) prevRtv->Release();
    if (prevDsv) prevDsv->Release();
}

}

bool Overlay_Init(IDXGISwapChain* swapChain, ID3D11Device* device) {
    if (g_imguiReady)
        return true;

    ShutdownImGui();
    ReleaseRtv();

    device->AddRef();
    g_device = device;
    g_device->GetImmediateContext(&g_context);

    DXGI_SWAP_CHAIN_DESC desc{};
    swapChain->GetDesc(&desc);
    HookWindow(desc.OutputWindow);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
    io.ConfigWindowsResizeFromEdges = true;
    Gui::loadFont(16.f);
    Gui::applyStyle();
    ImGui_ImplWin32_Init(desc.OutputWindow);
    ImGui_ImplDX11_Init(g_device, g_context);

    g_imguiReady = true;
    g_showMenu = false;
    return true;
}

void Overlay_OnPresent(IDXGISwapChain* swapChain) {
    if (g_imguiReady)
        RenderFrame(swapChain);
}

void Overlay_OnResize() {
    ReleaseRtv();
    if (g_context)
        g_context->OMSetRenderTargets(0, nullptr, nullptr);
}

void Overlay_Shutdown() {
    if (g_hwnd && g_origWndProc) {
        SetWindowLongPtrW(g_hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(g_origWndProc));
        g_hwnd = nullptr;
        g_origWndProc = nullptr;
    }

    App_Shutdown();
    ReleaseRtv();
    ShutdownImGui();
    g_imguiReady = false;

    if (g_context) {
        g_context->Release();
        g_context = nullptr;
    }
    if (g_device) {
        g_device->Release();
        g_device = nullptr;
    }
}

bool Overlay_IsReady() {
    return g_imguiReady;
}

}

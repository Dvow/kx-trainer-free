#include "internal/imgui_game_wndproc.h"

#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

namespace kx {
namespace {

constexpr auto kHoveredUi = ImGuiHoveredFlags_AnyWindow | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem;
constexpr auto kAnyPopup = ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel;

__forceinline bool isMouseButton(UINT msg) {
    return msg >= WM_LBUTTONDOWN && msg <= WM_XBUTTONDBLCLK;
}

__forceinline bool isMouseWheel(UINT msg) {
    return msg == WM_MOUSEWHEEL || msg == WM_MOUSEHWHEEL;
}

__forceinline LPARAM keyUpLParam(UINT vk) {
    const UINT scan = MapVirtualKeyW(vk, MAPVK_VK_TO_VSC);
    return static_cast<LPARAM>((scan << 16) | (1u << 30) | (1u << 31));
}

} // namespace

ImGuiGameWndProc* ImGuiGameWndProc::instance_ = nullptr;

void ImGuiGameWndProc::install(HWND hwnd, ImGuiHandler handler) {
    if (!hwnd || !handler || hwnd_)
        return;
    hwnd_ = hwnd;
    imguiHandler_ = handler;
    instance_ = this;
    original_ = reinterpret_cast<WNDPROC>(
        SetWindowLongPtrW(hwnd_, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&trampoline)));
}

void ImGuiGameWndProc::uninstall() {
    if (hwnd_ && original_) {
        SetWindowLongPtrW(hwnd_, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(original_));
        hwnd_ = nullptr;
        original_ = nullptr;
    }
    if (instance_ == this)
        instance_ = nullptr;
    reset();
}

void ImGuiGameWndProc::reset() {
    imguiHandler_ = nullptr;
    menuVisible_ = false;
    imguiReady_ = false;
    for (bool& down : gameKeyDown_)
        down = false;
}

LRESULT CALLBACK ImGuiGameWndProc::trampoline(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (instance_)
        return instance_->dispatch(hwnd, msg, wParam, lParam);
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

bool ImGuiGameWndProc::routeMouseToImGui() const {
    if (ImGui::IsWindowHovered(kHoveredUi))
        return true;
    if (ImGui::IsAnyItemActive())
        return true;
    return ImGui::IsPopupOpen(nullptr, kAnyPopup);
}

bool ImGuiGameWndProc::filterImGui(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    const bool mouseButton = isMouseButton(msg);
    const bool routeMouse = !mouseButton || routeMouseToImGui();

    if (!mouseButton || routeMouse)
        imguiHandler_(hwnd, msg, wParam, lParam);

    const ImGuiIO& io = ImGui::GetIO();

    if (mouseButton) {
        if (routeMouse && io.WantCaptureMouse)
            return true;
        if (!routeMouse)
            ImGui::ClearActiveID();
        return false;
    }

    if (isMouseWheel(msg) && io.WantCaptureMouse)
        return true;

    return false;
}

void ImGuiGameWndProc::forwardKeyUp(HWND hwnd, WPARAM vk) {
    if (!original_ || vk >= 256)
        return;
    const bool isSysKey = (vk == VK_MENU || vk == VK_LMENU || vk == VK_RMENU);
    const UINT keyMsg = isSysKey ? WM_SYSKEYUP : WM_KEYUP;
    CallWindowProcW(original_, hwnd, keyMsg, vk, keyUpLParam(static_cast<UINT>(vk)));
    gameKeyDown_[vk] = false;
}

void ImGuiGameWndProc::syncStuckKeys(HWND hwnd) {
    for (int vk = 0; vk < 256; ++vk) {
        if (!gameKeyDown_[vk])
            continue;
        if (GetAsyncKeyState(vk) & 0x8000)
            continue;
        forwardKeyUp(hwnd, vk);
    }
}

LRESULT ImGuiGameWndProc::forwardToGame(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (!original_)
        return DefWindowProcW(hwnd, msg, wParam, lParam);

    const LRESULT result = CallWindowProcW(original_, hwnd, msg, wParam, lParam);
    if (wParam < 256) {
        switch (msg) {
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
            gameKeyDown_[wParam] = true;
            break;
        case WM_KEYUP:
        case WM_SYSKEYUP:
            gameKeyDown_[wParam] = false;
            break;
        default:
            break;
        }
    }
    return result;
}

LRESULT ImGuiGameWndProc::dispatch(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_EXITSIZEMOVE)
        syncStuckKeys(hwnd);

    if (imguiReady_ && menuVisible_ && ImGui::GetCurrentContext() && imguiHandler_) {
        if (filterImGui(hwnd, msg, wParam, lParam))
            return 1;
    }

    return forwardToGame(hwnd, msg, wParam, lParam);
}

} // namespace kx

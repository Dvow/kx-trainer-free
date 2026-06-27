#include "internal/imgui_game_wndproc.h"

#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

namespace kx {
namespace {

constexpr auto kHoveredUi = ImGuiHoveredFlags_AnyWindow | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem;
constexpr auto kAnyPopup = ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel;

bool routeMouseToImGui() {
    return ImGui::IsWindowHovered(kHoveredUi)
        || ImGui::IsAnyItemActive()
        || ImGui::IsPopupOpen(nullptr, kAnyPopup);
}

bool mouseButtonEvent(UINT msg, WPARAM wParam, int& button, bool& down) {
    switch (msg) {
    case WM_LBUTTONDOWN: case WM_LBUTTONDBLCLK: button = 0; down = true; return true;
    case WM_LBUTTONUP: button = 0; down = false; return true;
    case WM_RBUTTONDOWN: case WM_RBUTTONDBLCLK: button = 1; down = true; return true;
    case WM_RBUTTONUP: button = 1; down = false; return true;
    case WM_MBUTTONDOWN: case WM_MBUTTONDBLCLK: button = 2; down = true; return true;
    case WM_MBUTTONUP: button = 2; down = false; return true;
    case WM_XBUTTONDOWN: case WM_XBUTTONDBLCLK:
        button = (GET_XBUTTON_WPARAM(wParam) == XBUTTON1) ? 3 : 4; down = true; return true;
    case WM_XBUTTONUP:
        button = (GET_XBUTTON_WPARAM(wParam) == XBUTTON1) ? 3 : 4; down = false; return true;
    default: return false;
    }
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
    gameMouseDown_ = 0;
    for (bool& down : gameKeyDown_)
        down = false;
}

LRESULT CALLBACK ImGuiGameWndProc::trampoline(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    return instance_ ? instance_->dispatch(hwnd, msg, wParam, lParam)
                     : DefWindowProcW(hwnd, msg, wParam, lParam);
}

bool ImGuiGameWndProc::handleMenu(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    int button = 0;
    bool down = false;
    const bool mouseBtn = mouseButtonEvent(msg, wParam, button, down);
    const bool routeMouse = !mouseBtn || routeMouseToImGui();

    if (mouseBtn) {
        const uint8_t mask = static_cast<uint8_t>(1u << button);
        if (down) {
            if (!routeMouse) {
                gameMouseDown_ |= mask;
                ImGui::GetIO().AddMousePosEvent(-FLT_MAX, -FLT_MAX);
            }
        } else {
            gameMouseDown_ &= static_cast<uint8_t>(~mask);
        }
    } else if (gameMouseDown_ && (msg == WM_MOUSEMOVE || msg == WM_MOUSEWHEEL || msg == WM_MOUSEHWHEEL)) {
        if (msg == WM_MOUSEMOVE)
            ImGui::GetIO().AddMousePosEvent(-FLT_MAX, -FLT_MAX);
        return false;
    }

    if (!mouseBtn || routeMouse)
        imguiHandler_(hwnd, msg, wParam, lParam);

    const ImGuiIO& io = ImGui::GetIO();
    if (mouseBtn) {
        if (routeMouse && io.WantCaptureMouse)
            return true;
        if (!routeMouse)
            ImGui::ClearActiveID();
        return false;
    }

    return (msg == WM_MOUSEWHEEL || msg == WM_MOUSEHWHEEL) && io.WantCaptureMouse;
}

LRESULT ImGuiGameWndProc::forwardToGame(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (!original_)
        return DefWindowProcW(hwnd, msg, wParam, lParam);

    const LRESULT result = CallWindowProcW(original_, hwnd, msg, wParam, lParam);
    if (wParam < 256) {
        switch (msg) {
        case WM_KEYDOWN:  case WM_SYSKEYDOWN:  gameKeyDown_[wParam] = true;  break;
        case WM_KEYUP:    case WM_SYSKEYUP:    gameKeyDown_[wParam] = false; break;
        default: break;
        }
    }
    return result;
}

void ImGuiGameWndProc::syncStuckKeys(HWND hwnd) {
    if (!original_)
        return;
    for (int vk = 0; vk < 256; ++vk) {
        if (!gameKeyDown_[vk] || (GetAsyncKeyState(vk) & 0x8000))
            continue;
        const bool alt = (vk == VK_MENU || vk == VK_LMENU || vk == VK_RMENU);
        const UINT scan = MapVirtualKeyW(vk, MAPVK_VK_TO_VSC);
        const LPARAM lp = static_cast<LPARAM>((scan << 16) | (1u << 30) | (1u << 31));
        CallWindowProcW(original_, hwnd, alt ? WM_SYSKEYUP : WM_KEYUP, vk, lp);
        gameKeyDown_[vk] = false;
    }
}

LRESULT ImGuiGameWndProc::dispatch(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_EXITSIZEMOVE)
        syncStuckKeys(hwnd);

    if (imguiReady_ && menuVisible_ && ImGui::GetCurrentContext() && imguiHandler_) {
        if (handleMenu(hwnd, msg, wParam, lParam))
            return 1;
    }

    return forwardToGame(hwnd, msg, wParam, lParam);
}

} // namespace kx

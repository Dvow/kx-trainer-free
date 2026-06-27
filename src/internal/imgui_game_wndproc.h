#pragma once

#include <cstdint>
#include <windows.h>

namespace kx {

// Lets the trainer menu and game share the same HWND without fighting over input:
// you can move and change settings at the same time, RMB camera still works off-UI,
// and keys don't get stuck after dragging or resizing the menu.
class ImGuiGameWndProc {
public:
    using ImGuiHandler = LRESULT (*)(HWND, UINT, WPARAM, LPARAM);

    void install(HWND hwnd, ImGuiHandler handler);
    void uninstall();
    void reset();

    void setMenuVisible(bool visible) { menuVisible_ = visible; }
    void setImGuiReady(bool ready) { imguiReady_ = ready; }

    HWND hwnd() const { return hwnd_; }

private:
    static LRESULT CALLBACK trampoline(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    LRESULT dispatch(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    bool handleMenu(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT forwardToGame(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    void syncStuckKeys(HWND hwnd);

    static ImGuiGameWndProc* instance_;

    HWND hwnd_ = nullptr;
    WNDPROC original_ = nullptr;
    ImGuiHandler imguiHandler_ = nullptr;
    bool menuVisible_ = false;
    bool imguiReady_ = false;
    uint8_t gameMouseDown_ = 0;
    bool gameKeyDown_[256]{};
};

} // namespace kx

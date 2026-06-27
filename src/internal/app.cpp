#include "app.h"

#include "constants.h"
#include "config.h"
#include "hack.h"
#include "hack_gui.h"
#include "internal/hooks.h"
#include "path_util.h"
#include "version_gate.h"
#include "log.h"

#include "imgui/imgui.h"

#include <windows.h>

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

namespace kx {
namespace {

enum class AppState { Loading, Ready, Failed };

std::atomic<AppState> g_state{ AppState::Loading };
std::atomic<bool> g_openMenuWhenReady{ true };
std::string g_errorMessage;
std::mutex g_mutex;
std::unique_ptr<Hack> g_hack;
std::unique_ptr<HackGUI> g_gui;

void SetDllSearchPath() {
    const std::wstring directory = moduleDirectory(GetModuleHandleW(Constants::DLL_FILENAME));
    if (!directory.empty())
        SetDllDirectoryW(directory.c_str());
}

void DrawErrorUi() {
    ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(420, 250.f), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("KX Trainer - Error", nullptr, ImGuiWindowFlags_NoCollapse)) {
        ImGui::TextWrapped("%s", g_errorMessage.c_str());
        ImGui::TextDisabled("Press Insert to hide this window.");
        ImGui::Separator();
        Log::renderInline();
    }
    ImGui::End();
}

void AppInitThread() {
    SetDllSearchPath();
    Config::load();

    VersionGate gate;
    if (!gate.check()) {
        g_errorMessage = gate.lastMessage();
        g_state = AppState::Failed;
        return;
    }

    auto hack = std::make_unique<Hack>();
    if (!hack->initialize()) {
        if (g_errorMessage.empty())
            g_errorMessage = "Hack initialization failed. Load into the game world with a character, then reinject. See log for details.";
        g_state = AppState::Failed;
        return;
    }

    {
        std::lock_guard lock(g_mutex);
        g_hack = std::move(hack);
        g_gui = std::make_unique<HackGUI>(*g_hack, Hooks_Unload);
    }
    g_state = AppState::Ready;
}

}

void App_Start() {
    g_openMenuWhenReady = true;
    std::thread(AppInitThread).detach();
}

void App_Shutdown() {
    std::lock_guard lock(g_mutex);
    if (g_gui)
        g_gui->flushWindowLayout();
    g_gui.reset();
    g_hack.reset();
}

void App_DrawUi(bool* showMenu) {
    switch (g_state.load()) {
    case AppState::Loading:
        *showMenu = false;
        return;
    case AppState::Ready:
        if (g_openMenuWhenReady.exchange(false))
            *showMenu = true;
        {
            std::lock_guard lock(g_mutex);
            if (g_gui)
                g_gui->render(showMenu);
        }
        return;
    case AppState::Failed:
        if (showMenu && *showMenu)
            DrawErrorUi();
        return;
    }
}

}

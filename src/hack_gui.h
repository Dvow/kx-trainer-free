#pragma once

#include "config.h"
#include "gui_sections.h"

#include <array>
#include <functional>
#include <vector>

namespace kx {

class Hack;

struct Hotkey {
    const char* name;
    Config::KeyChord recommendedBinding;
    Config::KeyChord currentBinding;
    std::function<void(Hack&)> action;
};

class HackGUI {
public:
    explicit HackGUI(Hack& hack, void (*uninject)());
    void render(bool* showMenu);
    void flushWindowLayout();

private:
    enum class RebindPhase { None, WaitRelease, Listen };

    Hack& m_hack;
    std::vector<Hotkey> m_hotkeys;
    std::vector<bool> m_holdMode;
    int m_rebindingIndex = -1;
    RebindPhase m_rebindPhase = RebindPhase::None;
    Config::KeyChord m_rebindPreview;
    Config::KeyChord m_prevHeldChord;
    std::array<bool, 256> m_keyWasDown{};

    Config::WindowLayout m_window{};
    Config::WindowLayout m_savedLayout{ -1.f, -1.f, -1.f, -1.f };
    bool m_windowApplied = false;
    bool m_hasWindow = false;
    bool m_menuWasVisible = false;
    std::array<bool, kSectionCount> m_sectionsOpen{};
    std::array<bool, kSectionCount> m_sectionsInit{};

    void loadHoldModes();
    void setHoldMode(std::size_t toggleIndex, bool hold);
    void handleHotkeys();
    void handleHoldHotkeys(const Config::KeyChord& held);
    void handlePressedHotkeys(const Config::KeyChord& held);
    void handleRebinding();
    void beginRebind(int index);
    void endRebind();
    bool keyDown(int vk) const;
    bool keyPressed(int vk);
    void updateKeyState();
    void renderHotkeyRow(int index);
    void renderToggles();
    void renderActions();
    void renderHotkeys();
    void renderLog();
    void renderInfo();
    void persistHotkeys();
    void persistToggles();
    void restoreTogglesFromConfig();
    void restoreSectionsFromConfig();
    void persistSections();
    bool sectionHeader(std::size_t index, const char* label);
    void applyWindowLayout();
    void saveWindowLayout(bool force = false);
};

}

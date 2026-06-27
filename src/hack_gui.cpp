#include "hack_gui.h"

#include "constants.h"
#include "hack.h"
#include "internal/overlay.h"
#include "key_chord.h"
#include "log.h"
#include "shell_util.h"

#include "imgui/imgui.h"

#include <windows.h>

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstring>
#include <string>

namespace kx {
namespace {

struct ToggleDef {
    const char* name;
    const char* label;
    int defaultVk;
    bool (Hack::*isEnabled)() const;
    void (Hack::*setEnabled)(bool);
};

struct ActionDef {
    const char* name;
    int defaultVk;
    void (Hack::*action)();
};

using namespace Constants::Hotkeys;

constexpr ToggleDef kToggles[] = {
    { "No Fog", "No Fog", NO_FOG, &Hack::isFogEnabled, &Hack::setFog },
    { "Object Clipping", "Object Clipping", OBJECT_CLIPPING, &Hack::isObjectClippingEnabled, &Hack::setObjectClipping },
    { "Full Strafe", "Full Strafe", FULL_STRAFE, &Hack::isFullStrafeEnabled, &Hack::setFullStrafe },
    { "Sprint", "Sprint", SPRINT, &Hack::isSprintEnabled, &Hack::setSprint },
    { "Super Sprint", "Super Sprint", SUPER_SPRINT, &Hack::isSuperSprintEnabled, &Hack::setSuperSprint },
    { "Fly", "Fly", FLY, &Hack::isFlyEnabled, &Hack::setFly },
    { "Invisibility", "Invisibility (Mobs)", INVISIBILITY, &Hack::isInvisibilityEnabled, &Hack::setInvisibility },
    { "Wall Climb", "Wall Climb", WALL_CLIMB, &Hack::isWallClimbEnabled, &Hack::setWallClimb },
    { "Clipping", "Clipping", CLIPPING, &Hack::isClippingEnabled, &Hack::setClipping },
};

constexpr ActionDef kActions[] = {
    { "Save Position", SAVE_POS, &Hack::savePosition },
    { "Load Position", LOAD_POS, &Hack::loadPosition },
};

constexpr size_t kToggleHotkeyBegin = std::size(kActions);
constexpr size_t kToggleHotkeyEnd = kToggleHotkeyBegin + std::size(kToggles);
constexpr size_t kToggleCount = std::size(kToggles);

bool isToggleHotkeyIndex(int index) {
    return index >= static_cast<int>(kToggleHotkeyBegin) &&
        index < static_cast<int>(kToggleHotkeyEnd);
}

std::size_t toggleIndexFromHotkey(int hotkeyIndex) {
    return static_cast<std::size_t>(hotkeyIndex - static_cast<int>(kToggleHotkeyBegin));
}

bool isForcedHoldToggle(std::size_t toggleIndex) {
    return std::strcmp(kToggles[toggleIndex].name, "Super Sprint") == 0
        || std::strcmp(kToggles[toggleIndex].name, "Fly") == 0;
}

std::vector<Hotkey> buildHotkeys(std::function<void(Hack&)> onUnload) {
    std::vector<Hotkey> keys;
    keys.reserve(kToggleHotkeyEnd + 1);

    for (const auto& action : kActions) {
        keys.push_back({
            action.name,
            Config::KeyChord::single(action.defaultVk),
            {},
            [fn = action.action](Hack& h) { (h.*fn)(); },
        });
    }

    for (const auto& toggle : kToggles) {
        keys.push_back({
            toggle.name,
            Config::KeyChord::single(toggle.defaultVk),
            {},
            [get = toggle.isEnabled, set = toggle.setEnabled](Hack& h) {
                (h.*set)(!(h.*get)());
            },
        });
    }

    keys.push_back({
        "Unload",
        Config::KeyChord::single(UNLOAD),
        {},
        std::move(onUnload),
    });

    return keys;
}

bool layoutNearEqual(const Config::WindowLayout& a, const Config::WindowLayout& b) {
    constexpr float kEpsilon = 0.5f;
    return std::abs(a.x - b.x) < kEpsilon &&
        std::abs(a.y - b.y) < kEpsilon &&
        std::abs(a.width - b.width) < kEpsilon &&
        std::abs(a.height - b.height) < kEpsilon;
}

}

HackGUI::HackGUI(Hack& hack, void (*unload)())
    : m_hack(hack),
      m_unload(unload),
      m_hotkeys(buildHotkeys([this](Hack&) { requestUnload(); })) {
    m_prevChordHeld.resize(m_hotkeys.size());
    for (auto& hotkey : m_hotkeys)
        hotkey.currentBinding = Config::hotkeyBindingFor(hotkey.name);
    m_hotkeysRequireFocus = Config::hotkeysRequireFocus();
    loadHoldModes();
    restoreTogglesFromConfig();
    restoreSectionsFromConfig();
}

void HackGUI::loadHoldModes() {
    m_holdMode.resize(kToggleCount);
    for (std::size_t i = 0; i < kToggleCount; ++i)
        m_holdMode[i] = isForcedHoldToggle(i) || Config::hotkeyHoldFor(kToggles[i].name, false);
}

void HackGUI::setHoldMode(std::size_t toggleIndex, bool hold) {
    if (toggleIndex >= kToggleCount || (isForcedHoldToggle(toggleIndex) && !hold))
        return;

    m_holdMode[toggleIndex] = hold;
    const auto& toggle = kToggles[toggleIndex];
    Config::setHotkeyHold(toggle.name, hold);

    if (hold) {
        if ((m_hack.*toggle.isEnabled)())
            (m_hack.*toggle.setEnabled)(false);
        Config::setToggle(toggle.name, false);
    }

    Config::save();
}

void HackGUI::applyWindowLayout() {
    if (m_windowApplied)
        return;

    const Config::WindowLayout layout = Config::loadWindowLayout();
    ImGui::SetNextWindowPos(ImVec2(layout.x, layout.y), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(layout.width, layout.height), ImGuiCond_Always);
    m_windowApplied = true;
}

void HackGUI::saveWindowLayout(bool force) {
    if (!m_hasWindow)
        return;
    if (!force && m_savedLayout.width >= 0.f && layoutNearEqual(m_window, m_savedLayout))
        return;

    m_savedLayout = m_window;
    Config::setWindowLayout(m_window.x, m_window.y, m_window.width, m_window.height);
    Config::save();
}

void HackGUI::flushWindowLayout() {
    if (m_hasWindow) {
        m_savedLayout = m_window;
        Config::setWindowLayout(m_window.x, m_window.y, m_window.width, m_window.height);
    }
    for (std::size_t i = 0; i < kSectionCount; ++i)
        Config::setSectionOpen(kSectionNames[i], m_sectionsOpen[i]);
    Config::save(true);
}

void HackGUI::restoreTogglesFromConfig() {
    for (std::size_t i = 0; i < kToggleCount; ++i) {
        const auto& toggle = kToggles[i];
        if (m_holdMode[i]) {
            (m_hack.*toggle.setEnabled)(false);
            continue;
        }
        if (Config::toggleFor(toggle.name, false))
            (m_hack.*toggle.setEnabled)(true);
    }
}

void HackGUI::restoreSectionsFromConfig() {
    for (std::size_t i = 0; i < kSectionCount; ++i)
        m_sectionsOpen[i] = Config::sectionOpen(kSectionNames[i], kSectionDefaults[i]);
}

void HackGUI::persistSections() {
    for (std::size_t i = 0; i < kSectionCount; ++i)
        Config::setSectionOpen(kSectionNames[i], m_sectionsOpen[i]);
    Config::save();
}

bool HackGUI::sectionHeader(std::size_t index, const char* label) {
    if (!m_sectionsInit[index]) {
        ImGui::SetNextItemOpen(m_sectionsOpen[index], ImGuiCond_Always);
        m_sectionsInit[index] = true;
    }

    const bool open = ImGui::CollapsingHeader(label);
    if (open != m_sectionsOpen[index]) {
        m_sectionsOpen[index] = open;
        persistSections();
    }
    return open;
}

void HackGUI::persistHotkeys() {
    for (const auto& hotkey : m_hotkeys)
        Config::setHotkeyBinding(hotkey.name, hotkey.currentBinding);
    Config::save();
}

void HackGUI::persistToggles() {
    for (const auto& toggle : kToggles)
        Config::setToggle(toggle.name, (m_hack.*toggle.isEnabled)());
    Config::save();
}

void HackGUI::renderHotkeyRow(int index) {
    Hotkey& hotkey = m_hotkeys[index];
    ImGui::PushID(index);

    ImGui::Text("%s:", hotkey.name);
    ImGui::SameLine(150.f);

    if (m_rebindingIndex == index) {
        if (m_rebindPhase == RebindPhase::WaitRelease)
            ImGui::TextDisabled("<Release all keys...>");
        else if (!m_rebindPreview.empty())
            ImGui::TextDisabled("<%s>", chordLabel(m_rebindPreview).c_str());
        else
            ImGui::TextDisabled("<Press combo, release to set>");
        ImGui::PopID();
        return;
    }

    ImGui::Text("%s", chordLabel(hotkey.currentBinding).c_str());
    ImGui::SameLine(280.f);
    if (ImGui::Button("Change"))
        beginRebind(index);

    if (isToggleHotkeyIndex(index)) {
        const std::size_t ti = toggleIndexFromHotkey(index);
        ImGui::SameLine(360.f);
        if (isForcedHoldToggle(ti)) {
            ImGui::BeginDisabled();
            bool hold = true;
            ImGui::Checkbox("Hold", &hold);
            ImGui::EndDisabled();
        } else {
            bool hold = m_holdMode[ti];
            if (ImGui::Checkbox("Hold", &hold))
                setHoldMode(ti, hold);
        }
    }

    ImGui::PopID();
}

bool HackGUI::keyDown(int vk) const {
    return (GetAsyncKeyState(vk) & 0x8000) != 0;
}

bool HackGUI::keyPressed(int vk) {
    return keyDown(vk) && !m_keyWasDown[vk];
}

void HackGUI::updateKeyState() {
    for (int vk = 0; vk < 256; ++vk)
        m_keyWasDown[vk] = keyDown(vk);
}

void HackGUI::beginRebind(int index) {
    m_rebindingIndex = index;
    m_rebindPhase = RebindPhase::WaitRelease;
    m_rebindPreview = {};
}

void HackGUI::endRebind() {
    m_rebindingIndex = -1;
    m_rebindPhase = RebindPhase::None;
    m_rebindPreview = {};
}

bool HackGUI::bindingInUseByOther(int exceptIndex, const Config::KeyChord& binding) const {
    if (binding.empty())
        return false;

    for (int i = 0; i < static_cast<int>(m_hotkeys.size()); ++i) {
        if (i == exceptIndex || m_hotkeys[i].currentBinding.empty())
            continue;
        if (m_hotkeys[i].currentBinding == binding)
            return true;
    }
    return false;
}

bool HackGUI::tryAssignBinding(int index, const Config::KeyChord& binding) {
    if (index < 0 || index >= static_cast<int>(m_hotkeys.size()))
        return false;

    if (!binding.empty() && bindingInUseByOther(index, binding)) {
        Log::add("WARN: " + chordLabel(binding) + " is already bound to another feature.");
        return false;
    }

    m_hotkeys[index].currentBinding = binding;
    return true;
}

void HackGUI::handleHoldHotkeys() {
    bool changed = false;

    for (std::size_t i = 0; i < kToggleCount; ++i) {
        if (!m_holdMode[i])
            continue;

        const auto& toggle = kToggles[i];
        const auto& binding = m_hotkeys[kToggleHotkeyBegin + i].currentBinding;
        if (binding.empty())
            continue;

        const bool on = chordHeld(binding);
        if ((m_hack.*toggle.isEnabled)() == on)
            continue;

        (m_hack.*toggle.setEnabled)(on);
        changed = true;
    }

    if (changed)
        persistToggles();
}

void HackGUI::handlePressedHotkeys() {
    for (std::size_t i = 0; i < m_hotkeys.size(); ++i) {
        if (isToggleHotkeyIndex(static_cast<int>(i)) &&
            m_holdMode[toggleIndexFromHotkey(static_cast<int>(i))]) {
            continue;
        }

        const auto& hotkey = m_hotkeys[i];
        if (hotkey.currentBinding.empty() || !hotkey.action)
            continue;

        const bool active = chordHeld(hotkey.currentBinding);
        if (!active || m_prevChordHeld[i])
            continue;

        hotkey.action(m_hack);
        if (isToggleHotkeyIndex(static_cast<int>(i)))
            persistToggles();
    }
}

void HackGUI::handleHotkeys() {
    if (m_rebindPhase != RebindPhase::None)
        return;

    if (!shouldProcessHotkeys()) {
        releaseHoldToggles();
        std::fill(m_prevChordHeld.begin(), m_prevChordHeld.end(), false);
        return;
    }

    handleHoldHotkeys();
    handlePressedHotkeys();

    for (std::size_t i = 0; i < m_hotkeys.size(); ++i)
        m_prevChordHeld[i] = chordHeld(m_hotkeys[i].currentBinding);
}

bool HackGUI::shouldProcessHotkeys() const {
    if (!m_hotkeysRequireFocus)
        return true;
    const HWND game = Overlay_GameHwnd();
    return game && GetForegroundWindow() == game;
}

void HackGUI::releaseHoldToggles() {
    bool changed = false;
    for (std::size_t i = 0; i < kToggleCount; ++i) {
        if (!m_holdMode[i])
            continue;
        const auto& toggle = kToggles[i];
        if ((m_hack.*toggle.isEnabled)()) {
            (m_hack.*toggle.setEnabled)(false);
            changed = true;
        }
    }
    if (changed)
        persistToggles();
}

void HackGUI::handleRebinding() {
    if (m_rebindPhase == RebindPhase::None)
        return;

    if (keyPressed(VK_ESCAPE)) {
        endRebind();
        return;
    }

    if (keyPressed(VK_DELETE) || keyPressed(VK_BACK)) {
        if (m_rebindingIndex >= 0)
            m_hotkeys[m_rebindingIndex].currentBinding = {};
        persistHotkeys();
        endRebind();
        return;
    }

    if (m_rebindPhase == RebindPhase::WaitRelease) {
        if (anyInputHeld())
            return;
        m_rebindPhase = RebindPhase::Listen;
        m_rebindPreview = {};
        return;
    }

    for (int vk = 1; vk < 256; ++vk) {
        if (isBindableKey(vk) && keyPressed(vk))
            addKeyToChord(m_rebindPreview, vk);
    }

    const Config::KeyChord held = readHeldBindableKeys();
    for (int vk : held.keys)
        addKeyToChord(m_rebindPreview, vk);

    if (held.empty()) {
        if (!m_rebindPreview.empty() && m_rebindingIndex >= 0) {
            if (tryAssignBinding(m_rebindingIndex, m_rebindPreview))
                persistHotkeys();
            endRebind();
        }
    }
}

void HackGUI::renderToggles() {
    if (!sectionHeader(0, kSectionNames[0]))
        return;

    for (std::size_t i = 0; i < kToggleCount; ++i) {
        const auto& toggle = kToggles[i];
        if (m_holdMode[i])
            ImGui::BeginDisabled();

        bool value = (m_hack.*toggle.isEnabled)();
        if (ImGui::Checkbox(toggle.label, &value)) {
            (m_hack.*toggle.setEnabled)(value);
            persistToggles();
        }

        if (m_holdMode[i]) {
            ImGui::EndDisabled();
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
                ImGui::SetTooltip("Hold hotkey controls this");
        }
    }
    ImGui::Spacing();
}

void HackGUI::renderActions() {
    if (!sectionHeader(1, kSectionNames[1]))
        return;

    const float width = ImGui::GetContentRegionAvail().x * 0.48f;
    for (std::size_t i = 0; i < std::size(kActions); ++i) {
        if (i > 0)
            ImGui::SameLine();
        const ImVec2 size(i + 1 < std::size(kActions) ? ImVec2(width, 0) : ImVec2(-1.f, 0));
        if (ImGui::Button(kActions[i].name, size))
            (m_hack.*kActions[i].action)();
    }
    ImGui::Spacing();
}

void HackGUI::renderHotkeys() {
    if (!sectionHeader(2, kSectionNames[2]))
        return;

    if (ImGui::Checkbox("Only when game is focused", &m_hotkeysRequireFocus)) {
        Config::setHotkeysRequireFocus(m_hotkeysRequireFocus);
        Config::save();
    }
    ImGui::Spacing();

    const bool disableHotkeyControls = m_rebindingIndex >= 0;
    if (disableHotkeyControls)
        ImGui::BeginDisabled();

    for (int i = 0; i < static_cast<int>(m_hotkeys.size()); ++i)
        renderHotkeyRow(i);

    if (disableHotkeyControls)
        ImGui::EndDisabled();

    ImGui::Separator();
    if (ImGui::Button("Apply Recommended Defaults")) {
        for (auto& hotkey : m_hotkeys)
            hotkey.currentBinding = hotkey.recommendedBinding;
        persistHotkeys();
    }
    ImGui::SameLine();
    if (ImGui::Button("Unbind All")) {
        for (auto& hotkey : m_hotkeys)
            hotkey.currentBinding = {};
        persistHotkeys();
    }
    ImGui::Spacing();
}

void HackGUI::renderLog() {
    if (!sectionHeader(3, kSectionNames[3]))
        return;

    Log::renderInline(100.f);
    if (ImGui::Button("Copy Log"))
        Log::copyToClipboard();
    ImGui::SameLine();
    if (ImGui::Button("Clear Log"))
        Log::clear();
    ImGui::Spacing();
}

void HackGUI::renderInfo() {
    if (!sectionHeader(4, kSectionNames[4]))
        return;

    ImGui::TextDisabled("Insert = toggle menu | Set hotkeys in Hotkeys section");
    ImGui::Separator();
    ImGui::Text("KX Trainer by Krixx");
    ImGui::Text("Consider the paid version at kxtools.xyz!");
    ImGui::Separator();

    if (ImGui::Button("Repository"))
        openUrl("https://github.com/Krixx1337/KX-Trainer-Free");
    ImGui::SameLine();
    if (ImGui::Button("kxtools.xyz"))
        openUrl("https://kxtools.xyz");
    ImGui::SameLine();
    if (ImGui::Button("Discord"))
        openUrl("https://discord.gg/z92rnB4kHm");
}

void HackGUI::requestUnload() {
    flushWindowLayout();
    if (m_unload)
        m_unload();
}

void HackGUI::render(bool* showMenu) {
    handleHotkeys();
    m_hack.tick();

    const bool visible = showMenu && *showMenu;
    if (m_menuWasVisible && !visible) {
        if (m_rebindPhase != RebindPhase::None)
            endRebind();
        saveWindowLayout(true);
        persistSections();
    }
    m_menuWasVisible = visible;

    if (!visible)
        return;

    applyWindowLayout();
    ImGui::SetNextWindowSizeConstraints(ImVec2(400.f, 250.f), ImVec2(FLT_MAX, FLT_MAX));
    ImGui::Begin(Constants::APP_NAME, showMenu);

    handleRebinding();
    renderToggles();
    renderActions();
    renderHotkeys();
    renderLog();
    renderInfo();

    ImGui::Separator();
    ImGui::Spacing();
    if (ImGui::Button("Unload", ImVec2(-1.f, 0.f)))
        requestUnload();

    const ImVec2 pos = ImGui::GetWindowPos();
    const ImVec2 size = ImGui::GetWindowSize();
    m_window = { pos.x, pos.y, size.x, size.y };
    m_hasWindow = true;
    saveWindowLayout();

    updateKeyState();
    ImGui::End();
}

}

#include "config.h"
#include "constants.h"
#include "key_chord.h"
#include "path_util.h"

#include <windows.h>

#include <fstream>
#include <filesystem>
#include <mutex>
#include <string>

#include "internal/nlohmann/json.hpp"

namespace kx::Config {
namespace {

constexpr int kSchemaVersion = 2;
std::mutex g_mutex;
nlohmann::json g_data = nlohmann::json::object();
bool g_loaded = false;
bool g_dirty = false;

nlohmann::json defaultDocument() {
    return nlohmann::json{
        { "version", kSchemaVersion },
        { "hotkeys", nlohmann::json::object() },
        { "hotkeyHold", nlohmann::json::object() },
        { "toggles", nlohmann::json::object() },
        { "window", {
            { "saved", false },
            { "x", kDefaultWindow.x },
            { "y", kDefaultWindow.y },
            { "width", kDefaultWindow.width },
            { "height", kDefaultWindow.height },
        } },
        { "position", {
            { "saved", false },
            { "x", 0.f },
            { "y", 0.f },
            { "z", 0.f },
        } },
        { "sections", nlohmann::json::object() },
    };
}

std::wstring configPath() {
    return moduleDirectoryForFile(
        GetModuleHandleW(Constants::DLL_FILENAME), L"config.json");
}

bool hasWindowLayout() {
    return g_data["window"].value("saved", false);
}

WindowLayout readWindowLayout() {
    const auto& window = g_data["window"];
    return {
        window.value("x", kDefaultWindow.x),
        window.value("y", kDefaultWindow.y),
        window.value("width", kDefaultWindow.width),
        window.value("height", kDefaultWindow.height),
    };
}

KeyChord readKeyChord(const nlohmann::json& value, const KeyChord& fallback) {
    if (value.is_number_integer())
        return KeyChord::single(value.get<int>());

    if (!value.is_array())
        return fallback;

    KeyChord chord;
    for (const auto& entry : value) {
        if (entry.is_number_integer())
            chord.keys.push_back(entry.get<int>());
    }
    kx::normalizeKeyChord(chord);
    return chord;
}

nlohmann::json writeKeyChord(const KeyChord& chord) {
    nlohmann::json out = nlohmann::json::array();
    for (int vk : chord.keys)
        out.push_back(vk);
    return out;
}

void mergeDefaults(nlohmann::json& doc) {
    if (!doc.contains("hotkeys") || !doc["hotkeys"].is_object())
        doc["hotkeys"] = nlohmann::json::object();
    if (!doc.contains("hotkeyHold") || !doc["hotkeyHold"].is_object())
        doc["hotkeyHold"] = nlohmann::json::object();
    if (!doc.contains("toggles") || !doc["toggles"].is_object())
        doc["toggles"] = nlohmann::json::object();
    if (!doc.contains("window") || !doc["window"].is_object())
        doc["window"] = defaultDocument()["window"];
    if (!doc.contains("position") || !doc["position"].is_object())
        doc["position"] = defaultDocument()["position"];
    if (!doc.contains("sections") || !doc["sections"].is_object())
        doc["sections"] = nlohmann::json::object();
}

} // namespace

KeyChord KeyChord::single(int vk) {
    if (vk == 0)
        return {};
    KeyChord chord{ { vk } };
    kx::normalizeKeyChord(chord);
    return chord;
}

bool operator==(const KeyChord& a, const KeyChord& b) {
    return a.keys == b.keys;
}

bool operator!=(const KeyChord& a, const KeyChord& b) {
    return !(a == b);
}

bool load() {
    std::lock_guard lock(g_mutex);
    g_data = defaultDocument();
    g_loaded = true;
    g_dirty = false;

    const std::wstring path = configPath();
    if (path.empty())
        return false;

    std::ifstream file{ std::filesystem::path(path) };
    if (!file)
        return false;

    nlohmann::json parsed = nlohmann::json::parse(file, nullptr, false);
    if (parsed.is_discarded() || !parsed.is_object())
        return false;

    g_data = std::move(parsed);
    mergeDefaults(g_data);
    return true;
}

bool save(bool force) {
    std::lock_guard lock(g_mutex);
    if (!g_loaded || (!force && !g_dirty))
        return g_loaded;

    const std::wstring path = configPath();
    if (path.empty())
        return false;

    std::ofstream file{ std::filesystem::path(path), std::ios::trunc };
    if (!file)
        return false;

    g_data["version"] = kSchemaVersion;
    file << g_data.dump(2);
    g_dirty = false;
    return static_cast<bool>(file);
}

KeyChord hotkeyBindingFor(const std::string& name, const KeyChord& defaultBinding) {
    std::lock_guard lock(g_mutex);
    if (!g_loaded)
        return defaultBinding;
    const auto& hotkeys = g_data["hotkeys"];
    if (!hotkeys.contains(name))
        return defaultBinding;
    return readKeyChord(hotkeys[name], defaultBinding);
}

void setHotkeyBinding(const std::string& name, const KeyChord& binding) {
    std::lock_guard lock(g_mutex);
    if (!g_loaded)
        return;
    g_data["hotkeys"][name] = writeKeyChord(binding);
    g_dirty = true;
}

bool hotkeyHoldFor(const std::string& name, bool defaultValue) {
    std::lock_guard lock(g_mutex);
    if (!g_loaded)
        return defaultValue;
    const auto& holds = g_data["hotkeyHold"];
    if (!holds.contains(name) || !holds[name].is_boolean())
        return defaultValue;
    return holds[name].get<bool>();
}

void setHotkeyHold(const std::string& name, bool hold) {
    std::lock_guard lock(g_mutex);
    if (!g_loaded)
        return;
    g_data["hotkeyHold"][name] = hold;
    g_dirty = true;
}

bool toggleFor(const std::string& name, bool defaultValue) {
    std::lock_guard lock(g_mutex);
    if (!g_loaded)
        return defaultValue;
    const auto& toggles = g_data["toggles"];
    if (!toggles.contains(name) || !toggles[name].is_boolean())
        return defaultValue;
    return toggles[name].get<bool>();
}

void setToggle(const std::string& name, bool enabled) {
    std::lock_guard lock(g_mutex);
    if (!g_loaded)
        return;
    g_data["toggles"][name] = enabled;
    g_dirty = true;
}

WindowLayout loadWindowLayout() {
    std::lock_guard lock(g_mutex);
    if (!g_loaded || !hasWindowLayout())
        return kDefaultWindow;
    return readWindowLayout();
}

void setWindowLayout(float x, float y, float width, float height) {
    std::lock_guard lock(g_mutex);
    if (!g_loaded)
        return;
    g_data["window"] = {
        { "saved", true },
        { "x", x },
        { "y", y },
        { "width", width },
        { "height", height },
    };
    g_dirty = true;
}

bool hasSavedPosition() {
    std::lock_guard lock(g_mutex);
    if (!g_loaded)
        return false;
    return g_data["position"].value("saved", false);
}

Position savedPosition() {
    std::lock_guard lock(g_mutex);
    if (!g_loaded)
        return {};
    const auto& pos = g_data["position"];
    return {
        pos.value("x", 0.f),
        pos.value("y", 0.f),
        pos.value("z", 0.f),
    };
}

void setSavedPosition(float x, float y, float z) {
    std::lock_guard lock(g_mutex);
    if (!g_loaded)
        return;
    g_data["position"] = {
        { "saved", true },
        { "x", x },
        { "y", y },
        { "z", z },
    };
    g_dirty = true;
}

bool sectionOpen(const std::string& name, bool defaultOpen) {
    std::lock_guard lock(g_mutex);
    if (!g_loaded)
        return defaultOpen;
    const auto& sections = g_data["sections"];
    if (!sections.contains(name) || !sections[name].is_boolean())
        return defaultOpen;
    return sections[name].get<bool>();
}

void setSectionOpen(const std::string& name, bool open) {
    std::lock_guard lock(g_mutex);
    if (!g_loaded)
        return;
    g_data["sections"][name] = open;
    g_dirty = true;
}

} // namespace kx::Config

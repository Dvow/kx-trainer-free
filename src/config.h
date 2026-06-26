#pragma once

#include <string>
#include <vector>

namespace kx::Config {

struct KeyChord {
    std::vector<int> keys;

    static KeyChord single(int vk);
    bool empty() const { return keys.empty(); }
};

bool operator==(const KeyChord& a, const KeyChord& b);
bool operator!=(const KeyChord& a, const KeyChord& b);

struct Position {
    float x = 0.f;
    float y = 0.f;
    float z = 0.f;
};

struct WindowLayout {
    float x;
    float y;
    float width;
    float height;
};

inline constexpr WindowLayout kDefaultWindow{ 20.f, 20.f, 420.f, 550.f };

bool load();
bool save(bool force = false);

KeyChord hotkeyBindingFor(const std::string& name);
void setHotkeyBinding(const std::string& name, const KeyChord& binding);

bool hotkeyHoldFor(const std::string& name, bool defaultValue = false);
void setHotkeyHold(const std::string& name, bool hold);

bool toggleFor(const std::string& name, bool defaultValue = false);
void setToggle(const std::string& name, bool enabled);

WindowLayout loadWindowLayout();
void setWindowLayout(float x, float y, float width, float height);

bool hasSavedPosition();
Position savedPosition();
void setSavedPosition(float x, float y, float z);

bool sectionOpen(const std::string& name, bool defaultOpen = false);
void setSectionOpen(const std::string& name, bool open);

} // namespace kx::Config

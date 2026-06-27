#include "key_chord.h"

#include <windows.h>

#include <algorithm>
#include <cstdio>
#include <map>
#include <string>

namespace kx {
namespace {

const char* lookupKeyName(int vk) {
    static const std::map<int, const char*> names = {
        {VK_LBUTTON, "LMouse"}, {VK_RBUTTON, "RMouse"}, {VK_MBUTTON, "MMouse"},
        {VK_BACK, "Backspace"}, {VK_TAB, "Tab"}, {VK_RETURN, "Enter"}, {VK_SHIFT, "Shift"},
        {VK_LSHIFT, "LShift"}, {VK_RSHIFT, "RShift"}, {VK_CONTROL, "Ctrl"}, {VK_LCONTROL, "LCtrl"},
        {VK_RCONTROL, "RCtrl"}, {VK_MENU, "Alt"}, {VK_LMENU, "LAlt"}, {VK_RMENU, "RAlt"},
        {VK_PAUSE, "Pause"}, {VK_CAPITAL, "Caps Lock"}, {VK_ESCAPE, "Esc"}, {VK_SPACE, "Space"},
        {VK_PRIOR, "Page Up"}, {VK_NEXT, "Page Down"}, {VK_END, "End"}, {VK_HOME, "Home"},
        {VK_LEFT, "Left Arrow"}, {VK_UP, "Up Arrow"}, {VK_RIGHT, "Right Arrow"}, {VK_DOWN, "Down Arrow"},
        {VK_INSERT, "Insert"}, {VK_DELETE, "Delete"},
        {'0', "0"}, {'1', "1"}, {'2', "2"}, {'3', "3"}, {'4', "4"}, {'5', "5"}, {'6', "6"},
        {'7', "7"}, {'8', "8"}, {'9', "9"}, {'A', "A"}, {'B', "B"}, {'C', "C"}, {'D', "D"},
        {'E', "E"}, {'F', "F"}, {'G', "G"}, {'H', "H"}, {'I', "I"}, {'J', "J"}, {'K', "K"},
        {'L', "L"}, {'M', "M"}, {'N', "N"}, {'O', "O"}, {'P', "P"}, {'Q', "Q"}, {'R', "R"},
        {'S', "S"}, {'T', "T"}, {'U', "U"}, {'V', "V"}, {'W', "W"}, {'X', "X"}, {'Y', "Y"},
        {'Z', "Z"}, {VK_NUMPAD0, "Num 0"}, {VK_NUMPAD1, "Num 1"}, {VK_NUMPAD2, "Num 2"},
        {VK_NUMPAD3, "Num 3"}, {VK_NUMPAD4, "Num 4"}, {VK_NUMPAD5, "Num 5"}, {VK_NUMPAD6, "Num 6"},
        {VK_NUMPAD7, "Num 7"}, {VK_NUMPAD8, "Num 8"}, {VK_NUMPAD9, "Num 9"}, {VK_MULTIPLY, "Num *"},
        {VK_ADD, "Num +"}, {VK_SEPARATOR, "Num Sep"}, {VK_SUBTRACT, "Num -"}, {VK_DECIMAL, "Num ."},
        {VK_DIVIDE, "Num /"}, {VK_F1, "F1"}, {VK_F2, "F2"}, {VK_F3, "F3"}, {VK_F4, "F4"},
        {VK_F5, "F5"}, {VK_F6, "F6"}, {VK_F7, "F7"}, {VK_F8, "F8"}, {VK_F9, "F9"},
        {VK_F10, "F10"}, {VK_F11, "F11"}, {VK_F12, "F12"}, {VK_NUMLOCK, "Num Lock"},
        {VK_SCROLL, "Scroll Lock"}, {VK_OEM_1, ";"}, {VK_OEM_PLUS, "="}, {VK_OEM_COMMA, ","},
        {VK_OEM_MINUS, "-"}, {VK_OEM_PERIOD, "."}, {VK_OEM_2, "/"}, {VK_OEM_3, "`"},
        {VK_OEM_4, "["}, {VK_OEM_5, "\\"}, {VK_OEM_6, "]"}, {VK_OEM_7, "'"},
    };

    const auto it = names.find(vk);
    return it != names.end() ? it->second : nullptr;
}

} // namespace

void normalizeKeyChord(Config::KeyChord& chord) {
    auto& keys = chord.keys;
    std::sort(keys.begin(), keys.end());
    keys.erase(std::unique(keys.begin(), keys.end()), keys.end());
}

void addKeyToChord(Config::KeyChord& chord, int vk) {
    for (int existing : chord.keys) {
        if (existing == vk)
            return;
    }
    chord.keys.push_back(vk);
    normalizeKeyChord(chord);
}

const char* keyDisplayName(int vk) {
    if (vk == 0)
        return "None";
    if (const char* name = lookupKeyName(vk))
        return name;

    static char unknown[32];
    snprintf(unknown, sizeof(unknown), "VK 0x%02X", vk);
    return unknown;
}

std::string chordLabel(const Config::KeyChord& chord) {
    if (chord.empty())
        return "None";

    std::string label;
    for (size_t i = 0; i < chord.keys.size(); ++i) {
        if (i > 0)
            label += " + ";
        label += keyDisplayName(chord.keys[i]);
    }
    return label;
}

bool isBindableKey(int vk) {
    switch (vk) {
    case VK_LBUTTON:
    case VK_RBUTTON:
    case VK_ESCAPE:
    case VK_DELETE:
    case VK_BACK:
    case VK_CAPITAL:
    case VK_NUMLOCK:
    case VK_SCROLL:
    case VK_SHIFT:
    case VK_CONTROL:
    case VK_MENU:
        return false;
    default:
        return vk > 0 && vk < 256;
    }
}

bool chordHeld(const Config::KeyChord& binding) {
    if (binding.empty())
        return false;
    for (int vk : binding.keys) {
        if (!(GetAsyncKeyState(vk) & 0x8000))
            return false;
    }
    return true;
}

Config::KeyChord readHeldBindableKeys() {
    Config::KeyChord chord;
    for (int vk = 1; vk < 256; ++vk) {
        if (isBindableKey(vk) && (GetAsyncKeyState(vk) & 0x8000))
            addKeyToChord(chord, vk);
    }
    return chord;
}

bool anyInputHeld() {
    for (int vk = 0; vk < 256; ++vk) {
        if (GetAsyncKeyState(vk) & 0x8000)
            return true;
    }
    return false;
}

} // namespace kx

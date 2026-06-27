#pragma once

#include <array>
#include <windows.h>

namespace kx {
namespace Constants {

constexpr int APP_VERSION = 20200;
constexpr char APP_NAME[] = "KX Trainer";
constexpr char API_URL[] = "https://kxtools.xyz/api/v1/version/kx-trainer-free";
constexpr wchar_t DLL_FILENAME[] = L"KX-Trainer-Free.dll";
constexpr int MENU_TOGGLE = VK_INSERT;

namespace Scan {
    constexpr uintptr_t BASE_ADDRESS_MIN_VALUE = 0x10000;
    constexpr uintptr_t POINTER_LOCATION_OFFSET = 0x8;
    constexpr int MAX_BASE_SCAN_ATTEMPTS = 15;
    constexpr int BASE_SCAN_RETRY_DELAY_MS = 1000;
}

namespace Patterns {
    inline constexpr char BASE[] = "\x01\x00\x03\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08\x00\x00\x00\x00\x00\x00\x00\x01";
    inline constexpr char BASE_MASK[] = "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
    inline constexpr char FOG[] = "\xF3\x0F\x11\x00\x24\x00\xF3\x0F\x11\x5C\x24\x00\xF3\x0F\x10\x9C\x24";
    inline constexpr char FOG_MASK[] = "xxx?x?xxxxx?xxxxx";
    inline constexpr char OBJECT_CLIPPING[] = "\xD3\x0F\x29\x54\x24\x60\x0F\x28\xCA";
    inline constexpr char OBJECT_CLIPPING_MASK[] = "?xxxxxxxx";
    inline constexpr char FULL_STRAFE[] = "\x75\x00\x00\x28\x4C\x24\x00\x0F\x59\x8B";
    inline constexpr char FULL_STRAFE_MASK[] = "x??xxx?xxx";
}

namespace Offsets {
    constexpr unsigned BYTE1 = 0x50;
    constexpr unsigned BYTE2 = 0x88;
    constexpr unsigned BYTE3 = 0x10;
    constexpr unsigned BYTE4 = 0x68;
}

namespace Chains {
    using Coord = std::array<unsigned, 5>;
    using Field = std::array<unsigned, 4>;

    inline constexpr Coord X{ Offsets::BYTE1, Offsets::BYTE2, Offsets::BYTE3, Offsets::BYTE4, 0x120 };
    inline constexpr Coord Y{ Offsets::BYTE1, Offsets::BYTE2, Offsets::BYTE3, Offsets::BYTE4, 0x128 };
    inline constexpr Coord Z{ Offsets::BYTE1, Offsets::BYTE2, Offsets::BYTE3, Offsets::BYTE4, 0x124 };
    inline constexpr Coord ZHeight1{ Offsets::BYTE1, Offsets::BYTE2, Offsets::BYTE3, Offsets::BYTE4, 0x118 };
    inline constexpr Coord ZHeight2{ Offsets::BYTE1, Offsets::BYTE2, Offsets::BYTE3, Offsets::BYTE4, 0x114 };
    inline constexpr Field Gravity{ Offsets::BYTE1, Offsets::BYTE2, Offsets::BYTE3, 0x1FC };
    inline constexpr Field Speed{ Offsets::BYTE1, Offsets::BYTE2, Offsets::BYTE3, 0x220 };
    inline constexpr Field WallClimb{ Offsets::BYTE1, Offsets::BYTE2, Offsets::BYTE3, 0x204 };
}

namespace Hotkeys {
    constexpr int SAVE_POS = VK_F5;
    constexpr int LOAD_POS = VK_F6;
    constexpr int INVISIBILITY = VK_F7;
    constexpr int WALL_CLIMB = VK_F8;
    constexpr int CLIPPING = VK_F9;
    constexpr int OBJECT_CLIPPING = VK_F10;
    constexpr int FULL_STRAFE = VK_END;
    constexpr int NO_FOG = VK_HOME;
    constexpr int SUPER_SPRINT = VK_RSHIFT;
    constexpr int SPRINT = VK_LSHIFT;
    constexpr int FLY = VK_LCONTROL;
    constexpr int UNINJECT = VK_F2;
}

namespace Settings {
    constexpr float SPRINT_SPEED = 12.22f;
    constexpr float NORMAL_SPEED = 9.1875f;
    constexpr float SUPER_SPRINT_SPEED = 30.0f;
    constexpr float FLY_SPEED = 20.0f;
    constexpr float FLY_NORMAL_SPEED = -40.625f;
    constexpr float WALLCLIMB_SPEED = 20.0f;
    constexpr float WALLCLIMB_NORMAL_SPEED = 2.1875f;
    constexpr byte OBJECT_CLIPPING_ON = 0xDB;
    constexpr byte OBJECT_CLIPPING_OFF = 0xD3;
    constexpr float INVISIBILITY_ON = 2.7f;
    constexpr float INVISIBILITY_OFF = 1.0f;
    constexpr float CLIPPING_ON = 99999.0f;
    constexpr float CLIPPING_OFF = 0.0f;
    constexpr byte FULL_STRAFE_ON = 0x75;
    constexpr byte FULL_STRAFE_OFF = 0x0F;
    constexpr byte NO_FOG_ON = 0x54;
    constexpr byte NO_FOG_OFF = 0x5C;
}

} // namespace Constants
} // namespace kx

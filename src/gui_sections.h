#pragma once

#include <cstddef>

namespace kx {

inline constexpr const char* kSectionNames[] = {
    "Toggles", "Actions", "Hotkeys", "Log", "Info",
};
inline constexpr bool kSectionDefaults[] = { true, true, false, false, false };
inline constexpr std::size_t kSectionCount = sizeof(kSectionNames) / sizeof(kSectionNames[0]);
static_assert(sizeof(kSectionNames) / sizeof(kSectionNames[0]) ==
    sizeof(kSectionDefaults) / sizeof(kSectionDefaults[0]));

} // namespace kx

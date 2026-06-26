#pragma once

#include <shellapi.h>
#include <windows.h>

namespace kx {

inline void openUrl(const char* url) {
    ShellExecuteA(nullptr, "open", url, nullptr, nullptr, SW_SHOWNORMAL);
}

} // namespace kx

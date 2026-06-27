#pragma once

#include <windows.h>

#include <string>

namespace kx {

inline std::wstring moduleDirectory(HMODULE module) {
    wchar_t path[MAX_PATH]{};
    if (!GetModuleFileNameW(module, path, MAX_PATH))
        return {};

    std::wstring directory(path);
    const auto slash = directory.find_last_of(L"\\/");
    if (slash == std::wstring::npos)
        return {};
    directory.resize(slash + 1);
    return directory;
}

inline std::wstring moduleDirectoryForFile(HMODULE module, const wchar_t* filename) {
    std::wstring directory = moduleDirectory(module);
    if (directory.empty())
        return {};
    directory += filename;
    return directory;
}

} // namespace kx

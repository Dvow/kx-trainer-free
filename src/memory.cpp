#define NOMINMAX

#include "memory.h"
#include "log.h"

#include <algorithm>
#include <cstring>
#include <string>

namespace {

bool ReadBytes(uintptr_t address, void* buffer, size_t size) {
    __try {
        memcpy(buffer, reinterpret_cast<const void*>(address), size);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

}

namespace kx {

uintptr_t Memory::ScanMainModule(const char* pattern, const char* mask) const {
    const MODULEENTRY32 module = GetMainModule();
    if (module.th32ModuleID == 0) {
        LogError("ScanMainModule failed: main module not found", false);
        return 0;
    }

    const uintptr_t begin = reinterpret_cast<uintptr_t>(module.modBaseAddr);
    return ScanPatternRange(begin, begin + module.modBaseSize, pattern, mask);
}

uintptr_t Memory::ScanPatternRange(uintptr_t begin, uintptr_t end, const char* pattern, const char* mask) const {
    if (begin >= end)
        return 0;

    const size_t patternLength = strlen(mask);
    if (patternLength == 0)
        return 0;

    constexpr size_t kChunkSize = 4096;
    char buffer[kChunkSize];
    uintptr_t current = begin;

    while (current < end) {
        const size_t remaining = static_cast<size_t>(end - current);
        const size_t readSize = std::min(kChunkSize, remaining);
        if (!ReadBytes(current, buffer, readSize)) {
            current += readSize;
            continue;
        }

        if (const char* found = ScanPatternInternal(buffer, readSize, pattern, mask))
            return current + static_cast<uintptr_t>(found - buffer);

        current += (readSize > patternLength) ? readSize - (patternLength - 1) : readSize;
        if (end - current < patternLength)
            break;
    }

    return 0;
}

MODULEENTRY32 Memory::GetMainModule() const {
    MODULEENTRY32 modEntry{ sizeof(MODULEENTRY32) };
    const DWORD pid = GetCurrentProcessId();
    const HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
    if (snapshot == INVALID_HANDLE_VALUE) {
        modEntry.th32ModuleID = 0;
        return modEntry;
    }

    wchar_t exePath[MAX_PATH]{};
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    const wchar_t* exeName = wcsrchr(exePath, L'\\');
    exeName = exeName ? exeName + 1 : exePath;

    bool found = false;
    if (Module32FirstW(snapshot, &modEntry)) {
        do {
            if (_wcsicmp(modEntry.szModule, exeName) == 0) {
                found = true;
                break;
            }
        } while (Module32NextW(snapshot, &modEntry));
    }

    CloseHandle(snapshot);
    if (!found)
        modEntry.th32ModuleID = 0;
    return modEntry;
}

const char* Memory::ScanPatternInternal(const char* base, size_t size, const char* pattern, const char* mask) const {
    const size_t patternLength = strlen(mask);
    if (patternLength == 0 || size < patternLength)
        return nullptr;

    for (size_t i = 0; i <= size - patternLength; ++i) {
        bool found = true;
        for (size_t j = 0; j < patternLength; ++j) {
            if (mask[j] != '?' && pattern[j] != base[i + j]) {
                found = false;
                break;
            }
        }
        if (found)
            return base + i;
    }
    return nullptr;
}

bool Memory::WriteProtected(uintptr_t address, const void* data, size_t size) const {
    if (!data || size == 0)
        return false;

    void* const ptr = reinterpret_cast<void*>(address);
    DWORD oldProtect = 0;
    if (!VirtualProtect(ptr, size, PAGE_EXECUTE_READWRITE, &oldProtect))
        return false;

    bool ok = false;
    __try {
        memcpy(ptr, data, size);
        ok = true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        ok = false;
    }

    DWORD ignored = 0;
    VirtualProtect(ptr, size, oldProtect, &ignored);
    return ok;
}

void Memory::LogError(const std::string& message, bool includeWinError) const {
    std::string fullMessage = "ERROR: " + message;
    if (includeWinError) {
        const DWORD errorCode = GetLastError();
        if (errorCode != 0)
            fullMessage += " (WinError " + std::to_string(errorCode) + ")";
    }
    Log::add(fullMessage);
}

} // namespace kx

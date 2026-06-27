#pragma once

#include <windows.h>
#include <tlhelp32.h>

#include <array>
#include <cstdint>
#include <cstring>
#include <sstream>
#include <string>

namespace kx {

inline std::string to_hex(uintptr_t address) {
    std::ostringstream oss;
    oss << "0x" << std::hex << std::uppercase << address;
    return oss.str();
}

class Memory {
public:
    template <typename T>
    bool Read(uintptr_t address, T& outValue) const;

    template <typename T>
    bool Write(uintptr_t address, const T& value) const;

    bool WriteProtected(uintptr_t address, const void* data, size_t size) const;

    template <std::size_t N>
    uintptr_t ResolvePointerChain(uintptr_t baseAddress, const std::array<unsigned int, N>& offsets) const;

    uintptr_t ScanMainModule(const char* pattern, const char* mask) const;

private:
    uintptr_t ScanPatternRange(uintptr_t begin, uintptr_t end, const char* pattern, const char* mask) const;
    MODULEENTRY32 GetMainModule() const;
    const char* ScanPatternInternal(const char* base, size_t size, const char* pattern, const char* mask) const;
    void LogError(const std::string& message, bool includeWinError = true) const;
};

template <typename T>
bool Memory::Read(uintptr_t address, T& outValue) const {
    __try {
        outValue = *reinterpret_cast<const T*>(address);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

template <typename T>
bool Memory::Write(uintptr_t address, const T& value) const {
    __try {
        *reinterpret_cast<T*>(address) = value;
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

template <std::size_t N>
uintptr_t Memory::ResolvePointerChain(uintptr_t baseAddress, const std::array<unsigned int, N>& offsets) const {
    uintptr_t currentAddress = baseAddress;
    for (std::size_t i = 0; i < N; ++i) {
        if (!Read(currentAddress, currentAddress) || currentAddress == 0)
            return 0;
        currentAddress += offsets[i];
    }
    return currentAddress;
}

} // namespace kx

#include <windows.h>
#include <tlhelp32.h>

#include "constants.h"
#include "path_util.h"

#include <cstdio>
#include <string>

namespace {

DWORD FindPid(const wchar_t* name) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE)
        return 0;

    PROCESSENTRY32W pe{ sizeof(pe) };
    DWORD pid = 0;
    if (Process32FirstW(snap, &pe)) {
        do {
            if (_wcsicmp(pe.szExeFile, name) == 0) {
                pid = pe.th32ProcessID;
                break;
            }
        } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);
    return pid;
}

bool IsLoaded(DWORD pid) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
    if (snap == INVALID_HANDLE_VALUE)
        return false;

    MODULEENTRY32W me{ sizeof(me) };
    bool found = false;
    if (Module32FirstW(snap, &me)) {
        do {
            if (_wcsicmp(me.szModule, kx::Constants::DLL_FILENAME) == 0) {
                found = true;
                break;
            }
        } while (Module32NextW(snap, &me));
    }
    CloseHandle(snap);
    return found;
}

std::wstring DllPath() {
    return kx::moduleDirectoryForFile(nullptr, kx::Constants::DLL_FILENAME);
}

bool Inject(DWORD pid, const std::wstring& dll) {
    if (IsLoaded(pid)) {
        std::fwprintf(stderr, L"Already injected - use Uninject in-game first.\n");
        return false;
    }

    HANDLE proc = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION |
                                  PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ,
                              FALSE, pid);
    if (!proc) {
        std::fwprintf(stderr, L"OpenProcess failed (%lu)\n", GetLastError());
        return false;
    }

    const size_t bytes = (dll.size() + 1) * sizeof(wchar_t);
    void* remote = VirtualAllocEx(proc, nullptr, bytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!remote) {
        std::fwprintf(stderr, L"VirtualAllocEx failed (%lu)\n", GetLastError());
        CloseHandle(proc);
        return false;
    }

    SIZE_T written = 0;
    if (!WriteProcessMemory(proc, remote, dll.c_str(), bytes, &written) || written != bytes) {
        std::fwprintf(stderr, L"WriteProcessMemory failed (%lu)\n", GetLastError());
        VirtualFreeEx(proc, remote, 0, MEM_RELEASE);
        CloseHandle(proc);
        return false;
    }

    const auto loadLib = reinterpret_cast<LPTHREAD_START_ROUTINE>(
        GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "LoadLibraryW"));

    HANDLE thread = CreateRemoteThread(proc, nullptr, 0, loadLib, remote, 0, nullptr);
    if (!thread) {
        std::fwprintf(stderr, L"CreateRemoteThread failed (%lu)\n", GetLastError());
        VirtualFreeEx(proc, remote, 0, MEM_RELEASE);
        CloseHandle(proc);
        return false;
    }

    WaitForSingleObject(thread, INFINITE);
    DWORD code = 0;
    GetExitCodeThread(thread, &code);
    CloseHandle(thread);
    VirtualFreeEx(proc, remote, 0, MEM_RELEASE);
    CloseHandle(proc);

    if (!code) {
        std::fwprintf(stderr, L"LoadLibraryW returned NULL (missing dependency?)\n");
        return false;
    }
    return true;
}

}

int wmain(int argc, wchar_t** argv) {
    const wchar_t* target = (argc >= 2) ? argv[1] : L"Gw2-64.exe";

    const DWORD pid = FindPid(target);
    if (!pid) {
        std::fwprintf(stderr, L"Process not found: %s\n", target);
        return 1;
    }

    const std::wstring dll = DllPath();
    if (GetFileAttributesW(dll.c_str()) == INVALID_FILE_ATTRIBUTES) {
        std::fwprintf(stderr, L"DLL not found: %s\n", dll.c_str());
        return 1;
    }

    std::wprintf(L"Injecting %s into %s (PID %lu)\n", dll.c_str(), target, pid);
    if (!Inject(pid, dll)) {
        std::fwprintf(stderr, L"Injection failed\n");
        return 1;
    }

    std::wprintf(L"OK - press Insert in-game to toggle the menu.\n");
    return 0;
}

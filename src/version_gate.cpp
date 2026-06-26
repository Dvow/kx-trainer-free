#include "version_gate.h"

#include "constants.h"
#include "log.h"
#include "shell_util.h"

#include <windows.h>

#include <string>
#include <vector>

#include "internal/nlohmann/json.hpp"

namespace kx {
namespace {

bool fetchUrl(const std::string& url, std::string& body) {
    wchar_t tmp[MAX_PATH]{};
    GetTempPathW(MAX_PATH, tmp);
    wchar_t out[MAX_PATH]{};
    GetTempFileNameW(tmp, L"kx", 0, out);

    wchar_t sys[MAX_PATH]{};
    GetSystemDirectoryW(sys, MAX_PATH);
    std::wstring cmd = L"\"" + std::wstring(sys) + L"\\curl.exe\" -sSf -m 15 -o \"" +
        std::wstring(out) + L"\" \"" + std::wstring(url.begin(), url.end()) + L"\"";
    std::vector<wchar_t> buf(cmd.begin(), cmd.end());
    buf.push_back(L'\0');

    STARTUPINFOW si{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    PROCESS_INFORMATION pi{};
    if (!CreateProcessW(nullptr, buf.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW,
            nullptr, nullptr, &si, &pi)) {
        DeleteFileW(out);
        return false;
    }

    WaitForSingleObject(pi.hProcess, 20000);
    DWORD code = 1;
    GetExitCodeProcess(pi.hProcess, &code);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    if (code != 0) {
        DeleteFileW(out);
        return false;
    }

    HANDLE f = CreateFileW(out, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
    DeleteFileW(out);
    if (f == INVALID_HANDLE_VALUE)
        return false;

    char data[4096]{};
    DWORD n = 0;
    ReadFile(f, data, sizeof(data) - 1, &n, nullptr);
    CloseHandle(f);
    body.assign(data, n);
    return true;
}

} // namespace

bool VersionGate::check() {
    lastMessage_.clear();
#ifdef _DEBUG
    return true;
#endif

    std::string body;
    if (!fetchUrl(Constants::API_URL, body)) {
        lastMessage_ = "Could not reach the update server.";
        Log::add(lastMessage_);
        return false;
    }

    nlohmann::json response = nlohmann::json::parse(body, nullptr, false);
    if (response.is_discarded() || !response.is_object()) {
        lastMessage_ = "Invalid update server response.";
        Log::add(lastMessage_);
        return false;
    }

    if (!response.contains("status") || !response.contains("version") ||
        !response.contains("message") || !response.contains("download")) {
        lastMessage_ = "Invalid update server response.";
        Log::add(lastMessage_);
        return false;
    }

    const int apiStatus = response.value("status", 0);
    const int apiVersion = response.value("version", 0);
    const std::string message = response.value("message", std::string{});
    const std::string download = response.value("download", std::string{});

    if (apiStatus == 1) {
        if (apiVersion <= Constants::APP_VERSION)
            return true;
        lastMessage_ = "Update required - opening download page.";
        Log::add(lastMessage_);
        if (!download.empty())
            openUrl(download.c_str());
        return false;
    }

    lastMessage_ = message.empty() ? std::string(Constants::APP_NAME) + " is currently disabled." : message;
    Log::add(lastMessage_);
    return false;
}

} // namespace kx

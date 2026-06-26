#include "hooks.h"
#include "overlay.h"

#include "MinHook.h"

#include <d3d11.h>
#include <dxgi.h>
#include <dxgi1_2.h>

namespace kx {
namespace {

volatile LONG g_uninjecting = 0;
HMODULE g_module = nullptr;

using PresentFn = HRESULT(WINAPI*)(IDXGISwapChain*, UINT, UINT);
using Present1Fn = HRESULT(WINAPI*)(IDXGISwapChain1*, UINT, UINT, const DXGI_PRESENT_PARAMETERS*);
using ResizeFn = HRESULT(WINAPI*)(IDXGISwapChain*, UINT, UINT, UINT, DXGI_FORMAT, UINT);

PresentFn oPresent = nullptr;
Present1Fn oPresent1 = nullptr;
ResizeFn oResize = nullptr;

bool Hook(void* target, void* detour, void** original) {
    MH_STATUS st = MH_CreateHook(target, detour, original);
    if (st == MH_ERROR_ALREADY_CREATED) {
        MH_RemoveHook(target);
        st = MH_CreateHook(target, detour, original);
    }
    return st == MH_OK;
}

bool GetDxgiVtable(void** present, void** present1, void** resize) {
    WNDCLASSEXW wc{ sizeof(wc), CS_CLASSDC, DefWindowProcW, 0, 0, GetModuleHandleW(nullptr),
                    nullptr, nullptr, nullptr, nullptr, L"KX_HOOK", nullptr };
    RegisterClassExW(&wc);
    HWND hwnd = CreateWindowW(wc.lpszClassName, L"", WS_OVERLAPPEDWINDOW, 0, 0, 100, 100,
                              nullptr, nullptr, wc.hInstance, nullptr);

    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferCount = 1;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hwnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    IDXGISwapChain* sc = nullptr;
    ID3D11Device* dev = nullptr;
    ID3D11DeviceContext* ctx = nullptr;
    D3D_FEATURE_LEVEL fl{};
    const D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_0 };

    const HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, levels, 1, D3D11_SDK_VERSION,
        &sd, &sc, &dev, &fl, &ctx);

    bool ok = false;
    if (SUCCEEDED(hr) && sc) {
        auto** vtbl = *reinterpret_cast<void***>(sc);
        *present = vtbl[8];
        *resize = vtbl[13];
        *present1 = nullptr;
        IDXGISwapChain1* sc1 = nullptr;
        if (SUCCEEDED(sc->QueryInterface(IID_PPV_ARGS(&sc1))) && sc1) {
            *present1 = (*reinterpret_cast<void***>(sc1))[22];
            sc1->Release();
        }
        ok = *present && *resize;
    }

    if (ctx) ctx->Release();
    if (dev) dev->Release();
    if (sc) sc->Release();
    DestroyWindow(hwnd);
    UnregisterClassW(wc.lpszClassName, wc.hInstance);
    return ok;
}

void OnPresent(IDXGISwapChain* swapChain) {
    if (g_uninjecting)
        return;

    static thread_local bool active = false;
    if (active)
        return;
    active = true;

    if (!Overlay_IsReady()) {
        ID3D11Device* device = nullptr;
        if (SUCCEEDED(swapChain->GetDevice(IID_PPV_ARGS(&device))) && device) {
            Overlay_Init(swapChain, device);
            device->Release();
        }
    }
    if (Overlay_IsReady())
        Overlay_OnPresent(swapChain);

    active = false;
}

HRESULT WINAPI hkPresent(IDXGISwapChain* sc, UINT a, UINT b) {
    OnPresent(sc);
    return oPresent(sc, a, b);
}

HRESULT WINAPI hkPresent1(IDXGISwapChain1* sc, UINT a, UINT b, const DXGI_PRESENT_PARAMETERS* p) {
    OnPresent(sc);
    return oPresent1(sc, a, b, p);
}

HRESULT WINAPI hkResize(IDXGISwapChain* sc, UINT a, UINT b, UINT c, DXGI_FORMAT d, UINT e) {
    const HRESULT hr = oResize(sc, a, b, c, d, e);
    if (SUCCEEDED(hr) && Overlay_IsReady())
        Overlay_OnResize();
    return hr;
}

DWORD WINAPI HooksInitThread(LPVOID) {
    const MH_STATUS st = MH_Initialize();
    if (st != MH_OK && st != MH_ERROR_ALREADY_INITIALIZED)
        return 1;

    void* present = nullptr;
    void* present1 = nullptr;
    void* resize = nullptr;
    if (!GetDxgiVtable(&present, &present1, &resize))
        return 1;

    if (!Hook(present, &hkPresent, reinterpret_cast<void**>(&oPresent)))
        return 1;
    if (!Hook(resize, &hkResize, reinterpret_cast<void**>(&oResize)))
        return 1;

    MH_EnableHook(present);
    MH_EnableHook(resize);
    if (present1 && Hook(present1, &hkPresent1, reinterpret_cast<void**>(&oPresent1)))
        MH_EnableHook(present1);

    return 0;
}

DWORD WINAPI UninjectThread(LPVOID) {
    Sleep(100);
    Hooks_Shutdown();
    Sleep(200);
    if (g_module)
        FreeLibraryAndExitThread(g_module, 0);
    return 0;
}

}

bool Hooks_Init(HMODULE module) {
    g_module = module;
    HANDLE t = CreateThread(nullptr, 0, HooksInitThread, nullptr, 0, nullptr);
    if (!t)
        return false;
    CloseHandle(t);
    return true;
}

void Hooks_Shutdown() {
    static bool shutDown = false;
    if (shutDown)
        return;
    shutDown = true;

    Overlay_Shutdown();
    MH_DisableHook(MH_ALL_HOOKS);
    MH_Uninitialize();
}

void Hooks_Uninject() {
    if (InterlockedCompareExchange(&g_uninjecting, 1, 0) != 0)
        return;

    HANDLE t = CreateThread(nullptr, 0, UninjectThread, nullptr, 0, nullptr);
    if (t)
        CloseHandle(t);
}

}

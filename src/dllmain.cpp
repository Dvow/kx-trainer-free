#include "internal/app.h"
#include "internal/hooks.h"

#include <windows.h>

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID reserved) {
    (void)reserved;

    switch (reason) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(module);
        kx::App_Start();
        kx::Hooks_Init(module);
        break;
    case DLL_PROCESS_DETACH:
        kx::Hooks_Shutdown();
        break;
    }
    return TRUE;
}

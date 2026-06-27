#pragma once

#include <windows.h>

namespace kx {

bool Hooks_Init(HMODULE module);
void Hooks_Shutdown();
void Hooks_Unload();

}

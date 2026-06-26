#pragma once

#include <d3d11.h>
#include <dxgi.h>

namespace kx {

bool Overlay_Init(IDXGISwapChain* swapChain, ID3D11Device* device);
void Overlay_OnPresent(IDXGISwapChain* swapChain);
void Overlay_OnResize();
void Overlay_Shutdown();
bool Overlay_IsReady();

}

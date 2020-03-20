#pragma once

#include <d3d11.h>
#include <d3dcompiler.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "D3DCompiler.lib")




namespace Hooks
{
	extern long __stdcall hk_present(IDXGISwapChain* p_swapchain, unsigned int syncintreval, unsigned int flags);
	extern LRESULT hkWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	extern void InitHook();

}
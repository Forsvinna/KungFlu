#include <windows.h>
#include <cstdint>
#include <thread>
#include <chrono>
#include <mutex>
#include <vector>
#include "hooks.h"





bool DllMain(HMODULE module_instance, DWORD call_reason, void*)
{
	if (call_reason != DLL_PROCESS_ATTACH)
		return false;

	wchar_t file_name[MAX_PATH] = L"";
	GetModuleFileNameW(module_instance, file_name, _countof(file_name));
	LoadLibraryW(file_name);

	return true;
}

extern "C" __declspec(dllexport)
LRESULT msg_hk(int32_t code, WPARAM wparam, LPARAM lparam)
{
	static auto done_once = false;

	const auto pmsg = reinterpret_cast<MSG*>(lparam);

	if (!done_once && pmsg->message == 0x5b0)
	{
		UnhookWindowsHookEx(reinterpret_cast<HHOOK>(lparam));

		Hooks::InitHook();

		done_once = true;
	}

	return CallNextHookEx(nullptr, code, wparam, lparam);
}

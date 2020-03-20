#include <cstdio> // printf
#include "hooks.h"
#include "utils.hpp"

#pragma warning(disable : 4996)


/*Discord API*/

typedef int(__stdcall* createhook_fn)(LPVOID pTarget, LPVOID pDetour, LPVOID* ppOriginal);
createhook_fn CreateHook = nullptr;

typedef int(__stdcall* enablehook_fn)(LPVOID pTarget, BOOL enable);
enablehook_fn EnableHook = nullptr;

typedef int(__stdcall* applyqueued_fn)(VOID);
applyqueued_fn EnableHookQue = nullptr;

typedef long(__stdcall* present_fn) (IDXGISwapChain* p_swapchain, UINT syncintreval, UINT flags);
present_fn o_present = nullptr;

/*Discord API*/

static float width = 0;
static float height = 0;

DWORD64 G::GameUnityBase;
HWND G::GameWnd;
bool G::PressedKeys[256];
Player* G::LocalPlayer;

LOCALGAMEWORLD* G::gameworld = nullptr;
CameraEntity* G::camera = nullptr;

Variables Vars;
geo::vec2_t WndSize;

std::vector<Player*> entities;

namespace Hooks
{

	ID3D11Device* g_pdevice = nullptr;
	ID3D11DeviceContext* g_pcontext = nullptr;
	ID3D11RenderTargetView* g_prendertargetview = nullptr;
	WNDPROC o_wndproc = nullptr;
}


void client_loop()
{
	entities.clear();

	if (!G::gameworld->m_pPlayerList)
		return;

	for (int i = 0; i < G::gameworld->m_pPlayerList->Count; i++)
	{
		const auto player = *reinterpret_cast<Player**>(std::uintptr_t(G::gameworld->m_pPlayerList->m_pList) + (0x20 + (i * 8)));

		if (!player || std::find(entities.begin(), entities.end(), player) != entities.end())
			continue;

		if (!G::LocalPlayer)
		{
			if (player->m_pLocalPlayerChecker)
				G::LocalPlayer = player;
		}


		entities.push_back(player);

	}
}

void client_render()
{
	for (const auto& entity : entities)
	{
		if (!entity)
			continue;

		if (entity == G::LocalPlayer)
			continue;

		//geo::vector2 screennike;
		//geo::vector3 outpos;

		geo::vector2 screennbase, screenHead;
		geo::vector3 origin, head;

		unity::GetBoneByID(entity, Bones::HumanHead, head);
		unity::GetBoneByID(entity, Bones::HumanBase, origin);

		float distance = unity::localplayer::get_localplayerpos().distance(origin);

		if (distance > 250.f)
			continue;

		unity::DrawSkeleton(entity);

		if (unity::world_to_screen(origin, screennbase) && unity::world_to_screen(head, screenHead))
		{
			d3d::DrawString(11, geo::vector2(screennbase.x, screennbase.y), LIGHT_YELLOW, true, true, E("%0.2fm"), distance);
			d3d::DrawCircle(screenHead, 5.f, LIGHT_RED, 10, 2.f);

			if(unity::IsPlayer(entity))
				d3d::DrawString(9, geo::vector2(screennbase.x, screennbase.y + 10), WHITE, true, true, E("LIVE"));
		}

		//printf("%i \n", entity->m_pPlayerProfile->m_PlayerInfo->CreationDate);
	}
}

long __stdcall Hooks::hk_present(IDXGISwapChain* p_swapchain, unsigned int syncintreval, unsigned int flags)
{
	static bool bOnce = true;
	if (bOnce)
	{
		p_swapchain->GetDevice(__uuidof(g_pdevice), reinterpret_cast<void**>(&g_pdevice));
		g_pdevice->GetImmediateContext(&g_pcontext);
		ID3D11Texture2D* pBackBuffer;
		p_swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBuffer);
		D3D11_TEXTURE2D_DESC backBufferDesc = { 0 };
		pBackBuffer->GetDesc(&backBufferDesc);


		g_pdevice->CreateRenderTargetView(pBackBuffer, NULL, &g_prendertargetview);

		width = (float)backBufferDesc.Width;
		height = (float)backBufferDesc.Height;

		pBackBuffer->Release();

		ImGui::GetIO().Fonts->AddFontFromFileTTF(E("C:\\Windows\\Fonts\\consola.ttf"), 12.0f);
		ImGui_ImplDX11_Init(G::GameWnd, g_pdevice, g_pcontext);
		ImGui_ImplDX11_CreateDeviceObjects();

		bOnce = false;
	}

	g_pcontext->OMSetRenderTargets(1, &g_prendertargetview, NULL);
	ImGui_ImplDX11_NewFrame();


	d3d::DrawString(12, geo::vector2(65, 52), 0xFFEB2EFE, true, true, E("uGameManager:%llX"), OFFSET::uGameManager - G::GameUnityBase); //std::printf("uGameManager:%llX\n", OFFSET::uGameManager - G::GameUnityBase);

	if (G::gameworld && G::camera)
	{
		d3d::DrawString(12, geo::vector2(49, 68), 0xFFEB2EFE, true, true, E("players: %i"), G::gameworld->m_pPlayerList->Count);

		if (G::gameworld->m_pPlayerList->Count > 0)
		{
			//Obtain entities
			client_loop();

			//draw entities
			client_render();

			float xs = width / 2, ys = height / 2;
			d3d::DrawCircle(geo::vector2(xs, ys), 10.f, BLACK, 10, 3.f);
			d3d::DrawCircle(geo::vector2(xs, ys), 10.f, LIGHT_RED, 10, 1.5f);
		

		}

	}

	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
	return o_present(p_swapchain, syncintreval, flags);
}


extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT Hooks::hkWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	ImGuiIO& io = ImGui::GetIO();
	switch (uMsg)
	{
	case WM_SIZE:
	{

		break;
	}
	case WM_LBUTTONDOWN:
		io.MouseDown[0] = true;
		G::PressedKeys[VK_LBUTTON] = true;
		break;
	case WM_LBUTTONUP:
		io.MouseDown[0] = false;
		G::PressedKeys[VK_LBUTTON] = false;
		break;
	case WM_RBUTTONDOWN:
		io.MouseDown[1] = true;
		G::PressedKeys[VK_RBUTTON] = true;
		break;
	case WM_RBUTTONUP:
		io.MouseDown[1] = false;
		G::PressedKeys[VK_RBUTTON] = false;
		break;
	case WM_MBUTTONDOWN:
		io.MouseDown[2] = true;
		G::PressedKeys[VK_MBUTTON] = true;
		break;
	case WM_MBUTTONUP:
		io.MouseDown[2] = false;
		G::PressedKeys[VK_MBUTTON] = false;
		break;
	case WM_MOUSEWHEEL:
		io.MouseWheel += GET_WHEEL_DELTA_WPARAM(wParam) > 0 ? 1.0f : -1.0f;
		break;
	case WM_MOUSEMOVE:
		io.MousePos.x = (signed short)(lParam);
		io.MousePos.y = (signed short)(lParam >> 16);
		break;
	case WM_KEYDOWN:
		G::PressedKeys[wParam] = true;
		if (wParam == 52)
			break;
		if (wParam < 256)
			io.KeysDown[wParam] = 1;
		break;
	case WM_KEYUP:
		G::PressedKeys[wParam] = false;
		if (wParam == 52)
			break;
		if (wParam < 256)
			io.KeysDown[wParam] = 0;
		break;
	case WM_XBUTTONDOWN:
	{
		UINT button = GET_XBUTTON_WPARAM(wParam);
		if (button == XBUTTON1)
		{
			G::PressedKeys[VK_XBUTTON1] = true;
		}
		else if (button == XBUTTON2)
		{
			G::PressedKeys[VK_XBUTTON2] = true;
		}
		break;
	}
	case WM_XBUTTONUP:
	{
		UINT button = GET_XBUTTON_WPARAM(wParam);
		if (button == XBUTTON1)
		{
			G::PressedKeys[VK_XBUTTON1] = false;
		}
		else if (button == XBUTTON2)
		{
			G::PressedKeys[VK_XBUTTON2] = false;
		}
		break;
	}
	}
	if (G::PressedKeys[VK_HOME])
	{
		unity::obtain_objects(*reinterpret_cast<void**>(OFFSET::uGameManager), true, E("GameWorld"));
		unity::obtain_objects(*reinterpret_cast<void**>(OFFSET::uGameManager), false, E("FPS Camera"));
	}
	if (G::PressedKeys[VK_END])
	{
		G::gameworld = nullptr;
		G::camera = nullptr;
		entities.clear();
		G::LocalPlayer = nullptr;
	}

	return CallWindowProc(o_wndproc, hWnd, uMsg, wParam, lParam);
}

void Hooks::InitHook()
{
	G::GameUnityBase = (DWORD64)utils::memory::GetModuleBase_Wrapper(E("UnityPlayer.dll"));

	G::GameWnd = FindWindowA(E("UnityWndClass"), NULL);

	const auto game_object_manager_address = utils::memory::FindPattern(E("48 8B 15 ? ? ? ? 66 39"), E("UnityPlayer.dll"));

	OFFSET::uGameManager = reinterpret_cast<std::uintptr_t>(game_object_manager_address + *reinterpret_cast<std::int32_t*>(game_object_manager_address + 3) + 7);
	//std::printf("uGameManager:%llX\n", OFFSET::uGameManager - G::GameUnityBase);



	const auto dwpresent = utils::memory::FindPattern(E("48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC 20 48 8B D9 41 8B F8"), E("DiscordHook64.dll"));
	CreateHook = (createhook_fn)(utils::memory::FindPattern(E("40 53 55 56 57 41 54 41 56 41 57 48 83 EC 60"), E("DiscordHook64.dll")));
	EnableHook = (enablehook_fn)(utils::memory::FindPattern(E("48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 41 56 41 57 48 83 EC 20 33 F6 8B FA"), E("DiscordHook64.dll")));
	EnableHookQue = (applyqueued_fn)(utils::memory::FindPattern(E("48 89 5C 24 ? 48 89 6C 24 ? 48 89 7C 24 ? 41 57"), E("DiscordHook64.dll")));

	
	ImGui::CreateContext();

	o_wndproc = (WNDPROC)SetWindowLongPtr(G::GameWnd, GWLP_WNDPROC, (DWORD_PTR)Hooks::hkWndProc);


	CreateHook((void*)dwpresent, (void*)hk_present, (void**)&o_present);
	EnableHook((void*)dwpresent, 1);
	EnableHookQue();

}


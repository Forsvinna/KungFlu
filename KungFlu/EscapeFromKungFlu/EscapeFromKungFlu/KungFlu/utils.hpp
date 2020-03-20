#pragma once
#include <windows.h>
#include <memory>
#include <string>
#include <sstream>
#include <array>
#include <algorithm>
#include <optional>
#include <vector>
#include <atomic>
#include <codecvt>
#include <winternl.h>
#include <list>


#include "vector.hpp"
#include "classes.hpp"
#include "vars.h"
#include "xor.h"
#include "color.h"

#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui.h"



std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;

#define UTF8_TO_UTF16(str) converter.from_bytes(str).c_str( )
#define UTF16_TO_UTF8(str) converter.to_bytes(str).c_str( )
#define ToLower(Char) ((Char >= 'A' && Char <= 'Z') ? (Char + 32) : Char)

#define PI (3.141592653589793f)
#define ReadPointer(base, offset) (*(PVOID *)(((PBYTE)base + offset)))
#define ReadDWORD(base, offset) (*(PDWORD)(((PBYTE)base + offset)))
#define ReadBYTE(base, offset) (*(((PBYTE)base + offset)))


#define RELATIVE_ADDR(addr, size) ((PBYTE)((UINT_PTR)(addr) + *(PINT)((UINT_PTR)(addr) + ((size) - sizeof(INT))) + (size)))

typedef struct LDR_DATA_TABLE_ENTRY_FIX {
	PVOID Reserved1[2];
	LIST_ENTRY InMemoryOrderLinks;
	PVOID Reserved2[2];
	PVOID DllBase;
	PVOID Reserved3[1];
	ULONG64 SizeOfImage;
	UNICODE_STRING FullDllName;
	UNICODE_STRING BaseDllName;
	PVOID Reserved5[2];
#pragma warning(push)
#pragma warning(disable: 4201)
	union {
		ULONG CheckSum;
		PVOID Reserved6;
	} DUMMYUNIONNAME;
#pragma warning(pop)
	ULONG TimeDateStamp;
};

typedef struct {
	bool Aimbot;
	bool AutoAimbot;
	bool SilentAimbot;
	bool NoSpreadAimbot;
	float AimbotFOV;
	float AimbotSlow;
	bool InstantReload;
	float FOV;

	struct {
		bool AimbotFOV;
		bool Players;
		bool PlayerLines;
		bool PlayerNames;
		float PlayerVisibleColor[3];
		float PlayerNotVisibleColor[3];
		bool Ammo;
		bool Containers;
		bool Weapons;
		INT MinWeaponTier;
	} ESP;
} SETTINGS;


namespace G
{
	extern HINSTANCE MoudleBase;
	extern DWORD64 GameUnityBase;
	extern HWND GameWnd;
	extern bool PressedKeys[256];
	extern LOCALGAMEWORLD* gameworld;
	extern CameraEntity* camera;
	extern Player* LocalPlayer;

}

SETTINGS Settings = { 0 };
namespace SettingsHelper {
	VOID SaveSettings() {
		CHAR path[0xFF];
		GetTempPathA(sizeof(path) / sizeof(path[0]), path);
		strcat(path, E("fnambt.settings"));

		auto file = fopen(path, E("wb"));
		if (file) {
			fwrite(&Settings, sizeof(Settings), 1, file);
			fclose(file);
		}
	}

	VOID ResetSettings() {
		Settings = { 0 };
		Settings.Aimbot = true;
		Settings.AutoAimbot = false;
		Settings.NoSpreadAimbot = true;
		Settings.AimbotFOV = 100.0f;
		Settings.AimbotSlow = 0.0f;
		Settings.InstantReload = true;
		Settings.FOV = 120.0f;
		Settings.ESP.AimbotFOV = true;
		Settings.ESP.Players = true;
		Settings.ESP.PlayerNames = true;
		Settings.ESP.PlayerVisibleColor[0] = 1.0f;
		Settings.ESP.PlayerVisibleColor[1] = 0.0f;
		Settings.ESP.PlayerVisibleColor[2] = 0.0f;
		Settings.ESP.PlayerNotVisibleColor[0] = 1.0f;
		Settings.ESP.PlayerNotVisibleColor[1] = 0.08f;
		Settings.ESP.PlayerNotVisibleColor[2] = 0.6f;
		Settings.ESP.Containers = true;
		Settings.ESP.Weapons = true;
		Settings.ESP.MinWeaponTier = 4;

		SaveSettings();
	}

	VOID Initialize() {
		CHAR path[0xFF] = { 0 };
		GetTempPathA(sizeof(path) / sizeof(path[0]), path);
		strcat(path, E("fnambt.settings"));

		auto file = fopen(path, E("rb"));
		if (file) {
			fseek(file, 0, SEEK_END);
			auto size = ftell(file);

			if (size == sizeof(Settings)) {
				fseek(file, 0, SEEK_SET);
				fread(&Settings, sizeof(Settings), 1, file);
				fclose(file);
			}
			else {
				fclose(file);
				ResetSettings();
			}
		}
		else {
			ResetSettings();
		}
	}
}

namespace OFFSET
{
	std::uintptr_t uGameManager;
}

namespace utils
{
	namespace memory
	{		
		template <typename t> t read( std::uintptr_t const address )
		{
			return *reinterpret_cast< t* >( address );
		}

		template <typename t> void write( std::uintptr_t const address, t data )
		{
			*reinterpret_cast< t* >( address ) = data;
		}

		//StrCompare (with StrInStrI(Two = false))
		template <typename StrType, typename StrType2>
		bool StrCmp(StrType Str, StrType2 InStr, bool Two) {
			if (!Str || !InStr) return false;
			wchar_t c1, c2; do {
				c1 = *Str++; c2 = *InStr++;
				c1 = ToLower(c1); c2 = ToLower(c2);
				if (!c1 && (Two ? !c2 : 1)) return true;
			} while (c1 == c2); return false;
		}

		//GetModuleBase
		template <typename StrType>
		PBYTE GetModuleBase_Wrapper(StrType ModuleName) {
			PPEB_LDR_DATA Ldr = ((PTEB)__readgsqword(FIELD_OFFSET(NT_TIB, Self)))->ProcessEnvironmentBlock->Ldr; void* ModBase = nullptr;
			for (PLIST_ENTRY CurEnt = Ldr->InMemoryOrderModuleList.Flink; CurEnt != &Ldr->InMemoryOrderModuleList; CurEnt = CurEnt->Flink) {
				LDR_DATA_TABLE_ENTRY_FIX* pEntry = CONTAINING_RECORD(CurEnt, LDR_DATA_TABLE_ENTRY_FIX, InMemoryOrderLinks);
				if (!ModuleName || StrCmp(ModuleName, pEntry->BaseDllName.Buffer, false)) return (PBYTE)pEntry->DllBase;
			} return nullptr;
		}

		//Signature Scan
		PBYTE FindPattern(const char* Pattern, const char* Module = nullptr)
		{
			//find pattern utils
         #define InRange(x, a, b) (x >= a && x <= b) 
         #define GetBits(x) (InRange(x, '0', '9') ? (x - '0') : ((x - 'A') + 0xA))
         #define GetByte(x) ((BYTE)(GetBits(x[0]) << 4 | GetBits(x[1])))

         //get module range
			PBYTE ModuleStart = (PBYTE)GetModuleBase_Wrapper(Module); if (!ModuleStart) return nullptr;
			PIMAGE_NT_HEADERS NtHeader = ((PIMAGE_NT_HEADERS)(ModuleStart + ((PIMAGE_DOS_HEADER)ModuleStart)->e_lfanew));
			PBYTE ModuleEnd = (PBYTE)(ModuleStart + NtHeader->OptionalHeader.SizeOfImage - 0x1000); ModuleStart += 0x1000;

			//scan pattern main
			PBYTE FirstMatch = nullptr;
			const char* CurPatt = Pattern;
			for (; ModuleStart < ModuleEnd; ++ModuleStart)
			{
				bool SkipByte = (*CurPatt == '\?');
				if (SkipByte || *ModuleStart == GetByte(CurPatt)) {
					if (!FirstMatch) FirstMatch = ModuleStart;
					SkipByte ? CurPatt += 2 : CurPatt += 3;
					if (CurPatt[-1] == 0) return FirstMatch;
				}

				else if (FirstMatch) {
					ModuleStart = FirstMatch;
					FirstMatch = nullptr;
					CurPatt = Pattern;
				}
			}

			return nullptr;
		}
	}
	
	namespace math
	{
		constexpr auto r2d = 57.2957795131f; /* 180 / pi, used for conversion from radians to degrees */
		constexpr auto d2r = 0.01745329251f; /* pi / 180, used for conversion from degrees to radians */

	
		void ToMatrixWithScale(float* in, float out[4][4]) {
			auto* rotation = &in[0];
			auto* translation = &in[4];
			auto* scale = &in[8];

			out[3][0] = translation[0];
			out[3][1] = translation[1];
			out[3][2] = translation[2];

			auto x2 = rotation[0] + rotation[0];
			auto y2 = rotation[1] + rotation[1];
			auto z2 = rotation[2] + rotation[2];

			auto xx2 = rotation[0] * x2;
			auto yy2 = rotation[1] * y2;
			auto zz2 = rotation[2] * z2;
			out[0][0] = (1.0f - (yy2 + zz2)) * scale[0];
			out[1][1] = (1.0f - (xx2 + zz2)) * scale[1];
			out[2][2] = (1.0f - (xx2 + yy2)) * scale[2];

			auto yz2 = rotation[1] * z2;
			auto wx2 = rotation[3] * x2;
			out[2][1] = (yz2 - wx2) * scale[2];
			out[1][2] = (yz2 + wx2) * scale[1];

			auto xy2 = rotation[0] * y2;
			auto wz2 = rotation[3] * z2;
			out[1][0] = (xy2 - wz2) * scale[1];
			out[0][1] = (xy2 + wz2) * scale[0];

			auto xz2 = rotation[0] * z2;
			auto wy2 = rotation[3] * y2;
			out[2][0] = (xz2 + wy2) * scale[2];
			out[0][2] = (xz2 - wy2) * scale[0];

			out[0][3] = 0.0f;
			out[1][3] = 0.0f;
			out[2][3] = 0.0f;
			out[3][3] = 1.0f;
		}

		float Normalize(float angle) {
			float a = (float)fmod(fmod(angle, 360.0) + 360.0, 360.0);
			if (a > 180.0f) {
				a -= 360.0f;
			}
			return a;
		}

	   void CalcAngle(float* src, float* dst, float* angles) {
			float rel[3] = {
				dst[0] - src[0],
				dst[1] - src[1],
				dst[2] - src[2],
			};

			auto dist = sqrtf(rel[0] * rel[0] + rel[1] * rel[1] + rel[2] * rel[2]);
			auto yaw = atan2f(rel[1], rel[0]) * (180.0f / PI);
			auto pitch = (-((acosf((rel[2] / dist)) * 180.0f / PI) - 90.0f));

			angles[0] = Normalize(pitch);
			angles[1] = Normalize(yaw);
		}

		void MultiplyMatrices(float a[4][4], float b[4][4], float out[4][4]) {
			for (auto r = 0; r < 4; ++r) {
				for (auto c = 0; c < 4; ++c) {
					auto sum = 0.0f;

					for (auto i = 0; i < 4; ++i) {
						sum += a[r][i] * b[i][c];
					}

					out[r][c] = sum;
				}
			}
		}
	}
	

}


namespace Hexto 
{
	std::wstring MBytesToWString(const char* lpcszString)
	{
		int len = strlen(lpcszString);
		int unicodeLen = ::MultiByteToWideChar(CP_ACP, 0, lpcszString, -1, NULL, 0);
		wchar_t* pUnicode = new wchar_t[unicodeLen + 1];
		memset(pUnicode, 0, (unicodeLen + 1) * sizeof(wchar_t));
		::MultiByteToWideChar(CP_ACP, 0, lpcszString, -1, (LPWSTR)pUnicode, unicodeLen);
		std::wstring wString = (wchar_t*)pUnicode;
		delete[] pUnicode;
		return wString;
	}
	std::string WStringToMBytes(const wchar_t* lpwcszWString)
	{
		char* pElementText;
		int iTextLen;
		// wide char to multi char
		iTextLen = ::WideCharToMultiByte(CP_ACP, 0, lpwcszWString, -1, NULL, 0, NULL, NULL);
		pElementText = new char[iTextLen + 1];
		memset((void*)pElementText, 0, (iTextLen + 1) * sizeof(char));
		::WideCharToMultiByte(CP_ACP, 0, lpwcszWString, -1, pElementText, iTextLen, NULL, NULL);
		std::string strReturn(pElementText);
		delete[] pElementText;
		return strReturn;
	}
	std::wstring UTF8ToWString(const char* lpcszString)
	{
		int len = strlen(lpcszString);
		int unicodeLen = ::MultiByteToWideChar(CP_UTF8, 0, lpcszString, -1, NULL, 0);
		wchar_t* pUnicode;
		pUnicode = new wchar_t[unicodeLen + 1];
		memset((void*)pUnicode, 0, (unicodeLen + 1) * sizeof(wchar_t));
		::MultiByteToWideChar(CP_UTF8, 0, lpcszString, -1, (LPWSTR)pUnicode, unicodeLen);
		std::wstring wstrReturn(pUnicode);
		delete[] pUnicode;
		return wstrReturn;
	}
	std::string WStringToUTF8(const wchar_t* lpwcszWString)
	{
		char* pElementText;
		int iTextLen = ::WideCharToMultiByte(CP_UTF8, 0, (LPWSTR)lpwcszWString, -1, NULL, 0, NULL, NULL);
		pElementText = new char[iTextLen + 1];
		memset((void*)pElementText, 0, (iTextLen + 1) * sizeof(char));
		::WideCharToMultiByte(CP_UTF8, 0, (LPWSTR)lpwcszWString, -1, pElementText, iTextLen, NULL, NULL);
		std::string strReturn(pElementText);
		delete[] pElementText;
		return strReturn;
	}
	std::string readString(const DWORD64 dwPtr)
	{
		std::string ret = WStringToMBytes(UTF8ToWString((const char*)dwPtr).c_str());
		return ret;
	}
}


namespace d3d
{
	


	void DrawString(float fontSize, const geo::vector2& vec, uint32_t color, bool bCenter, bool stroke, const char* pText, ...)
	{
		float a = (color >> 24) & 0xff;
		float r = (color >> 16) & 0xff;
		float g = (color >> 8) & 0xff;
		float b = (color) & 0xff;

		va_list va_alist;
		char buf[1024] = { 0 };
		va_start(va_alist, pText);
		_vsnprintf_s(buf, sizeof(buf), pText, va_alist);
		va_end(va_alist);
		std::string text = Hexto::WStringToUTF8(Hexto::MBytesToWString(buf).c_str());
		geo::vector2 drawPos = vec;
		if (bCenter)
		{
			ImVec2 textSize = ImGui::CalcTextSize(text.c_str());
			drawPos.x = vec.x - textSize.x / 2;
			drawPos.y = vec.y - textSize.y;
		}
		if (stroke)
		{
			ImGui::GetOverlayDrawList()->AddText(ImGui::GetFont(), fontSize, ImVec2(drawPos.x + 1, drawPos.y + 1), ImGui::ColorConvertFloat4ToU32(ImVec4(0, 0, 0, 1)), text.c_str());
			ImGui::GetOverlayDrawList()->AddText(ImGui::GetFont(), fontSize, ImVec2(drawPos.x - 1, drawPos.y - 1), ImGui::ColorConvertFloat4ToU32(ImVec4(0, 0, 0, 1)), text.c_str());
			ImGui::GetOverlayDrawList()->AddText(ImGui::GetFont(), fontSize, ImVec2(drawPos.x + 1, drawPos.y - 1), ImGui::ColorConvertFloat4ToU32(ImVec4(0, 0, 0, 1)), text.c_str());
			ImGui::GetOverlayDrawList()->AddText(ImGui::GetFont(), fontSize, ImVec2(drawPos.x - 1, drawPos.y + 1), ImGui::ColorConvertFloat4ToU32(ImVec4(0, 0, 0, 1)), text.c_str());
		}
		ImGui::GetOverlayDrawList()->AddText(ImGui::GetFont(), fontSize, ImVec2(drawPos.x, drawPos.y), ImGui::GetColorU32({ r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f }), text.c_str());
	}
	void DrawBox(const geo::vector2& vecStart, const geo::vector2& vecEnd, uint32_t color, float rounding, uint32_t roundingCornersFlags, float thickness)
	{
		
		float a = (color >> 24) & 0xFF;
		float r = (color >> 16) & 0xFF;
		float g = (color >> 8) & 0xFF;
		float b = (color) & 0xFF;

		ImGui::GetOverlayDrawList()->AddRect(ImVec2(vecStart.x, vecStart.y), ImVec2(vecStart.x + vecEnd.x, vecStart.y + vecEnd.y), ImGui::GetColorU32({ r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f }), rounding, roundingCornersFlags, thickness);
	}

	void DrawFilledBox(const geo::vector2& vecStart, const geo::vector2& vecEnd, uint32_t color, float rounding, uint32_t roundingCornersFlags)
	{

		float a = (color >> 24) & 0xFF;
		float r = (color >> 16) & 0xFF;
		float g = (color >> 8) & 0xFF;
		float b = (color) & 0xFF;

		ImGui::GetOverlayDrawList()->AddRectFilled(ImVec2(vecStart.x, vecStart.y), ImVec2(vecStart.x + vecEnd.x, vecStart.y + vecEnd.y), ImGui::GetColorU32({ r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f }), rounding, roundingCornersFlags);
	}

	void DrawLine(const geo::vector2& vecStart, const geo::vector2& vecEnd, uint32_t color, float thickness = 1.5f)
	{
		float a = (color >> 24) & 0xff;
		float r = (color >> 16) & 0xff;
		float g = (color >> 8) & 0xff;
		float b = (color) & 0xff;

		ImGui::GetOverlayDrawList()->AddLine(ImVec2(vecStart.x, vecStart.y), ImVec2(vecEnd.x, vecEnd.y), ImGui::GetColorU32({ r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f }), thickness);

	}

	void RenderCircleFilled(const geo::vector2& vecCenter, float radius, uint32_t color, uint32_t segments)
	{
		
		float a = (color >> 24) & 0xff;
		float r = (color >> 16) & 0xff;
		float g = (color >> 8) & 0xff;
		float b = (color) & 0xff;

		ImGui::GetOverlayDrawList()->AddCircleFilled(ImVec2(vecCenter.x, vecCenter.y), radius, ImGui::GetColorU32({ r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f }));
	}


	void DrawCircle(const geo::vector2& vecCenter, float radius, uint32_t color, int num_seg, float thickness)
	{
		float a = (color >> 24) & 0xff;
		float r = (color >> 16) & 0xff;
		float g = (color >> 8) & 0xff;
		float b = (color) & 0xff;

		ImGui::GetOverlayDrawList()->AddCircle(ImVec2(vecCenter.x, vecCenter.y), radius, ImGui::GetColorU32({ r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f }), num_seg, thickness);
	}


}

namespace unity
{

	std::list<int> upper_part = { Bones::HumanNeck, Bones::HumanHead };
	std::list<int> right_arm = { Bones::HumanNeck, Bones::HumanRUpperarm, Bones::HumanRForearm1, Bones::HumanRPalm };
	std::list<int> left_arm = { Bones::HumanNeck, Bones::HumanLUpperarm, Bones::HumanLForearm1, Bones::HumanLPalm };
	std::list<int> spine = { Bones::HumanNeck, Bones::HumanSpine1, Bones::HumanSpine2, Bones::HumanPelvis };

	std::list<int> lower_right = { Bones::HumanPelvis, Bones::HumanRThigh1, Bones::HumanRCalf, Bones::HumanRFoot };
	std::list<int> lower_left = { Bones::HumanPelvis, Bones::HumanLThigh1, Bones::HumanLCalf, Bones::HumanLFoot };

	std::list<std::list<int>> skeleton = { upper_part, right_arm, left_arm, spine, lower_right, lower_left };



	bool world_to_screen(geo::vec3_t& world, geo::vec2_t& screen)
	{
		const auto matrix = G::camera->ViewMatrix.transpose();

		const geo::vec3_t translation = { matrix[3][0], matrix[3][1], matrix[3][2] };
		const geo::vec3_t up = { matrix[1][0], matrix[1][1], matrix[1][2] };
		const geo::vec3_t right = { matrix[0][0], matrix[0][1], matrix[0][2] };

		const auto w = translation.dot_product(world) + matrix[3][3];

		if (w < 0.1f)
			return false;

		const auto x = right.dot_product(world) + matrix[0][3];
		const auto y = up.dot_product(world) + matrix[1][3];

		screen.x = (1920 / 2) * (1.f + x / w);
		screen.y = (1080 / 2) * (1.f - y / w);

		return true;
	}



	geo::vec3_t get_position(void* transform)
	{
		if (!transform)
			return {};

		geo::vec3_t position{};

		static const auto get_position_injected = reinterpret_cast<uint64_t(__fastcall*)(void*, geo::vec3_t&)>(G::GameUnityBase + 0xc09a20);
		get_position_injected(transform, position);

		return position;
	}

	void GetBoneByID(Player* m_player, int id, geo::vec3_t& out)
	{
		PlayerBody* m_body = m_player->m_pPlayerBody;
		m_pSkeletonRootJoin* m_skeleton = m_body->m_pSkeletonRootJoin;
		BoneEnumerator* Bones = m_skeleton->m_pBoneEnumerator;
		TransformArray* m_array = Bones->m_pTransformArray;

		const auto player = *reinterpret_cast<unity_transform**>(std::uintptr_t(m_array) + (0x20 + (id * 8)));

		out = get_position(player->transform);

	}

	bool IsPlayer(Player* m_player)
	{
		if(m_player->m_pPlayerProfile->m_PlayerInfo->CreationDate != 0)
			return true;

		return false;

	}

	//this is ugly asf but fuck it we internal bois
	void DrawSkeleton(Player* player)
	{
		geo::vector3 neckpos;
		GetBoneByID(player, Bones::HumanNeck, neckpos);
		geo::vector3 pelvispos;
		GetBoneByID(player, Bones::HumanPelvis, pelvispos);
		geo::vector3 previous(0, 0, 0);
		geo::vector3 current;
		geo::vector2 p1, c1;
		for (auto a : skeleton)
		{
			previous = geo::vector3(0, 0, 0);
			for (int bone : a)
			{
				geo::vector3 bones;
				GetBoneByID(player, bone, bones);
				current = bone == Bones::HumanNeck ? neckpos : (bone == Bones::HumanPelvis ? pelvispos : bones);
				if (previous.x == 0.f)
				{
					previous = current;
					continue;
				}
				world_to_screen(previous, p1);
				world_to_screen(current, c1);
				d3d::DrawLine(p1, c1, WHITE, 1.0f);

				previous = current;
			}
		}
	}

	void obtain_objects(void* game_object_manager, bool activeobjects, const char* objectName)
	{
		char String[256];
		int n;

		auto last_object = *reinterpret_cast<unk1**>(game_object_manager);
		auto first_object = *reinterpret_cast<unk1**>(std::uintptr_t(game_object_manager) + 0x8);

		if (activeobjects) //lol im such a retard
		{
			last_object = *reinterpret_cast<unk1**>(std::uintptr_t(game_object_manager) + 0x10);
			first_object = *reinterpret_cast<unk1**>(std::uintptr_t(game_object_manager) + 0x18);
		}


		for (auto object = first_object; object != last_object; object = object->next)
		{
			n = std::sprintf(String, E("%s"), *reinterpret_cast<char**>(std::uintptr_t(object->object) + 0x60));
			if (strcmp(String, objectName) == 0)
			{
				if (activeobjects)//gameworld
				{
					G::gameworld = (reinterpret_cast<LOCALGAMEWORLD*>(object->object->pObjectClass->unk->localgameworld));
					//std::printf("[%s] - Found!\n", String);
					break;
				}
				else//fps camera
				{
					G::camera = (reinterpret_cast<CameraEntity*>(object->object->pObjectClass->unk));
					//std::printf("[%s] - Found!\n", String);
					break;
				}
			}
		}

		n = std::sprintf(String, E("%s"), *reinterpret_cast<char**>(std::uintptr_t(last_object->object) + 0x60));
		if (strcmp(String, objectName) == 0)
		{
			if (activeobjects)//gameworld
			{
				G::gameworld = (reinterpret_cast<LOCALGAMEWORLD*>(last_object->object->pObjectClass->unk->localgameworld));
				//std::printf("Last Object: [%s] - Found!\n", String);
			}
			else//fps camera
			{
				G::camera = (reinterpret_cast<CameraEntity*>(last_object->object->pObjectClass->unk));
				//std::printf("Last Object: [%s] - Found!\n", String);
			}
		}
	}

	namespace localplayer
	{
		geo::vec3_t get_localplayerpos()
		{
			geo::vector3 origin;
			unity::GetBoneByID(G::LocalPlayer, Bones::HumanBase, origin);

			return origin;
		}

	}
}
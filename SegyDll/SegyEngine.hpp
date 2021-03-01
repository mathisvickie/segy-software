#include "D:\work\SDK\XorStr.hpp"
#include "D:\work\SDK\lazy_importer.hpp"
#include "D:\work\SDK\ntdll.h"
#include <Windows.h>
#include <TlHelp32.h>
#include <math.h>
#include <iostream>
#include <vector>
#pragma comment(lib, "ntdll.lib")

#ifdef RGB
#undef RGB
#endif
#define RGB(r,g,b) ((COLORREF)(((BYTE)(b)|((WORD)((BYTE)(g))<<8))|(((DWORD)(BYTE)(r))<<16)))

namespace Engine {

	class Offsets {
	public:
		static DWORD LocalPlayer;
		static DWORD EntityList;
		static DWORD ViewProjectionMatrix;
		static DWORD Model;
		static DWORD IsDormant;
		static DWORD Team;
		static DWORD Health;
		static DWORD Position;
		static DWORD BoneMatrix;
		static DWORD AimPunch;
		static DWORD IsDefusing;
		static DWORD CrossHairId;
	};

	class Options {
	public:
		static BOOLEAN Trigger;
		static BOOLEAN Vision;
		static BYTE AttackKey;
		static DWORD WindowWidth;
		static DWORD WindowHeight;
	};

	class NT {
	public:
		static HANDLE ProcessHandle;
		static HANDLE OverlayHandle;
		static PBYTE ClientBase;
		static PBYTE BufferBase;

		static DWORD GetPidFromName(LPCSTR szName)
		{
			PROCESSENTRY32 ProcEntry = { NULL };
			ProcEntry.dwSize = sizeof(PROCESSENTRY32);

			HANDLE hSnapshot = LI_FN(CreateToolhelp32Snapshot)(TH32CS_SNAPPROCESS, NULL);

			if (hSnapshot == INVALID_HANDLE_VALUE)
				return NULL;

			if (LI_FN(Process32First)(hSnapshot, &ProcEntry)) {
				do if (!strcmp(ProcEntry.szExeFile, szName)) {

					LI_FN(CloseHandle)(hSnapshot);
					return ProcEntry.th32ProcessID;
				}
				while (LI_FN(Process32Next)(hSnapshot, &ProcEntry));
			}
			LI_FN(CloseHandle)(hSnapshot);

			return NULL;
		};

		static PBYTE GetModuleBase(LPCSTR szModName, DWORD dwPid, OUT OPTIONAL PDWORD pdwModSize)
		{
			HANDLE hSnapshot = LI_FN(CreateToolhelp32Snapshot)(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, dwPid);

			if (hSnapshot == INVALID_HANDLE_VALUE)
				return NULL;

			MODULEENTRY32 ModuleEntry;
			ModuleEntry.dwSize = sizeof(MODULEENTRY32);

			if (LI_FN(Module32First)(hSnapshot, &ModuleEntry)) {
				do if (!strcmp(ModuleEntry.szModule, szModName)) {

					LI_FN(CloseHandle)(hSnapshot);

					if (pdwModSize)
						*pdwModSize = ModuleEntry.modBaseSize;

					return ModuleEntry.modBaseAddr;

				} while (LI_FN(Module32Next)(hSnapshot, &ModuleEntry));
			}
			LI_FN(CloseHandle)(hSnapshot);

			return NULL;
		};

		static PSYSTEM_HANDLE_INFORMATION EnumSystemHandles(VOID) {

			DWORD dwSystemHandleInfoSize = 0x10000;
			PSYSTEM_HANDLE_INFORMATION pSystemHandleInfo = reinterpret_cast<PSYSTEM_HANDLE_INFORMATION>(malloc(dwSystemHandleInfoSize));

			while (LI_FN(NtQuerySystemInformation)(SystemHandleInformation, pSystemHandleInfo, dwSystemHandleInfoSize, nullptr) == STATUS_INFO_LENGTH_MISMATCH)
				pSystemHandleInfo = reinterpret_cast<PSYSTEM_HANDLE_INFORMATION>(realloc(pSystemHandleInfo, dwSystemHandleInfoSize *= 2));

			return pSystemHandleInfo;
		}

		static HANDLE FindProcessHandle(PSYSTEM_HANDLE_INFORMATION pSystemHandles, DWORD dwPid)
		{
			for (DWORD i = NULL; i < pSystemHandles->NumberOfHandles; i++) {

				if (pSystemHandles->Handles[i].UniqueProcessId != LI_FN(GetCurrentProcessId)() || pSystemHandles->Handles[i].GrantedAccess != 0x1478)
					continue;

				HANDLE hHijack = reinterpret_cast<HANDLE>(pSystemHandles->Handles[i].HandleValue);

				PROCESS_BASIC_INFORMATION ProcessInfo;
				ZeroMemory(&ProcessInfo, sizeof(PROCESS_BASIC_INFORMATION));

				if (NT_SUCCESS(LI_FN(NtQueryInformationProcess)(hHijack, ProcessBasicInformation, &ProcessInfo, sizeof(PROCESS_BASIC_INFORMATION), nullptr))
					&& dwPid == reinterpret_cast<DWORD>(ProcessInfo.UniqueProcessId))
					return hHijack;
			}
			return NULL;
		};

		static PCHAR ScanMemory(PCHAR pMemoryStart, DWORD dwSize, LPCSTR szPattern)
		{
			for (PCHAR pCurr = pMemoryStart; pCurr < pMemoryStart + dwSize; pCurr++) {
				BOOL bMatched = TRUE;

				for (LPCSTR szPatternIt = szPattern, pCurrIt = pCurr; *szPatternIt; szPatternIt++, pCurrIt++) {

					if (*szPatternIt != '?' && *pCurrIt != *szPatternIt) {
						bMatched = FALSE;
						break;
					}
				}
				if (bMatched)
					return pCurr;
			}

			return NULL;
		}

		static VOID Delay(DWORD dwMilliseconds) {
			if (dwMilliseconds) LI_FN(Sleep).cached()(dwMilliseconds);
		}

		static BOOLEAN ReadGameMem(LPVOID lpFrom, LPVOID lpTo, DWORD dwSize) {
			return LI_FN(ReadProcessMemory).cached()(Engine::NT::ProcessHandle, lpFrom, lpTo, dwSize, nullptr);
		}

		static BOOLEAN SendKey(BYTE bKey) {
			return LI_FN(WriteProcessMemory).cached()(Engine::NT::OverlayHandle, Engine::NT::BufferBase + 0x3, &bKey, sizeof(BYTE), nullptr);
		}

		static BOOLEAN ShouldExit(VOID) {

			DWORD dwAttributes = LI_FN(GetFileAttributesA).cached()(XorStr("D:\\u"));

			return (dwAttributes != INVALID_FILE_ATTRIBUTES) && (dwAttributes & FILE_ATTRIBUTE_DIRECTORY);
		}
	};

	class Math {
	public:

		class Vec {
		public:
			FLOAT X, Y, Z;

			static FLOAT Distance(Engine::Math::Vec Src, Engine::Math::Vec Dst) {
				return sqrtf((Src.X - Dst.X)*(Src.X - Dst.X) + (Src.Y - Dst.Y)*(Src.Y - Dst.Y) + (Src.Z - Dst.Z)*(Src.Z - Dst.Z));
			}
		};

		static FLOAT VPMatrix[16];

		static BOOLEAN ReadViewProjectionMatrix(VOID) {
			return Engine::NT::ReadGameMem(Engine::NT::ClientBase + Engine::Offsets::ViewProjectionMatrix, &Engine::Math::VPMatrix, 0x40);
		}

		static BOOLEAN WorldToScreen(Engine::Math::Vec Position, Engine::Math::Vec &Screen) {

			Engine::Math::Vec ClipCoords;
			ClipCoords.X = Position.X*Engine::Math::VPMatrix[0] + Position.Y*Engine::Math::VPMatrix[1] + Position.Z*Engine::Math::VPMatrix[2] + Engine::Math::VPMatrix[3];
			ClipCoords.Y = Position.X*Engine::Math::VPMatrix[4] + Position.Y*Engine::Math::VPMatrix[5] + Position.Z*Engine::Math::VPMatrix[6] + Engine::Math::VPMatrix[7];
			ClipCoords.Z = Position.X*Engine::Math::VPMatrix[12] + Position.Y*Engine::Math::VPMatrix[13] + Position.Z*Engine::Math::VPMatrix[14] + Engine::Math::VPMatrix[15];

			if (ClipCoords.Z < 0.1f)
				return FALSE;

			Engine::Math::Vec NDC;
			NDC.X = ClipCoords.X / ClipCoords.Z;
			NDC.Y = ClipCoords.Y / ClipCoords.Z;

			Screen.X = (Engine::Options::WindowWidth / 2 * NDC.X) + (NDC.X + Engine::Options::WindowWidth / 2);
			Screen.Y = -(Engine::Options::WindowHeight / 2 * NDC.Y) + (NDC.Y + Engine::Options::WindowHeight / 2);
			return TRUE;
		}
	};

	class Graphics {
	public:

		static BOOLEAN ForceRepaint(VOID) {

			BYTE bStatus = TRUE;
			return LI_FN(WriteProcessMemory).cached()(Engine::NT::OverlayHandle, Engine::NT::BufferBase + 0x7, &bStatus, sizeof(BYTE), nullptr);
		}

		static BYTE IsRepainting(VOID) {

			BYTE bStatus;
			LI_FN(ReadProcessMemory).cached()(Engine::NT::OverlayHandle, Engine::NT::BufferBase + 0x7, &bStatus, sizeof(BYTE), nullptr);
			return bStatus;
		}

		static BOOLEAN SetPixel(COLORREF Color, Engine::Math::Vec Position) {

			if (Position.X < 0.f || Position.Y < 0.f || Position.X >= Engine::Options::WindowWidth || Position.Y >= Engine::Options::WindowHeight)
				return TRUE;

			PBYTE pAddr = Engine::NT::BufferBase + (static_cast<DWORD>(Position.Y) * Engine::Options::WindowWidth + static_cast<DWORD>(Position.X)) * 4;

			return LI_FN(WriteProcessMemory).cached()(Engine::NT::OverlayHandle, pAddr, &Color, 0x3, nullptr);
		}

		static VOID DrawLine(COLORREF Color, Engine::Math::Vec Point1, Engine::Math::Vec Point2) {

			Engine::Math::Vec Diff;
			Diff.X = Point2.X - Point1.X;
			Diff.Y = Point2.Y - Point1.Y;

			DWORD Dots = NULL;
			Engine::Math::Vec Step;

			auto lSign = [](FLOAT X)
			{
				if (X < NULL)
					return -1.f;
				else
					return 1.f;
			};

			auto lAbs = [](FLOAT X)
			{
				if (X < NULL)
					return -X;
				else
					return X;
			};

			if (lAbs(Diff.X) > lAbs(Diff.Y)) {
				Dots = static_cast<DWORD>(lAbs(Diff.X));

				Step.X = lSign(Point2.X - Point1.X);
				Step.Y = lAbs(Diff.Y) / lAbs(Diff.X) * lSign(Diff.Y);
			}
			else {
				Dots = static_cast<DWORD>(lAbs(Diff.Y));

				Step.X = lAbs(Diff.X) / lAbs(Diff.Y) * lSign(Diff.X);
				Step.Y = lSign(Point2.Y - Point1.Y);
			}

			for (DWORD _ = NULL; _ <= Dots; ++_) {
				Engine::Graphics::SetPixel(Color, Point1);

				Point1.X += Step.X;
				Point1.Y += Step.Y;
			}
		}
	};

	class Entity {
	public:
		PBYTE Base;
		PBYTE Model;
		BYTE IsDormant;
		DWORD Team;
		DWORD Health;
		Engine::Math::Vec Position;
		PBYTE Bones;
		Engine::Math::Vec AimPunch;
		BYTE IsDefusing;
		DWORD CrossHairId;

		enum class ClassID
		{
			C4 = 34,
			Chicken = 36,
			Player = 40,
			Hostage = 97,
			PlantedC4 = 128,
		};

		Engine::Entity::ClassID ClassId;

		VOID ReadInformation(PBYTE EntBase) {

			this->Base = EntBase;
			this->Model = NULL;
			this->Bones = NULL;

			if (this->Base) {
				PBYTE tmp = this->Base;

				Engine::NT::ReadGameMem(tmp + 0x8, &tmp, sizeof(DWORD));
				Engine::NT::ReadGameMem(tmp + 0x8, &tmp, sizeof(DWORD));
				Engine::NT::ReadGameMem(tmp + 0x1, &tmp, sizeof(DWORD));
				Engine::NT::ReadGameMem(tmp + 0x14, &this->ClassId, sizeof(INT));

				Engine::NT::ReadGameMem(this->Base + Engine::Offsets::Model, &this->Model, sizeof(DWORD));
				Engine::NT::ReadGameMem(this->Base + Engine::Offsets::IsDormant, &this->IsDormant, sizeof(BYTE));
				Engine::NT::ReadGameMem(this->Base + Engine::Offsets::BoneMatrix, &this->Bones, sizeof(DWORD));
				Engine::NT::ReadGameMem(this->Base + Engine::Offsets::IsDefusing, &this->IsDefusing, sizeof(BYTE));
			}
			else {
				Engine::NT::ReadGameMem(Engine::NT::ClientBase + Engine::Offsets::LocalPlayer, &this->Base, sizeof(DWORD));

				Engine::NT::ReadGameMem(this->Base + Engine::Offsets::AimPunch, &this->AimPunch, sizeof(Engine::Math::Vec));
				Engine::NT::ReadGameMem(this->Base + Engine::Offsets::CrossHairId, &this->CrossHairId, sizeof(DWORD));
			}

			Engine::NT::ReadGameMem(this->Base + Engine::Offsets::Team, &this->Team, sizeof(DWORD));
			Engine::NT::ReadGameMem(this->Base + Engine::Offsets::Health, &this->Health, sizeof(DWORD));
			Engine::NT::ReadGameMem(this->Base + Engine::Offsets::Position, &this->Position, sizeof(Engine::Math::Vec));
		}

		VOID ReadClassId(DWORD ID) {

			this->Base = NULL;
			Engine::NT::ReadGameMem(Engine::NT::ClientBase + Engine::Offsets::EntityList + (ID - 1) * 0x10, &this->Base, sizeof(DWORD));

			PBYTE tmp = this->Base;
			Engine::NT::ReadGameMem(tmp + 0x8, &tmp, sizeof(DWORD));
			Engine::NT::ReadGameMem(tmp + 0x8, &tmp, sizeof(DWORD));
			Engine::NT::ReadGameMem(tmp + 0x1, &tmp, sizeof(DWORD));
			Engine::NT::ReadGameMem(tmp + 0x14, &this->ClassId, sizeof(INT));
		}

		Engine::Math::Vec GetBonePosition(DWORD ID) {

			Engine::Math::Vec Position;

			Engine::NT::ReadGameMem(this->Bones + 0x30 * ID + 0x0C, &Position.X, sizeof(FLOAT));
			Engine::NT::ReadGameMem(this->Bones + 0x30 * ID + 0x1C, &Position.Y, sizeof(FLOAT));
			Engine::NT::ReadGameMem(this->Bones + 0x30 * ID + 0x2C, &Position.Z, sizeof(FLOAT));

			return Position;
		}

		COLORREF GetDrawColor(VOID) {
			return RGB(255 - static_cast<FLOAT>(this->Health) * 2.55f, static_cast<FLOAT>(this->Health) * 2.55f, 0);
		}

		VOID DrawBone(COLORREF Color, DWORD Bone1, DWORD Bone2) {

			Engine::Math::Vec ScreenPosition1;
			Engine::Math::Vec ScreenPosition2;

			Engine::Math::Vec BonePosition1 = this->GetBonePosition(Bone1);
			Engine::Math::Vec BonePosition2 = this->GetBonePosition(Bone2);

			if (Engine::Math::WorldToScreen(BonePosition1, ScreenPosition1) && Engine::Math::WorldToScreen(BonePosition2, ScreenPosition2))
				Engine::Graphics::DrawLine(Color, ScreenPosition1, ScreenPosition2);
		}

		BOOLEAN IsModelType(CHAR* Search) {

			CHAR szModel[0x40];
			Engine::NT::ReadGameMem(this->Model + 0x04, &szModel, 0x40);

			for (DWORD i = NULL; i < 0x40; i++) {
				BOOLEAN bFound = TRUE;

				for (DWORD j = NULL; j < strlen(Search); j++)
					if (Search[j] != szModel[i + j]) { bFound = FALSE; break; }

				if (bFound)
					return TRUE;
			}
			return FALSE;
		}

		VOID DrawBox(COLORREF Color, FLOAT BoxBaseSize, FLOAT BoxHeight) {
			Engine::Math::ReadViewProjectionMatrix();

			Engine::Math::Vec VertexPosition;
			Engine::Math::Vec ScreenPoint1;
			Engine::Math::Vec ScreenPoint2;
			Engine::Math::Vec ScreenPoint3;
			Engine::Math::Vec ScreenPoint4;
			Engine::Math::Vec ScreenPoint5;
			Engine::Math::Vec ScreenPoint6;
			Engine::Math::Vec ScreenPoint7;
			Engine::Math::Vec ScreenPoint8;

			//************************************************************** Box Base
			VertexPosition.X = this->Position.X - BoxBaseSize;
			VertexPosition.Y = this->Position.Y - BoxBaseSize;
			VertexPosition.Z = this->Position.Z;
			if (!Engine::Math::WorldToScreen(VertexPosition, ScreenPoint1)) return;
			VertexPosition.X = this->Position.X + BoxBaseSize;
			VertexPosition.Y = this->Position.Y - BoxBaseSize;
			VertexPosition.Z = this->Position.Z;
			if (!Engine::Math::WorldToScreen(VertexPosition, ScreenPoint2)) return;
			Engine::Graphics::DrawLine(Color, ScreenPoint1, ScreenPoint2);

			VertexPosition.X = this->Position.X + BoxBaseSize;
			VertexPosition.Y = this->Position.Y + BoxBaseSize;
			VertexPosition.Z = this->Position.Z;
			if (!Engine::Math::WorldToScreen(VertexPosition, ScreenPoint3)) return;
			Engine::Graphics::DrawLine(Color, ScreenPoint2, ScreenPoint3);

			VertexPosition.X = this->Position.X - BoxBaseSize;
			VertexPosition.Y = this->Position.Y + BoxBaseSize;
			VertexPosition.Z = this->Position.Z;
			if (!Engine::Math::WorldToScreen(VertexPosition, ScreenPoint4)) return;
			Engine::Graphics::DrawLine(Color, ScreenPoint3, ScreenPoint4);

			VertexPosition.X = this->Position.X - BoxBaseSize;
			VertexPosition.Y = this->Position.Y - BoxBaseSize;
			VertexPosition.Z = this->Position.Z;
			if (!Engine::Math::WorldToScreen(VertexPosition, ScreenPoint1)) return;
			Engine::Graphics::DrawLine(Color, ScreenPoint4, ScreenPoint1);

			//************************************************************** Box Top
			VertexPosition.X = this->Position.X - BoxBaseSize;
			VertexPosition.Y = this->Position.Y - BoxBaseSize;
			VertexPosition.Z = this->Position.Z + BoxHeight;
			if (!Engine::Math::WorldToScreen(VertexPosition, ScreenPoint5)) return;
			VertexPosition.X = this->Position.X + BoxBaseSize;
			VertexPosition.Y = this->Position.Y - BoxBaseSize;
			VertexPosition.Z = this->Position.Z + BoxHeight;
			if (!Engine::Math::WorldToScreen(VertexPosition, ScreenPoint6)) return;
			Engine::Graphics::DrawLine(Color, ScreenPoint5, ScreenPoint6);

			VertexPosition.X = this->Position.X + BoxBaseSize;
			VertexPosition.Y = this->Position.Y + BoxBaseSize;
			VertexPosition.Z = this->Position.Z + BoxHeight;
			if (!Engine::Math::WorldToScreen(VertexPosition, ScreenPoint7)) return;
			Engine::Graphics::DrawLine(Color, ScreenPoint6, ScreenPoint7);

			VertexPosition.X = this->Position.X - BoxBaseSize;
			VertexPosition.Y = this->Position.Y + BoxBaseSize;
			VertexPosition.Z = this->Position.Z + BoxHeight;
			if (!Engine::Math::WorldToScreen(VertexPosition, ScreenPoint8)) return;
			Engine::Graphics::DrawLine(Color, ScreenPoint7, ScreenPoint8);

			VertexPosition.X = this->Position.X - BoxBaseSize;
			VertexPosition.Y = this->Position.Y - BoxBaseSize;
			VertexPosition.Z = this->Position.Z + BoxHeight;
			if (!Engine::Math::WorldToScreen(VertexPosition, ScreenPoint5)) return;
			Engine::Graphics::DrawLine(Color, ScreenPoint8, ScreenPoint5);

			//************************************************************** Vertical
			Engine::Graphics::DrawLine(Color, ScreenPoint1, ScreenPoint5);
			Engine::Graphics::DrawLine(Color, ScreenPoint2, ScreenPoint6);
			Engine::Graphics::DrawLine(Color, ScreenPoint3, ScreenPoint7);
			Engine::Graphics::DrawLine(Color, ScreenPoint4, ScreenPoint8);
		}

		VOID DrawSkeleton(COLORREF Color) {
			Engine::Math::ReadViewProjectionMatrix();

			if (this->IsModelType(XorStr("tm_separatist"))) {

				this->DrawBone(Color, 0, 6);
				this->DrawBone(Color, 6, 7);
				this->DrawBone(Color, 7, 8);

				this->DrawBone(Color, 7, 10);
				this->DrawBone(Color, 10, 11);
				this->DrawBone(Color, 11, 12);

				this->DrawBone(Color, 7, 38);
				this->DrawBone(Color, 38, 39);
				this->DrawBone(Color, 39, 40);

				this->DrawBone(Color, 67, 66);
				this->DrawBone(Color, 66, 0);
				this->DrawBone(Color, 0, 73);
				this->DrawBone(Color, 73, 74);

			}
			else if (this->IsModelType(XorStr("tm_leet"))) {

				this->DrawBone(Color, 0, 6);
				this->DrawBone(Color, 6, 7);
				this->DrawBone(Color, 7, 8);

				this->DrawBone(Color, 7, 11);
				this->DrawBone(Color, 11, 12);
				this->DrawBone(Color, 12, 13);

				this->DrawBone(Color, 7, 39);
				this->DrawBone(Color, 39, 40);
				this->DrawBone(Color, 40, 41);

				this->DrawBone(Color, 75, 74);
				this->DrawBone(Color, 74, 0);
				this->DrawBone(Color, 68, 67);
				this->DrawBone(Color, 67, 0);

			}
			else if (this->IsModelType(XorStr("tm_phoenix"))) {

				this->DrawBone(Color, 0, 6);
				this->DrawBone(Color, 6, 7);
				this->DrawBone(Color, 7, 8);

				this->DrawBone(Color, 7, 10);
				this->DrawBone(Color, 10, 11);
				this->DrawBone(Color, 11, 12);

				this->DrawBone(Color, 7, 38);
				this->DrawBone(Color, 38, 39);
				this->DrawBone(Color, 39, 40);

				this->DrawBone(Color, 67, 66);
				this->DrawBone(Color, 66, 0);
				this->DrawBone(Color, 0, 73);
				this->DrawBone(Color, 73, 74);

			}
			else if (this->IsModelType(XorStr("tm_balkan"))) {

				this->DrawBone(Color, 0, 6);
				this->DrawBone(Color, 6, 7);
				this->DrawBone(Color, 7, 8);

				this->DrawBone(Color, 7, 10);
				this->DrawBone(Color, 10, 11);
				this->DrawBone(Color, 11, 12);

				this->DrawBone(Color, 7, 38);
				this->DrawBone(Color, 38, 39);
				this->DrawBone(Color, 39, 40);

				this->DrawBone(Color, 0, 73);
				this->DrawBone(Color, 73, 74);
				this->DrawBone(Color, 0, 66);
				this->DrawBone(Color, 66, 67);

			}
			else if (this->IsModelType(XorStr("tm_professional"))) {

				this->DrawBone(Color, 0, 6);
				this->DrawBone(Color, 6, 7);
				this->DrawBone(Color, 7, 8);

				this->DrawBone(Color, 7, 11);
				this->DrawBone(Color, 11, 12);
				this->DrawBone(Color, 12, 13);

				this->DrawBone(Color, 7, 39);
				this->DrawBone(Color, 39, 40);
				this->DrawBone(Color, 40, 41);

				this->DrawBone(Color, 72, 71);
				this->DrawBone(Color, 71, 0);
				this->DrawBone(Color, 0, 78);
				this->DrawBone(Color, 78, 79);

			}
			else if (this->IsModelType(XorStr("tm_anarchist"))) {

				this->DrawBone(Color, 0, 6);
				this->DrawBone(Color, 6, 7);
				this->DrawBone(Color, 7, 8);

				this->DrawBone(Color, 7, 12);
				this->DrawBone(Color, 12, 13);
				this->DrawBone(Color, 13, 14);

				this->DrawBone(Color, 7, 40);
				this->DrawBone(Color, 40, 41);
				this->DrawBone(Color, 41, 42);

				this->DrawBone(Color, 69, 68);
				this->DrawBone(Color, 68, 0);
				this->DrawBone(Color, 0, 75);
				this->DrawBone(Color, 75, 76);

			}
			else if (this->IsModelType(XorStr("ctm_swat"))) {

				this->DrawBone(Color, 0, 6);
				this->DrawBone(Color, 6, 7);
				this->DrawBone(Color, 7, 8);

				this->DrawBone(Color, 7, 12);
				this->DrawBone(Color, 12, 13);
				this->DrawBone(Color, 13, 14);

				this->DrawBone(Color, 7, 40);
				this->DrawBone(Color, 40, 41);
				this->DrawBone(Color, 41, 42);

				this->DrawBone(Color, 69, 68);
				this->DrawBone(Color, 68, 0);
				this->DrawBone(Color, 0, 75);
				this->DrawBone(Color, 75, 76);

			}
			else if (this->IsModelType(XorStr("ctm_sas"))) {

				this->DrawBone(Color, 0, 6);
				this->DrawBone(Color, 6, 7);
				this->DrawBone(Color, 7, 8);

				this->DrawBone(Color, 7, 11);
				this->DrawBone(Color, 11, 12);
				this->DrawBone(Color, 12, 13);

				this->DrawBone(Color, 7, 40);
				this->DrawBone(Color, 40, 41);
				this->DrawBone(Color, 41, 42);

				this->DrawBone(Color, 83, 82);
				this->DrawBone(Color, 82, 0);
				this->DrawBone(Color, 0, 73);
				this->DrawBone(Color, 73, 74);

			}
			else if (this->IsModelType(XorStr("ctm_idf"))) {

				this->DrawBone(Color, 0, 6);
				this->DrawBone(Color, 6, 7);
				this->DrawBone(Color, 7, 8);

				this->DrawBone(Color, 7, 11);
				this->DrawBone(Color, 11, 12);
				this->DrawBone(Color, 12, 13);

				this->DrawBone(Color, 7, 41);
				this->DrawBone(Color, 41, 42);
				this->DrawBone(Color, 42, 43);

				this->DrawBone(Color, 72, 71);
				this->DrawBone(Color, 71, 0);
				this->DrawBone(Color, 0, 78);
				this->DrawBone(Color, 78, 79);

			}
			else if (this->IsModelType(XorStr("ctm_st6"))) {

				this->DrawBone(Color, 0, 6);
				this->DrawBone(Color, 6, 7);
				this->DrawBone(Color, 7, 8);

				this->DrawBone(Color, 7, 10);
				this->DrawBone(Color, 10, 11);
				this->DrawBone(Color, 11, 12);

				this->DrawBone(Color, 7, 38);
				this->DrawBone(Color, 38, 39);
				this->DrawBone(Color, 39, 40);

				this->DrawBone(Color, 67, 66);
				this->DrawBone(Color, 66, 0);
				this->DrawBone(Color, 0, 73);
				this->DrawBone(Color, 73, 74);

			}
			else if (this->IsModelType(XorStr("ctm_fbi"))) {

				this->DrawBone(Color, 0, 6);
				this->DrawBone(Color, 6, 7);
				this->DrawBone(Color, 7, 8);

				this->DrawBone(Color, 7, 11);
				this->DrawBone(Color, 11, 12);
				this->DrawBone(Color, 12, 13);

				this->DrawBone(Color, 7, 39);
				this->DrawBone(Color, 39, 40);
				this->DrawBone(Color, 40, 41);

				this->DrawBone(Color, 0, 81);
				this->DrawBone(Color, 81, 82);
				this->DrawBone(Color, 0, 72);
				this->DrawBone(Color, 72, 73);

			}
			else if (this->IsModelType(XorStr("ctm_gsg9"))) {

				this->DrawBone(Color, 0, 6);
				this->DrawBone(Color, 6, 7);
				this->DrawBone(Color, 7, 8);

				this->DrawBone(Color, 7, 10);
				this->DrawBone(Color, 10, 11);
				this->DrawBone(Color, 11, 12);

				this->DrawBone(Color, 7, 38);
				this->DrawBone(Color, 38, 39);
				this->DrawBone(Color, 39, 40);

				this->DrawBone(Color, 67, 66);
				this->DrawBone(Color, 66, 0);
				this->DrawBone(Color, 0, 73);
				this->DrawBone(Color, 73, 74);

			}
			else if (this->IsModelType(XorStr("ctm_gign"))) {

				this->DrawBone(Color, 0, 6);
				this->DrawBone(Color, 6, 7);
				this->DrawBone(Color, 7, 8);

				this->DrawBone(Color, 7, 11);
				this->DrawBone(Color, 11, 12);
				this->DrawBone(Color, 12, 13);

				this->DrawBone(Color, 7, 40);
				this->DrawBone(Color, 40, 41);
				this->DrawBone(Color, 41, 42);

				this->DrawBone(Color, 71, 70);
				this->DrawBone(Color, 70, 0);
				this->DrawBone(Color, 0, 77);
				this->DrawBone(Color, 77, 79);
			}
		}
	};

	class EntityList {
	public:
		static std::vector<PBYTE> Vector;

		static VOID ReadEntityList(VOID) {

			Engine::Entity CurrEnt;
			std::vector<PBYTE> EntList;

			for (DWORD i = NULL; i < 2048; i++) {

				CurrEnt.ReadClassId(i);

				if (CurrEnt.ClassId != Engine::Entity::ClassID::C4 && CurrEnt.ClassId != Engine::Entity::ClassID::PlantedC4 &&
					CurrEnt.ClassId != Engine::Entity::ClassID::Player && CurrEnt.ClassId != Engine::Entity::ClassID::Hostage)
					continue;

				EntList.push_back(CurrEnt.Base);

				if (EntList.size() > 128)
					break;
			}
			Engine::EntityList::Vector = EntList;
		}
	};
};

DWORD Engine::Offsets::LocalPlayer = NULL;
DWORD Engine::Offsets::EntityList = NULL;
DWORD Engine::Offsets::ViewProjectionMatrix = NULL;
DWORD Engine::Offsets::Model = 0x6C;
DWORD Engine::Offsets::IsDormant = 0xED;
DWORD Engine::Offsets::Team = 0xF4;
DWORD Engine::Offsets::Health = 0x100;
DWORD Engine::Offsets::Position = 0x138;
DWORD Engine::Offsets::BoneMatrix = 0x26A8;
DWORD Engine::Offsets::AimPunch = 0x302C;
DWORD Engine::Offsets::IsDefusing = 0x3930;
DWORD Engine::Offsets::CrossHairId = 0xB3E4;

BOOLEAN Engine::Options::Trigger = TRUE;
BOOLEAN Engine::Options::Vision = TRUE;
BYTE Engine::Options::AttackKey = 0x32;
DWORD Engine::Options::WindowWidth = 1920;
DWORD Engine::Options::WindowHeight = 1080;

HANDLE Engine::NT::ProcessHandle = NULL;
HANDLE Engine::NT::OverlayHandle = NULL;
PBYTE Engine::NT::ClientBase = NULL;
PBYTE Engine::NT::BufferBase = NULL;

FLOAT Engine::Math::VPMatrix[0x10];
std::vector<PBYTE> Engine::EntityList::Vector;

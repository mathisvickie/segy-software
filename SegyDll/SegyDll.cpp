#include "SegyEngine.hpp"

DWORD WINAPI EntityListThread(LPVOID lpParam) {

	while (!Engine::NT::ShouldExit()) {
		Engine::NT::Delay(10);
		Engine::EntityList::ReadEntityList();
	}
	return NULL;
}

BOOL InitHax(VOID) {

	while (Engine::NT::ShouldExit())
		Engine::NT::Delay(1000);

	PSYSTEM_HANDLE_INFORMATION pSystemHandleInfo = Engine::NT::EnumSystemHandles();

	DWORD dwPid, dwClientSize = NULL, dwGameAddr = NULL;

	if (!(dwPid = Engine::NT::GetPidFromName(XorStr("explorer.exe"))) ||
		!(Engine::NT::OverlayHandle = Engine::NT::FindProcessHandle(pSystemHandleInfo, dwPid)))
		return FALSE;

	if (!LI_FN(ReadProcessMemory)(Engine::NT::OverlayHandle, Engine::NT::GetModuleBase(XorStr("lang-1029.dll"), dwPid, NULL) + 0x6680, &Engine::NT::BufferBase, sizeof(LPCVOID), nullptr))
		return FALSE;

	if (!(dwPid = Engine::NT::GetPidFromName(XorStr("csgo.exe"))) ||
		!(Engine::NT::ProcessHandle = Engine::NT::FindProcessHandle(pSystemHandleInfo, dwPid)) ||
		!(Engine::NT::ClientBase = Engine::NT::GetModuleBase(XorStr("client.dll"), dwPid, &dwClientSize)) || !dwClientSize)
		return FALSE;

	free(pSystemHandleInfo);

	PCHAR pClientCopyBase = static_cast<PCHAR>(LI_FN(VirtualAlloc)(nullptr, dwClientSize, MEM_COMMIT, PAGE_READWRITE));

	if (!pClientCopyBase || !LI_FN(ReadProcessMemory)(Engine::NT::ProcessHandle, Engine::NT::ClientBase, pClientCopyBase, dwClientSize, nullptr))
		return FALSE;

	if (!LI_FN(ReadProcessMemory)(Engine::NT::ProcessHandle, Engine::NT::ScanMemory(pClientCopyBase, dwClientSize, XorStr("\xBB????\x83\xFF\x01\x0F\x8C????\x3B\xF8")) - pClientCopyBase + Engine::NT::ClientBase + 1, &dwGameAddr, sizeof(DWORD), nullptr))
		return FALSE;

	Engine::Offsets::EntityList = dwGameAddr - reinterpret_cast<DWORD>(Engine::NT::ClientBase);

	if (!LI_FN(ReadProcessMemory)(Engine::NT::ProcessHandle, Engine::NT::ScanMemory(pClientCopyBase, dwClientSize, XorStr("\x8D\x34\x85????\x89\x15????\x8B\x41\x08\x8B\x48\x04\x83\xF9\xFF")) - pClientCopyBase + Engine::NT::ClientBase + 3, &dwGameAddr, sizeof(DWORD), nullptr))
		return FALSE;
	
	Engine::Offsets::LocalPlayer = 4 + dwGameAddr - reinterpret_cast<DWORD>(Engine::NT::ClientBase);

	if (!LI_FN(ReadProcessMemory)(Engine::NT::ProcessHandle, Engine::NT::ScanMemory(pClientCopyBase, dwClientSize, XorStr("\x0F\x10\x05????\x8D\x85????\xB9")) - pClientCopyBase + Engine::NT::ClientBase + 3, &dwGameAddr, sizeof(DWORD), nullptr))
		return FALSE;
	
	Engine::Offsets::ViewProjectionMatrix = 176 + dwGameAddr - reinterpret_cast<DWORD>(Engine::NT::ClientBase);

	LI_FN(VirtualFree)(pClientCopyBase, NULL, MEM_RELEASE);

	return LI_FN(CloseHandle)(LI_FN(CreateThread)(nullptr, NULL, EntityListThread, nullptr, NULL, nullptr));
}

VOID DoHax(VOID) {

	DWORD Anti1Tap = NULL;

	while (!Engine::NT::ShouldExit()) {

		Engine::Entity Local;
		Engine::Entity CurrEnt;

		Local.ReadInformation(NULL);
		CurrEnt.ReadClassId(Local.CrossHairId);
		CurrEnt.ReadInformation(CurrEnt.Base);

		DWORD AimPunch = static_cast<DWORD>(100.f * sqrtf(Local.AimPunch.X*Local.AimPunch.X + Local.AimPunch.Y*Local.AimPunch.Y)) + rand() % 13 + 20;

		if (AimPunch<69 && CurrEnt.Team != Local.Team && CurrEnt.Health > NULL && Engine::Options::Trigger &&
			CurrEnt.ClassId == Engine::Entity::ClassID::Player && !Anti1Tap) {

			Engine::NT::SendKey(Engine::Options::AttackKey);
			Anti1Tap = 0; //140
		}
		else if (Anti1Tap) {
			Anti1Tap--;

			Engine::NT::SendKey(Engine::Options::AttackKey);
		}

		if (!Engine::Graphics::IsRepainting()) {
			if (Engine::Options::Vision) {

				for (DWORD i = 0; i < Engine::EntityList::Vector.size(); i++) {

					CurrEnt.ReadInformation(Engine::EntityList::Vector[i]);

					switch (CurrEnt.ClassId) {
					case Engine::Entity::ClassID::Player:

						if (CurrEnt.Team != Local.Team && CurrEnt.Health > NULL && !CurrEnt.IsDormant) {

							CurrEnt.DrawSkeleton(CurrEnt.GetDrawColor());

							if (CurrEnt.IsDefusing)
								CurrEnt.DrawBox(RGB(0, 0, 255), 20.f, 80.f);
						}

						break;
					case Engine::Entity::ClassID::Hostage:

						if (!CurrEnt.IsDormant && 20.f < Engine::Math::Vec::Distance(Local.Position, CurrEnt.Position))
							CurrEnt.DrawBox(RGB(255, 0, 255), 15.f, 50.f);

						break;
					case Engine::Entity::ClassID::C4:
					case Engine::Entity::ClassID::PlantedC4:

						if (!CurrEnt.IsDormant && 20.f < Engine::Math::Vec::Distance(Local.Position, CurrEnt.Position))
							CurrEnt.DrawBox(RGB(0, 255, 255), 8.f, 5.f);

						break;
					}
				}
			}

			FLOAT CrossHairSize = 30.f;
			Engine::Math::Vec XHairPosition0;
			Engine::Math::Vec XHairPosition1;
			Engine::Math::Vec XHairPosition2;
			Engine::Math::Vec XHairPosition3;
			Engine::Math::Vec XHairPosition4;

			XHairPosition0.X = Engine::Options::WindowWidth / 2.f - 5.f * (Local.AimPunch.Y * 2.f);
			XHairPosition0.Y = Engine::Options::WindowHeight / 2.f + 5.f * (Local.AimPunch.X * 2.f);

			XHairPosition1.X = XHairPosition0.X - (CrossHairSize - 1.f) / 2.f;
			XHairPosition1.Y = XHairPosition0.Y;
			XHairPosition2.X = XHairPosition0.X - (CrossHairSize - 1.f) / 2.f + CrossHairSize;
			XHairPosition2.Y = XHairPosition0.Y;
			Engine::Graphics::DrawLine(RGB(0, 255, 94), XHairPosition1, XHairPosition2);

			XHairPosition3.X = XHairPosition0.X;
			XHairPosition3.Y = XHairPosition0.Y - (CrossHairSize - 1.f) / 2.f;
			XHairPosition4.X = XHairPosition0.X;
			XHairPosition4.Y = XHairPosition0.Y - (CrossHairSize - 1.f) / 2.f + CrossHairSize;
			Engine::Graphics::DrawLine(RGB(0, 255, 94), XHairPosition3, XHairPosition4);

			Engine::Graphics::ForceRepaint();
		}
		Engine::NT::Delay(1);
	}
}
#include "stdafx.h"
#include "D:\work\SDK\XorStr.hpp"
#include "D:\work\SDK\lazy_importer.hpp"
#include <Windows.h>

#include <cstdio>
#pragma warning(disable:4996)

BOOL InitHax(VOID);
VOID DoHax(VOID);

DWORD WINAPI MainThread(LPVOID hModule) {

	//freopen("D:\\log.txt", "w", stdout);

	if (InitHax())
		DoHax();

	//fclose(stdout);

	LI_FN(Sleep)(1000);
	LI_FN(FreeLibraryAndExitThread)(reinterpret_cast<HMODULE>(hModule), NULL);

	return NULL;
}

BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
	)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:

		LI_FN(DisableThreadLibraryCalls)(hModule);
		LI_FN(CloseHandle)(LI_FN(CreateThread)(nullptr, NULL, MainThread, hModule, NULL, nullptr));

	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}

	return TRUE;
}

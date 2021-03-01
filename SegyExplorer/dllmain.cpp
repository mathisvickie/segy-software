#include "stdafx.h"
#include "D:\work\SDK\XorStr.hpp"
#include "D:\work\SDK\lazy_importer.hpp"
#include <Windows.h>

#define WINDOW_NAME (XorStr("EdgeUiInputWndClass_2"))
#define WINDOW_WIDTH 1920
#define WINDOW_HEIGHT 1080
#define SleepRandom() (LI_FN(Sleep)((rand() % 30) + 30))

struct pixel {
	BYTE b, g, r, a;
};

HWND g_hWnd;
__declspec(dllexport) pixel* g_pPixels;

DWORD WINAPI InputThread(LPVOID lpParam) {

	while (TRUE) {
		LI_FN(Sleep).cached()(1);
		
		if (LI_FN(GetAsyncKeyState).cached()(0x58))
			g_pPixels[0].a = NULL;

		else if (g_pPixels[0].a) {
			INPUT Input;
			Input.type = INPUT_KEYBOARD;
			Input.ki.time = NULL;
			Input.ki.wVk = NULL;
			Input.ki.dwExtraInfo = NULL;
			Input.ki.wScan = g_pPixels[0].a;
			Input.ki.dwFlags = KEYEVENTF_SCANCODE;

			LI_FN(SendInput).cached()(1, &Input, sizeof(INPUT));
			SleepRandom();

			Input.ki.dwFlags |= KEYEVENTF_KEYUP;
			LI_FN(SendInput)(1, &Input, sizeof(INPUT));
			SleepRandom();
			g_pPixels[0].a = NULL;
		}
	}
}

DWORD WINAPI WindowThread(LPVOID hBmp) {
	
	LI_FN(Sleep)(100);
	LI_FN(ShowWindow)(g_hWnd, SW_SHOW);
	
	HDC hdc = LI_FN(GetDC)(g_hWnd);
	HDC BufferHdc = LI_FN(CreateCompatibleDC)(hdc);
	LI_FN(SelectObject)(BufferHdc, hBmp);
	
	LI_FN(CloseHandle)(LI_FN(CreateThread)(nullptr, NULL, InputThread, nullptr, NULL, nullptr));

	g_pPixels[0].r = 255; g_pPixels[0].g = 255; g_pPixels[0].b = 255;

	while (TRUE) {
		
		LI_FN(BitBlt).cached()(hdc, NULL, NULL, WINDOW_WIDTH, WINDOW_HEIGHT, BufferHdc, NULL, NULL, SRCCOPY);
		LI_FN(Sleep).cached()(8);
		ZeroMemory(&g_pPixels[1].b, WINDOW_HEIGHT * WINDOW_WIDTH * sizeof(pixel) - sizeof(pixel));
		
		while (!g_pPixels[1].a)
			LI_FN(Sleep).cached()(1);
	}
}

typedef LRESULT(WINAPI *fnDefWindowProcA)(HWND, UINT, WPARAM, LPARAM);

LRESULT CALLBACK WindowCallback(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {

	if (msg == WM_CREATE) {

		BITMAPINFO BmpInfo;
		BmpInfo.bmiHeader.biSize = sizeof(BITMAPINFO);
		BmpInfo.bmiHeader.biWidth = WINDOW_WIDTH;
		BmpInfo.bmiHeader.biHeight = -WINDOW_HEIGHT;
		BmpInfo.bmiHeader.biPlanes = 1;
		BmpInfo.bmiHeader.biBitCount = 32;
		BmpInfo.bmiHeader.biCompression = BI_RGB;
		BmpInfo.bmiHeader.biSizeImage = NULL;
		BmpInfo.bmiHeader.biXPelsPerMeter = NULL;
		BmpInfo.bmiHeader.biYPelsPerMeter = NULL;
		BmpInfo.bmiHeader.biClrUsed = NULL;
		BmpInfo.bmiHeader.biClrImportant = NULL;
		BmpInfo.bmiColors[0].rgbBlue = NULL;
		BmpInfo.bmiColors[0].rgbGreen = NULL;
		BmpInfo.bmiColors[0].rgbRed = NULL;
		BmpInfo.bmiColors[0].rgbReserved = NULL;
		
		HDC hdc = LI_FN(GetDC)(hWnd);
		HBITMAP hBmp = LI_FN(CreateDIBSection)(hdc, &BmpInfo, DIB_RGB_COLORS, reinterpret_cast<void**>(&g_pPixels), nullptr, NULL);
		LI_FN(DeleteDC)(hdc);
		
		LI_FN(CloseHandle)(LI_FN(CreateThread)(nullptr, NULL, WindowThread, hBmp, NULL, nullptr));
	}

	static fnDefWindowProcA oDefWindowProcA = reinterpret_cast<fnDefWindowProcA>(LI_FN(GetProcAddress)(LI_FN(GetModuleHandleA)(XorStr("ntdll.dll")), XorStr("NtdllDefWindowProc_A")));

	return oDefWindowProcA(hWnd, msg, wParam, lParam);;
}

DWORD WINAPI MainThread(LPVOID hModule) {

	LI_FN(LoadLibraryA)(XorStr("USER32.dll"));
	LI_FN(LoadLibraryA)(XorStr("GDI32.dll"));
	
	WNDCLASSEXA WindowClass;
	WindowClass.cbClsExtra = NULL;
	WindowClass.cbWndExtra = NULL;
	WindowClass.cbSize = sizeof(WNDCLASSEXA);
	WindowClass.hbrBackground = LI_FN(CreateSolidBrush)(NULL);
	WindowClass.hCursor = LI_FN(LoadCursorA)(nullptr, IDC_ARROW);
	WindowClass.hIcon = LI_FN(LoadIconA)(nullptr, IDI_APPLICATION);
	WindowClass.hIconSm = LI_FN(LoadIconA)(nullptr, IDI_APPLICATION);
	WindowClass.hInstance = NULL;
	WindowClass.lpfnWndProc = WindowCallback;
	WindowClass.lpszClassName = WINDOW_NAME;
	WindowClass.lpszMenuName = WINDOW_NAME;
	WindowClass.style = CS_VREDRAW | CS_HREDRAW;
	LI_FN(RegisterClassExA)(&WindowClass);

	g_hWnd = LI_FN(CreateWindowExA)(WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_TOOLWINDOW, WINDOW_NAME, WINDOW_NAME, WS_VISIBLE | WS_POPUP, NULL, NULL, WINDOW_WIDTH, WINDOW_HEIGHT, nullptr, nullptr, nullptr, nullptr);
	
	LI_FN(SetLayeredWindowAttributes)(g_hWnd, NULL, NULL, LWA_COLORKEY);
	LI_FN(ShowWindow)(g_hWnd, SW_SHOW);
	LI_FN(UpdateWindow)(g_hWnd);
	
	MSG msg;
	while (LI_FN(GetMessageA).cached()(&msg, nullptr, NULL, NULL) > NULL) {
		LI_FN(TranslateMessage).cached()(&msg);
		LI_FN(DispatchMessageA).cached()(&msg);
	}
	return NULL;
}

BOOL APIENTRY DllMain( HMODULE hModule,
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

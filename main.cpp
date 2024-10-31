#include <windows.h>
#include <gdiplus.h>
#include <string>
#include <fstream>
#pragma comment(lib, "gdiplus.lib")

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam);

HHOOK keyboardHook;
HHOOK mouseHook;
HWND g_hwnd = NULL;

char key1, key2, key3;
bool isKey1 = false;
bool isKey2 = false;
bool isKey3 = false;

bool readPasswordKeys() {
	std::ifstream file("password");
	if (!file.is_open()) return false;
	file >> key1 >> key2 >> key3;
	file.close();
	return true;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	if (!readPasswordKeys()) {
		MessageBoxA(NULL, "Failed to read password file", "Error", MB_OK);
		return 1;
	}
	
	// 初始化GDI+
	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR gdiplusToken;
	Gdiplus::Status status = Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
	if (status != Gdiplus::Ok) {
		MessageBoxA(NULL, "Failed to initialize GDI+", "Error", MB_OK);
		return 1;
	}
	
	// 检查图片是否存在
	Gdiplus::Image testImage(L"1.png");
	if (testImage.GetLastStatus() != Gdiplus::Ok) {
		MessageBoxA(NULL, "Failed to load image 1.png", "Error", MB_OK);
		Gdiplus::GdiplusShutdown(gdiplusToken);
		return 1;
	}
	
	ShowCursor(FALSE);
	
	keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, NULL, 0);
	mouseHook = SetWindowsHookEx(WH_MOUSE_LL, LowLevelMouseProc, NULL, 0);
	
	WNDCLASSA wc = {0};
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = "FullscreenWindow";
	wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	
	if (!RegisterClassA(&wc)) {
		MessageBoxA(NULL, "Window Registration Failed", "Error", MB_OK);
		return 1;
	}
	
	int screenWidth = GetSystemMetrics(SM_CXSCREEN);
	int screenHeight = GetSystemMetrics(SM_CYSCREEN);
	
	g_hwnd = CreateWindowExA(
		WS_EX_TOPMOST,
		"FullscreenWindow",
		"Fullscreen",
		WS_POPUP | WS_VISIBLE,
		0, 0, screenWidth, screenHeight,
		NULL, NULL, hInstance, NULL
		);
	
	if (!g_hwnd) {
		MessageBoxA(NULL, "Window Creation Failed", "Error", MB_OK);
		return 1;
	}
	
	// 强制窗口置顶
	SetWindowPos(g_hwnd, HWND_TOPMOST, 0, 0, screenWidth, screenHeight, SWP_SHOWWINDOW);
	UpdateWindow(g_hwnd);
	
	RECT clipRect;
	GetWindowRect(g_hwnd, &clipRect);
	ClipCursor(&clipRect);
	
	MSG msg = {0};
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	
	ClipCursor(NULL);
	ShowCursor(TRUE);
	UnhookWindowsHookEx(keyboardHook);
	UnhookWindowsHookEx(mouseHook);
	Gdiplus::GdiplusShutdown(gdiplusToken);
	return msg.wParam;
}

LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
	if (nCode == HC_ACTION) {
		switch (wParam) {
		case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN:
		case WM_MBUTTONDOWN:
		case WM_MOUSEWHEEL:
		case WM_MOUSEMOVE:
		case WM_LBUTTONUP:
		case WM_RBUTTONUP:
		case WM_MBUTTONUP:
			return 1;
		}
	}
	return CallNextHookEx(NULL, nCode, wParam, lParam);
}

LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
	if (nCode == HC_ACTION) {
		KBDLLHOOKSTRUCT* pKbStruct = (KBDLLHOOKSTRUCT*)lParam;
		
		if (wParam == WM_KEYDOWN) {
			if (pKbStruct->vkCode == key1) isKey1 = true;
			if (pKbStruct->vkCode == key2) isKey2 = true;
			if (pKbStruct->vkCode == key3) isKey3 = true;
			
			if (isKey1 && isKey2 && isKey3) {
				PostQuitMessage(0);
				return 0;
			}
			
			if (pKbStruct->vkCode != key1 && 
				pKbStruct->vkCode != key2 && 
				pKbStruct->vkCode != key3) {
				return 1;
			}
		}
		else if (wParam == WM_KEYUP) {
			if (pKbStruct->vkCode == key1) isKey1 = false;
			if (pKbStruct->vkCode == key2) isKey2 = false;
			if (pKbStruct->vkCode == key3) isKey3 = false;
		}
		
		if ((pKbStruct->flags & LLKHF_ALTDOWN) || 
			(GetAsyncKeyState(VK_CONTROL) & 0x8000) ||
			(GetAsyncKeyState(VK_LWIN) & 0x8000) ||
			(GetAsyncKeyState(VK_RWIN) & 0x8000)) {
			return 1;
		}
	}
	return CallNextHookEx(NULL, nCode, wParam, lParam);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	static Gdiplus::Image* image = NULL;
	
	switch (uMsg) {
	case WM_CREATE:
		image = new Gdiplus::Image(L"1.png");
		return 0;
		
		case WM_PAINT: {
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hwnd, &ps);
			
			if (image && image->GetLastStatus() == Gdiplus::Ok) {
				Gdiplus::Graphics graphics(hdc);
				RECT rect;
				GetClientRect(hwnd, &rect);
				graphics.DrawImage(image, 0, 0, rect.right, rect.bottom);
			}
			
			EndPaint(hwnd, &ps);
			return 0;
		}
		
	case WM_DESTROY:
		if (image) {
			delete image;
		}
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}


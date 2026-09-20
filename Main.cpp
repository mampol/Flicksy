#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <tchar.h>
#include <algorithm>
#include <string>
#include "qrcodegen.hpp"
#include "CSimpleHttpServer.h"
#include "CAppColorTheme.h"
#include "CImgListPng.h"
#include "resource.h"

#pragma comment(lib, "comctl32.lib")

#define APP_CLASS			_T("Flicksy")
#define CSIMPLEHTTPSERVER	_T("CSimpleHttpServer")
#define CAPPCOLORTHEME		_T("CAppColorTheme")

int gnPort = 10000;

static Gdiplus::GdiplusStartupInput ggdiplusStartupInput;
static ULONG_PTR ggdiplusToken = 0;

std::wstring Utf8ToUtf16(const std::string& src);
std::wstring SjisToUtf16(const std::string& s);
std::string Utf16ToUtf8(const std::wstring& s);
std::string SjisToUtf8(const std::string& s);

void SendUnicodeChar(wchar_t ch);
void SendUnicodeText(const std::wstring& text);
void SendClipboardText(const std::wstring& text);
bool IsExtendedKey(WORD vk);
void SendKey(WORD vk);

std::string GetLocalIPv4();
void DrawQrCodeBox(HDC hdc, const std::string& utf8, int x, int y, int size);
void DrawTextLine(HDC hdc, int x, int y, std::wstring& strTextLine);

INT_PTR PreCreateWindow(HWND hWnd, LPTSTR lpsCmdLine, int nCmdShow);
LRESULT CALLBACK MainWndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp);
LRESULT OnCreateWindow(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp);
LRESULT OnPaint(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp);
LRESULT OnCommand(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp);
LRESULT OnHttpPopMessage(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp);
LRESULT OnHttpPopKey(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp);
LRESULT OnHttpState(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp);
LRESULT OnClose(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp);
LRESULT OnDestroy(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp);


std::wstring Utf8ToUtf16(const std::string& src)
{
	if (src.empty()) return L"";

	int len = MultiByteToWideChar(
		CP_UTF8,
		0,
		src.c_str(),
		static_cast<int>(src.size()),
		nullptr,
		0
	);

	std::wstring dst(len, L'\0');

	MultiByteToWideChar(
		CP_UTF8,
		0,
		src.c_str(),
		static_cast<int>(src.size()),
		dst.data(),
		len
	);

	return dst;
}

std::wstring SjisToUtf16(const std::string& s)
{
	if (s.empty()) return L"";

	int len = MultiByteToWideChar(
		932,
		0,
		s.c_str(),
		(int)s.size(),
		nullptr,
		0);

	std::wstring dst(len, L'\0');

	MultiByteToWideChar(
		932,
		0,
		s.c_str(),
		(int)s.size(),
		dst.data(),
		len);

	return dst;
}

std::string Utf16ToUtf8(const std::wstring& s)
{
	if (s.empty()) return "";

	const int required = WideCharToMultiByte(
		CP_UTF8, 0,
		s.c_str(), -1,
		nullptr, 0,
		nullptr, nullptr);

	if (required <= 0) return "";

	std::string utf8;
	utf8.resize(required);

	WideCharToMultiByte(
		CP_UTF8, 0,
		s.c_str(), -1,
		utf8.data(), required,
		nullptr, nullptr);

	if (!utf8.empty() && utf8.back() == '\0')
	{
		utf8.pop_back();
	}

	return utf8;
}

std::string SjisToUtf8(const std::string& s)
{
	std::wstring utf16 = SjisToUtf16(s);
	std::string utf8 = Utf16ToUtf8(utf16);
	return utf8;
}

void SendUnicodeText(const std::wstring& text)
{
	for (wchar_t ch : text)
	{
		SendUnicodeChar(ch);
		Sleep(5);
	}
}

void SendUnicodeChar(wchar_t ch)
{

	INPUT input[2] = {};

	// Key Down
	input[0].type = INPUT_KEYBOARD;
	input[0].ki.wVk = 0;
	input[0].ki.wScan = ch;
	input[0].ki.dwFlags = KEYEVENTF_UNICODE;

	// Key Up
	input[1].type = INPUT_KEYBOARD;
	input[1].ki.wVk = 0;
	input[1].ki.wScan = ch;
	input[1].ki.dwFlags =
		KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;

	SendInput(_countof(input), input, sizeof(INPUT));
}

void SendClipboardText(const std::wstring& text)
{
	if (text.empty()) return;

	if (!OpenClipboard(nullptr)) return;

	EmptyClipboard();

	const SIZE_T size = (text.length() + 1) * sizeof(wchar_t);
	HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, size);
	if (!hMem)
	{
		CloseClipboard();
		return;
	}
	void* pMem = GlobalLock(hMem);
	if (!pMem)
	{
		GlobalFree(hMem);
		CloseClipboard();
		return;
	}
	memcpy(pMem, text.c_str(), size);
	GlobalUnlock(hMem);

	if (!SetClipboardData(CF_UNICODETEXT, hMem))
	{
		GlobalFree(hMem);
		CloseClipboard();
		return;
	}
	/*
		SetClipboardData成功後は
		hMemの所有権がWindows側へ移る
	*/
	CloseClipboard();

	// Ctrl + V
	INPUT input[4] = {};

	input[0].type = INPUT_KEYBOARD;
	input[0].ki.wVk = VK_CONTROL;

	input[1].type = INPUT_KEYBOARD;
	input[1].ki.wVk = 'V';

	input[2].type = INPUT_KEYBOARD;
	input[2].ki.wVk = 'V';
	input[2].ki.dwFlags = KEYEVENTF_KEYUP;

	input[3].type = INPUT_KEYBOARD;
	input[3].ki.wVk = VK_CONTROL;
	input[3].ki.dwFlags = KEYEVENTF_KEYUP;

	SendInput(_countof(input), input, sizeof(INPUT));
}

bool IsExtendedKey(WORD vk)
{
	switch (vk)
	{
	case VK_LEFT:
	case VK_RIGHT:
	case VK_UP:
	case VK_DOWN:

	case VK_HOME:
	case VK_END:
	case VK_PRIOR:      // Page Up
	case VK_NEXT:       // Page Down
	case VK_INSERT:
	case VK_DELETE:

	case VK_RCONTROL:
	case VK_RMENU:

	case VK_NUMLOCK:
	case VK_DIVIDE:

		return true;
	}

	return false;
}

void SendKey(WORD vk)
{
	WORD scan =
		static_cast<WORD>(
			MapVirtualKey(vk, MAPVK_VK_TO_VSC));

	DWORD flags =
		KEYEVENTF_SCANCODE;

	if (IsExtendedKey(vk))
	{
		flags |= KEYEVENTF_EXTENDEDKEY;
	}

	INPUT input[2] = {};

	// Key Down
	input[0].type = INPUT_KEYBOARD;
	input[0].ki.wVk = 0;
	input[0].ki.wScan = scan;
	input[0].ki.dwFlags = flags;

	// Key Up
	input[1].type = INPUT_KEYBOARD;
	input[1].ki.wVk = 0;
	input[1].ki.wScan = scan;
	input[1].ki.dwFlags = flags | KEYEVENTF_KEYUP;

	SendInput(_countof(input), input, sizeof(INPUT));
}

std::string GetLocalIPv4()
{
	char hostname[256] = {};

	if (gethostname(hostname, sizeof(hostname)) == SOCKET_ERROR)
		return "";

	addrinfo hints{};
	hints.ai_family = AF_INET;      // IPv4
	hints.ai_socktype = SOCK_STREAM;

	addrinfo* result = nullptr;

	if (getaddrinfo(hostname, nullptr, &hints, &result) != 0)
		return "";

	std::string ip;

	for (addrinfo* p = result; p != nullptr; p = p->ai_next)
	{
		sockaddr_in* addr =
			reinterpret_cast<sockaddr_in*>(p->ai_addr);

		char buf[INET_ADDRSTRLEN] = {};

		if (inet_ntop(
			AF_INET,
			&addr->sin_addr,
			buf,
			sizeof(buf)))
		{
			std::string candidate = buf;

			if (candidate != "127.0.0.1")
			{
				ip = candidate;
				break;
			}
		}
	}

	freeaddrinfo(result);

	return ip;
}

void DrawQrCodeBox(HDC hdc, const std::string& utf8, int x, int y, int size)
{
    if (utf8.empty() || size <= 0)
        return;

    try
    {
        using qrcodegen::QrCode;
        const QrCode qr = QrCode::encodeText(utf8.c_str(), QrCode::Ecc::MEDIUM);

        const int borderPad = 1;
        const int quietZone = 2;

        const int innerX = x + borderPad;
        const int innerY = y + borderPad;
        const int innerSize = size - borderPad * 2;

        const int modules = qr.getSize();
        const int totalModules = modules + quietZone * 2;
        const int cellSize = (std::max)(1, innerSize / totalModules);
        const int drawSize = cellSize * totalModules;

        const int qrX = innerX + (innerSize - drawSize) / 2;
        const int qrY = innerY + (innerSize - drawSize) / 2;

		HBRUSH hWhiteBrush = (HBRUSH)GetStockObject(WHITE_BRUSH);
		HBRUSH hBlackBrush = (HBRUSH)GetStockObject(BLACK_BRUSH);
		HPEN hGrayPen = (HPEN)CreatePen(PS_SOLID, 0, RGB(96, 96, 96));

		HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, hWhiteBrush);
		HPEN hOldPen = (HPEN)SelectObject(hdc, hGrayPen);
		RoundRect(hdc, x, y, x + size, y + size, 12, 12);

		SelectObject(hdc, hBlackBrush);
        for (int my = 0; my < modules; ++my)
        {
            for (int mx = 0; mx < modules; ++mx)
            {
                if (!qr.getModule(mx, my)) continue;

                const int x1 = qrX + (mx + quietZone) * cellSize;
                const int y1 = qrY + (my + quietZone) * cellSize;
                const int x2 = x1 + cellSize;
                const int y2 = y1 + cellSize;

				Rectangle(hdc, x1, y1, x2, y2);
			}
        }
	
		SelectObject(hdc, hOldBrush);
		SelectObject(hdc, hOldPen);
		DeleteObject(hGrayPen);
	}
    catch (...)
    {
        // payloadが長すぎるなどでQR生成に失敗した場合は何も描かない
    }

}

void DrawTextLine(HDC hdc, int x, int y, std::wstring& strTextLine)
{
	SIZE sz;
	GetTextExtentPoint32(hdc, strTextLine.c_str(), strTextLine.length(), &sz);
	RECT rc = { x, y, x + sz.cx + 4, y + sz.cy + 4 };
	DrawText(hdc, strTextLine.c_str(), -1, &rc, DT_SINGLELINE | DT_END_ELLIPSIS);
}

/*----------------------------------------------------------------------------------------
	WinMain
----------------------------------------------------------------------------------------*/
int APIENTRY _tWinMain(HINSTANCE hCurInst, HINSTANCE hPrevInst, LPTSTR lpsCmdLine, int nCmdShow)
{
	if (hPrevInst != NULL) return 0;

	HANDLE hMutex = CreateMutex(NULL, TRUE, APP_CLASS);
	if (GetLastError() == ERROR_ALREADY_EXISTS) {
		SetForegroundWindow(FindWindow(APP_CLASS, NULL));
		if (hMutex) CloseHandle(hMutex);
		return 0;
	}

	WNDCLASSEX wcex;
	HWND hWnd;
	ZeroMemory(&wcex, sizeof(WNDCLASSEX));
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = MainWndProc;
	wcex.hInstance = hCurInst;
	wcex.hIcon = (HICON)LoadImage(hCurInst,
		MAKEINTRESOURCE(IDI_ICON1),
		IMAGE_ICON,
		32, 32,
		LR_DEFAULTCOLOR | LR_SHARED);
	wcex.hCursor = (HCURSOR)LoadImage(NULL,
		MAKEINTRESOURCE(IDC_ARROW),
		IMAGE_CURSOR,
		0, 0,
		LR_DEFAULTCOLOR | LR_SHARED);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszClassName = APP_CLASS;
	wcex.lpszMenuName = MAKEINTRESOURCE(IDR_MENU1);
	wcex.hIconSm = (HICON)LoadImage(hCurInst,
		MAKEINTRESOURCE(IDI_ICON1),
		IMAGE_ICON,
		16, 16,
		LR_DEFAULTCOLOR | LR_SHARED);
	if (!RegisterClassEx(&wcex)) {
		MessageBox(HWND_DESKTOP,
			(LPCTSTR)_T("Error RegisterClassEx"), (LPCTSTR)APP_CLASS,
			MB_OK | MB_ICONSTOP | MB_SYSTEMMODAL);
		return 0;
	}

	RECT rc = {
		0,
		0,
		780,
		520
	};

	DWORD style =
		WS_OVERLAPPED |
		WS_CAPTION |
		WS_SYSMENU |
		WS_MINIMIZEBOX;

	AdjustWindowRectEx(
		&rc,
		style,
		FALSE,
		0);

	int windowWidth =
		rc.right - rc.left;
	int windowHeight =
		rc.bottom - rc.top;

	hWnd = CreateWindowEx(0,
		APP_CLASS,
		(LPCTSTR)APP_CLASS,
		style,
		CW_USEDEFAULT, CW_USEDEFAULT,
		windowWidth, windowHeight,
		NULL,
		NULL,
		hCurInst,
		NULL);
	if (!hWnd) {
		MessageBox(HWND_DESKTOP,
			(LPCTSTR)_T("Error CreateWindowEx"), (LPCTSTR)APP_CLASS,
			MB_OK | MB_ICONSTOP | MB_SYSTEMMODAL);
		return 0;
	}

	PreCreateWindow(hWnd, lpsCmdLine, nCmdShow);

	MSG msg;
	int nRet;
	while ((nRet = GetMessage(&msg, NULL, 0, 0))) {
		if (nRet < 0) break;
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	if (hMutex) CloseHandle(hMutex);

	return (int)(msg.wParam);
}

INT_PTR PreCreateWindow(HWND hWnd, LPTSTR lpsCmdLine, int nCmdShow)
{
	if (Gdiplus::GdiplusStartup(&ggdiplusToken, &ggdiplusStartupInput, NULL) != Gdiplus::Ok)
	{
		return -1;
	}

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	return 0;
}

LRESULT CALLBACK MainWndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg)
	{
	case WM_CREATE:
		return (OnCreateWindow(hWnd, msg, wp, lp));
	case WM_PAINT:
		return (OnPaint(hWnd, msg, wp, lp));
	case WM_COMMAND:
		return (OnCommand(hWnd, msg, wp, lp));
	case UM_HTTPPOPMSG:
		return (OnHttpPopMessage(hWnd, msg, wp, lp));
	case UM_HTTPPOPKEY:
		return (OnHttpPopKey(hWnd, msg, wp, lp));
	case UM_HTTPSTATE:
		return (OnHttpState(hWnd, msg, wp, lp));
	case WM_ENDSESSION:
	case WM_CLOSE:
		return (OnClose(hWnd, msg, wp, lp));
	case WM_DESTROY:
		return (OnDestroy(hWnd, msg, wp, lp));
	default:
		return (DefWindowProc(hWnd, msg, wp, lp));
	}
	return (0L);
}

LRESULT OnCreateWindow(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	CSimpleHttpServer* lpCSimpleHttpServer = (CSimpleHttpServer*)new CSimpleHttpServer();
	SetProp(hWnd, CSIMPLEHTTPSERVER, lpCSimpleHttpServer);

	CAppColorTheme* lpCAppColorTheme = (CAppColorTheme*)new CAppColorTheme();
	SetProp(hWnd, CAPPCOLORTHEME, lpCAppColorTheme);

	return (0L);
}

LRESULT OnPaint(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	PAINTSTRUCT ps;
	BeginPaint(hWnd, &ps);

	RECT rc;
	GetClientRect(hWnd, &rc);

	CAppColorTheme* lpCAppColorTheme = (CAppColorTheme*)GetProp(hWnd, CAPPCOLORTHEME);
	if (!lpCAppColorTheme) return 0L;

	HDC hmdc = CreateCompatibleDC(ps.hdc);
	HBITMAP hbmp = CreateCompatibleBitmap(ps.hdc, rc.right, rc.bottom);
	HBITMAP holdbmp = (HBITMAP)SelectObject(hmdc, hbmp);

	FillRect(hmdc, &rc, lpCAppColorTheme->WindowBrush());

	CImgListPng cilp;
	Gdiplus::Bitmap* pBitmap = cilp.LoadPngBitmap((HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE), IDB_PNG1);
	if (pBitmap)
	{
		Gdiplus::Graphics graphics(hmdc);
		graphics.DrawImage(pBitmap, 16, 8, pBitmap->GetWidth(), pBitmap->GetHeight());
		delete pBitmap;
	}

	HBRUSH holdbrush = (HBRUSH)SelectObject(hmdc, lpCAppColorTheme->PanelBrush(0));
	HPEN holdpen = (HPEN)SelectObject(hmdc, lpCAppColorTheme->PanelPen(0));
	// Server
	RoundRect(hmdc,
		16, 88,
		255, 288,
		18, 18);

	// Options
	SelectObject(hmdc, lpCAppColorTheme->PanelBrush(1));
	SelectObject(hmdc, lpCAppColorTheme->PanelPen(1));
	RoundRect(hmdc,
		271, 88,
		510, 288,
		18, 18);

	// QRCode
	SelectObject(hmdc, lpCAppColorTheme->PanelBrush(2));
	SelectObject(hmdc, lpCAppColorTheme->PanelPen(2));
	RoundRect(hmdc,
		526, 88,
		764, 288,
		18, 18);

	// Logs
	SelectObject(hmdc, lpCAppColorTheme->PanelBrush(3));
	SelectObject(hmdc, lpCAppColorTheme->PanelPen(3));
	RoundRect(hmdc,
		16, 304,
		764, 482,
		18, 18);

	SelectObject(hmdc, holdbrush);
	SelectObject(hmdc, holdpen);

	int noldbkmode = SetBkMode(hmdc, TRANSPARENT);
	HFONT hfontbold = CreateFont(-MulDiv((INT_PTR)12, GetDeviceCaps(hmdc, LOGPIXELSY), 72),
		0, 0, 0, FW_BOLD,
		FALSE, FALSE, FALSE,
		DEFAULT_CHARSET,
		OUT_DEFAULT_PRECIS,
		CLIP_DEFAULT_PRECIS,
		ANTIALIASED_QUALITY,
		DEFAULT_PITCH | FF_DONTCARE,
		_T("Meiryo UI"));
	HFONT hfont = CreateFont(-MulDiv((INT_PTR)11, GetDeviceCaps(hmdc, LOGPIXELSY), 72),
		0, 0, 0, FW_BOLD,
		FALSE, FALSE, FALSE,
		DEFAULT_CHARSET,
		OUT_DEFAULT_PRECIS,
		CLIP_DEFAULT_PRECIS,
		ANTIALIASED_QUALITY,
		DEFAULT_PITCH | FF_DONTCARE,
		_T("Meiryo UI"));
	HFONT hOldFont = (HFONT)SelectObject(hmdc, hfontbold);
	COLORREF cololdtextcolor = SetTextColor(hmdc, (lpCAppColorTheme->Colors()).text);
	// Server
	std::wstring wstr = _T("Server");
	DrawTextLine(hmdc, 32, 98, wstr);
	// Options
	wstr = _T("Options");
	DrawTextLine(hmdc, 287, 98, wstr);
	// QRCode
	wstr = _T("QR Code");
	DrawTextLine(hmdc, 542, 98, wstr);
	// Logs
	wstr = _T("Logs");
	DrawTextLine(hmdc, 32, 314, wstr);

	SelectObject(hmdc, hfont);
	std::wstring wstrState;
	COLORREF colState;
	CSimpleHttpServer* pServer =
		(CSimpleHttpServer*)GetProp(hWnd, CSIMPLEHTTPSERVER);
	if (pServer) {
		switch (pServer->GetState())
		{
		case ServerState::Starting:
			wstrState = L"● Starting";
			colState = RGB(7, 207, 1);
			break;

		case ServerState::Running:
			wstrState = L"● Running";
			colState = RGB(7, 207, 1);
			{
				std::string ip = GetLocalIPv4();
				std::string strURL = "http://" + ip + ":" + std::to_string(gnPort);
				DrawQrCodeBox(hmdc, strURL, 588, 130, 110);
			}
			break;
		case ServerState::Stopping:
			wstrState = L"■ Stopping";
			colState = RGB(37, 87, 225);
			break;

		case ServerState::Stopped:
			wstrState = L"■ Stopped";
			colState = RGB(37, 87, 225);
			break;

		case ServerState::Error:
			wstrState = L"▲ Error";
			colState = RGB(220, 1, 1);
			break;

		}
	}

	// Server
	wstr = _T("STATUS");
	DrawTextLine(hmdc, 32, 128, wstr);
	wstr = _T("URL");
	DrawTextLine(hmdc, 32, 156, wstr);
	wstr = _T("PORT");
	DrawTextLine(hmdc, 32, 184, wstr);
	SetTextColor(hmdc, colState);
	DrawTextLine(hmdc, 106, 128, wstrState);

	BitBlt(ps.hdc, 0, 0, rc.right, rc.bottom, hmdc, 0, 0, SRCCOPY);

	SetBkMode(hmdc, noldbkmode);

	SelectObject(hmdc, hOldFont);
	DeleteObject(hfontbold);
	DeleteObject(hfont);

	SelectObject(hmdc, holdbmp);
	DeleteObject(hbmp);
	DeleteDC(hmdc);

	EndPaint(hWnd, &ps);

	return 0L;
}

LRESULT OnCommand(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	CSimpleHttpServer* lpCSimpleHttpServer;
	switch (LOWORD(wp))
	{
	case ID_HTTP_START:
		lpCSimpleHttpServer =
			(CSimpleHttpServer*)GetProp(hWnd, CSIMPLEHTTPSERVER);
		if (lpCSimpleHttpServer) {
			lpCSimpleHttpServer->Start(hWnd);
		}
		break;
	case ID_HTTP_STOP:
		lpCSimpleHttpServer =
			(CSimpleHttpServer*)GetProp(hWnd, CSIMPLEHTTPSERVER);
		if (lpCSimpleHttpServer){
			lpCSimpleHttpServer->Stop();
		}
		break;
	case ID_FILE_QUIT:
		SendMessage(hWnd, WM_CLOSE, 0L, 0L);
		break;
	default:
		return (DefWindowProc(hWnd, msg, wp, lp));
	}
	return (0L);
}

LRESULT OnHttpPopMessage(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	CSimpleHttpServer* lpCSimpleHttpServer = (CSimpleHttpServer*)GetProp(hWnd, CSIMPLEHTTPSERVER);
	if (!lpCSimpleHttpServer) return 0L;

	std::string strPop;
	if (lpCSimpleHttpServer->PopMessage(strPop)) {
		std::wstring strPopW = Utf8ToUtf16(strPop);
		SendUnicodeText(strPopW);
		InvalidateRect(hWnd, NULL, TRUE);
	}

	return (0L);
}

LRESULT OnHttpPopKey(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	CSimpleHttpServer* pServer = (CSimpleHttpServer*)GetProp(hWnd, CSIMPLEHTTPSERVER);
	if (!pServer)return 0L;

	WORD vk;
	if (pServer->PopKey(vk))
	{
		SendKey(vk);
	}
	return (0L);
}

LRESULT OnHttpState(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	InvalidateRect(hWnd, NULL, TRUE);
	return (0L);
}

LRESULT OnClose(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	CSimpleHttpServer* lpCSimpleHttpServer = (CSimpleHttpServer*)GetProp(hWnd, CSIMPLEHTTPSERVER);
	if (lpCSimpleHttpServer)
	{
		RemoveProp(hWnd, CSIMPLEHTTPSERVER);
		delete lpCSimpleHttpServer;
	}

	CAppColorTheme* lpCAppColorTheme  = (CAppColorTheme*)GetProp(hWnd, CAPPCOLORTHEME);
	if (lpCAppColorTheme)
	{
		RemoveProp(hWnd, CAPPCOLORTHEME);
		delete lpCAppColorTheme;
	}

	Gdiplus::GdiplusShutdown(ggdiplusToken);

	DestroyWindow(hWnd);

	return (0L);
}

LRESULT OnDestroy(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	PostQuitMessage(0);
	return (0L);
}

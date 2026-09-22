#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <commctrl.h>
#include <tchar.h>
#include <vector>
#include <algorithm>
#include <string>
#include "qrcodegen.hpp"
#include "CSimpleHttpServer.h"
#include "CAppColorTheme.h"
#include "CImgListPng.h"
#include "CFlicksyConfig.h"
#include "CTrayIcon.h"
#include "CToggleSwitch.h"
#include "resource.h"
#include "Win32VisualStyle.h"

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "version.lib")

#define APP_CLASS			_T("Flicksy")
#define CSIMPLEHTTPSERVER	_T("CSimpleHttpServer")
#define CAPPCOLORTHEME		_T("CAppColorTheme")
#define CFLICKSYCONFIG		_T("CFlicksyConfig")
#define CTRAYICON			_T("CTrayIcon")

#define IDC_EDIT_PORT       1001
#define IDC_SPIN_PORT		1002
#define IDC_EDIT_ROOT       1003
#define IDC_RADIO_SENDINPUT	1004
#define IDC_RADIO_CLIPBOARD	1005
#define IDC_CHECK_TOPMOST	1006
#define IDC_CHECK_AUTOSTART	1007
#define IDC_CHECK_STARTUP	1008
#define IDC_LIST_LOG		1009
#define IDC_SWITCH_THEME	1010

std::filesystem::path gflicksytoml;
static Gdiplus::GdiplusStartupInput ggdiplusStartupInput;
static Gdiplus::Bitmap* gpBitmapBanner;
static Gdiplus::Bitmap* gpBitmapBannerDark;
static HIMAGELIST ghImgListBtn, ghImgListBtnHover, ghImgListBtnPressed, ghImgListBtnGrayed, ghImgListBtnPin;
static ULONG_PTR ggdiplusToken = 0;
static HFONT ghFontBold, ghFont;
std::wstring gwstrVer;

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
std::wstring GetVersionString(const wchar_t* key);

void DrawBackground(HWND hWnd, HDC hdc, RECT& rc, const CAppColorTheme& theme);
void DrawHeader(HWND hWnd, HDC hdc, RECT& rc, const CAppColorTheme& theme);
void DrawPanels(HWND hWnd, HDC hdc, const CAppColorTheme& theme);
void DrawPanel(HDC hdc, const CAppColorTheme& theme, int index, const RECT& rc);
void DrawServerPanel(HWND hWnd, HDC hdc, const CAppColorTheme& theme);
void DrawOptionPanel(HWND hWnd, HDC hdc, const CAppColorTheme& theme);
void DrawQrPanel(HWND hWnd, HDC hdc, const CAppColorTheme& theme);
void DrawLogPanel(HWND hWnd, HDC hdc, const CAppColorTheme& theme);
void DrawQrCodeBox(HDC hdc, const std::string& utf8, int x, int y, int size);
void DrawTextLine(HDC hdc, int x, int y, const std::wstring& strTextLine, const UINT format = DT_SINGLELINE | DT_END_ELLIPSIS);
void DrawTextLine(HDC hdc, RECT& rc, const std::wstring& strTextLine, UINT format);

#define BUTTONSUBCLASS			_T("OldButtonProc")
void EnterSubclassButton(HWND hButton);
void LeaveSubclassButton(HWND hButton);
LRESULT CALLBACK StartStopBtnProc(HWND hBtn, UINT uMsg, WPARAM wParam, LPARAM lParam);

#define REG_STARTUP				_T("Software\\Microsoft\\Windows\\CurrentVersion\\Run")
#define REG_VALUE				_T("Flicksy")
bool SetRunAtStartup(bool enabled);
bool IsRunAtStartup();

constexpr int MAX_LOG_COUNT = 1000;
enum class LogType
{
	Info,
	Server,
	Input,
	Error
};
typedef struct LogItem
{
	LogType type;
	SYSTEMTIME st;
	std::wstring text;
} LOGITEM, * LPLOGITEM;
void AddLog(HWND hWnd, const LogType type, const std::wstring& text);
COLORREF GetLogColor(LogType type);

INT_PTR PreCreateWindow(HWND hWnd, LPTSTR lpsCmdLine, int nCmdShow);
LRESULT CALLBACK MainWndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp);
LRESULT OnCreateWindow(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp);
LRESULT OnPaint(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp);
LRESULT OnMeasureItem(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp);
LRESULT OnDrawItem(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp);
LRESULT OnDrawStartStopBtn(HWND hWnd, LPDRAWITEMSTRUCT lpdis);
LRESULT OnDrawTopmost(HWND hWnd, LPDRAWITEMSTRUCT lpdis);
LRESULT OnDrawListLogs(HWND hWnd, LPDRAWITEMSTRUCT lpdis);
LRESULT OnCommand(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp);
LRESULT OnCtlColor(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp);
LRESULT OnHttpPopMessage(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp);
LRESULT OnHttpPopKey(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp);
LRESULT OnHttpState(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp);
LRESULT OnHttpLog(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp);
LRESULT OnTrayIcon(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp);
LRESULT OnClose(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp);
LRESULT OnDestroy(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp);

void ApplyTheme(HWND hWnd);

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

std::wstring GetVersionString(const wchar_t* key)
{
	wchar_t path[MAX_PATH] = {};
	GetModuleFileNameW(nullptr, path, _countof(path));

	DWORD handle = 0;
	DWORD size = GetFileVersionInfoSizeW(path, &handle);
	if (size == 0) return L"";

	std::vector<BYTE> data(size);

	if (!GetFileVersionInfoW(path, 0, size, data.data()))
	{
		return L"";
	}

	struct LANGANDCODEPAGE
	{
		WORD wLanguage;
		WORD wCodePage;
	};

	LANGANDCODEPAGE* translate = nullptr;
	UINT translateSize = 0;

	if (!VerQueryValueW(data.data(),
		L"\\VarFileInfo\\Translation",
		reinterpret_cast<LPVOID*>(&translate),
		&translateSize))
	{
		return L"";
	}

	if (translateSize < sizeof(LANGANDCODEPAGE)) return L"";

	wchar_t query[256] = {};

	swprintf_s(query, L"\\StringFileInfo\\%04x%04x\\%s", translate[0].wLanguage, translate[0].wCodePage, key);

	wchar_t* value = nullptr;
	UINT valueSize = 0;

	if (!VerQueryValueW(data.data(), query, reinterpret_cast<LPVOID*>(&value), &valueSize))
	{
		return L"";
	}

	return value ? value : L"";
}

void DrawBackground(HWND hWnd, HDC hdc, RECT& rc, const CAppColorTheme& theme)
{
	FillRect(hdc, &rc, theme.WindowBrush());

	SelectObject(hdc, ghFont);
	SetTextColor(hdc, (theme.Colors()).subText);
	SetBkColor(hdc, (theme.Colors()).windowBg);
	std::wstring wstr = _T("DarkMode");
	DrawTextLine(hdc, 640, 509, wstr);
}

void DrawHeader(HWND hWnd, HDC hdc, RECT& rc, const CAppColorTheme& theme)
{
	HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, theme.HeaderBrush());
	HPEN hOldPen = (HPEN)SelectObject(hdc, theme.HeaderPen());
	RoundRect(hdc, 2, 1, rc.right - 2, 84, 4, 4);

	if (gpBitmapBanner && gpBitmapBannerDark)
	{
		Gdiplus::Graphics graphics(hdc);
		if (theme.Mode() == ColorMode::Dark) {
			graphics.DrawImage(gpBitmapBannerDark, 16, 6, gpBitmapBannerDark->GetWidth(), gpBitmapBannerDark->GetHeight());
		}
		else {
			graphics.DrawImage(gpBitmapBanner, 16, 6, gpBitmapBanner->GetWidth(), gpBitmapBanner->GetHeight());
		}
	}

	HFONT hOldFont = (HFONT)SelectObject(hdc, ghFont);
	COLORREF colOldBkColor = SetBkColor(hdc, (theme.Colors()).headerBg);
	COLORREF colOldText = SetTextColor(hdc, (theme.Colors()).subText);
	DrawTextLine(hdc, 700, 52, gwstrVer);
	SetBkColor(hdc, colOldBkColor);
	SetTextColor(hdc, colOldText);
	SelectObject(hdc, hOldFont);

	SelectObject(hdc, hOldBrush);
	SelectObject(hdc, hOldPen);
}

void DrawPanels(HWND hWnd, HDC hdc, const CAppColorTheme& theme)
{
	DrawPanel(hdc, theme, 0,
		{ 16, 96, 255, 296 });

	DrawPanel(hdc, theme, 1,
		{ 271, 96, 510, 296 });

	DrawPanel(hdc, theme, 2,
		{ 526, 96, 764, 296 });

	DrawPanel(hdc, theme, 3,
		{ 16, 310, 764, 502 });
}

void DrawPanel(HDC hdc, const CAppColorTheme& theme, int index, const RECT& rc)
{
	HGDIOBJ oldBrush = SelectObject(hdc, theme.PanelBrush(index));
	HGDIOBJ oldPen = SelectObject(hdc, theme.PanelPen(index));

	RoundRect(hdc,
		rc.left,
		rc.top,
		rc.right,
		rc.bottom,
		18,
		18);

	SelectObject(hdc, oldBrush);
	SelectObject(hdc, oldPen);
}

void DrawServerPanel(HWND hWnd, HDC hdc, const CAppColorTheme& theme)
{
	HFONT hOldFont = (HFONT)SelectObject(hdc, ghFontBold);
	COLORREF oldtextcolor = SetTextColor(hdc, (theme.Colors()).text);
	COLORREF oldbkcolor = SetBkColor(hdc, (theme.Colors()).panelBg[0]);
	std::wstring wstr = _T("Server");
	DrawTextLine(hdc, 32, 102, wstr);

	std::wstring wstrState;
	COLORREF colState = theme.Colors().subText;
	CSimpleHttpServer* pServer = (CSimpleHttpServer*)GetProp(hWnd, CSIMPLEHTTPSERVER);
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
			break;
		case ServerState::Stopping:
			wstrState = L"■ Stopping";
			colState = RGB(220, 10, 10);
			break;

		case ServerState::Stopped:
			wstrState = L"■ Stopped";
			colState = RGB(220, 10, 10);
			break;

		case ServerState::Error:
			wstrState = L"▲ Error";
			colState = RGB(250, 1, 1);
			break;

		}
	}

	SetTextColor(hdc, (theme.Colors()).panelText[0]);
	SelectObject(hdc, ghFont);
	wstr = _T("STATUS");
	DrawTextLine(hdc, 32, 138, wstr);
	wstr = _T("PORT");
	DrawTextLine(hdc, 32, 167, wstr);
	wstr = _T("ROOT");
	DrawTextLine(hdc, 32, 198, wstr);

	SetTextColor(hdc, colState);
	DrawTextLine(hdc, 106, 138, wstrState);

	SelectObject(hdc, hOldFont);
	SetTextColor(hdc, oldtextcolor);
	SetBkColor(hdc, oldbkcolor);
}

void DrawOptionPanel(HWND hWnd, HDC hdc, const CAppColorTheme& theme)
{
	HFONT hOldFont = (HFONT)SelectObject(hdc, ghFontBold);
	COLORREF oldtextcolor = SetTextColor(hdc, (theme.Colors()).text);
	COLORREF oldbkcolor = SetBkColor(hdc, (theme.Colors()).panelBg[1]);
	std::wstring wstr = _T("Options");
	DrawTextLine(hdc, 287, 102, wstr);

	SetTextColor(hdc, (theme.Colors()).panelText[1]);
	SelectObject(hdc, ghFont);
	wstr = _T("TRANSMISSION");
	DrawTextLine(hdc, 287, 138, wstr);
	wstr = _T("STARTUP");
	DrawTextLine(hdc, 287, 216, wstr);

	SelectObject(hdc, hOldFont);
	SetTextColor(hdc, oldtextcolor);
	SetBkColor(hdc, oldbkcolor);
}

void DrawQrPanel(HWND hWnd, HDC hdc, const CAppColorTheme& theme)
{
	HFONT hOldFont = (HFONT)SelectObject(hdc, ghFontBold);
	COLORREF oldtextcolor = SetTextColor(hdc, (theme.Colors()).text);
	COLORREF oldbkcolor = SetBkColor(hdc, (theme.Colors()).panelBg[2]);
	std::wstring wstr = _T("QR code");
	DrawTextLine(hdc, 542, 102, wstr);

	SetTextColor(hdc, (theme.Colors()).panelText[2]);
	CSimpleHttpServer* lpCSimpleHttpServer = (CSimpleHttpServer*)GetProp(hWnd, CSIMPLEHTTPSERVER);
	if (lpCSimpleHttpServer)
	{
		if (lpCSimpleHttpServer->IsRunning())
		{
			std::string ip = GetLocalIPv4();
			std::string port = std::to_string(lpCSimpleHttpServer->GetPort());
			std::string strURL = "http://" + ip + ":" + port + "/?token=" + lpCSimpleHttpServer->GetToken();
			DrawQrCodeBox(hdc, strURL, 588, 134, 110);
			SelectObject(hdc, ghFont);
			std::wstring wstrURL = Utf8ToUtf16(ip) + L":" + Utf8ToUtf16(port);
			RECT rc = { 527, 250, 762, 290 };
			DrawTextLine(hdc, rc, wstrURL, DT_SINGLELINE | DT_CENTER);
		}
		else {
			SelectObject(hdc, ghFont);
			wstr = _T("Displays a QR code when the server is running.");
			RECT rc = { 542, 137, 724, 294 };
			DrawTextLine(hdc, rc, wstr, DT_WORDBREAK | DT_END_ELLIPSIS);
		}
	}

	SelectObject(hdc, hOldFont);
	SetTextColor(hdc, oldtextcolor);
	SetBkColor(hdc, oldbkcolor);
}

void DrawLogPanel(HWND hWnd, HDC hdc, const CAppColorTheme& theme)
{
	HFONT hOldFont = (HFONT)SelectObject(hdc, ghFontBold);
	COLORREF oldtextcolor = SetTextColor(hdc, (theme.Colors()).text);
	COLORREF oldbkcolor = SetBkColor(hdc, (theme.Colors()).panelBg[3]);
	std::wstring wstr = _T("Logs");
	DrawTextLine(hdc, 32, 318, wstr);


	SelectObject(hdc, hOldFont);
	SetTextColor(hdc, oldtextcolor);
	SetBkColor(hdc, oldbkcolor);
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
		HPEN hGrayPen = (HPEN)CreatePen(PS_SOLID, 0, RGB(128, 128, 128));

		HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, hWhiteBrush);
		HPEN hOldPen = (HPEN)SelectObject(hdc, hGrayPen);
		RoundRect(hdc, x, y, x + size, y + size, 12, 12);

		SelectObject(hdc, hBlackBrush);
		SelectObject(hdc, (HPEN)GetStockObject(BLACK_PEN));
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

void DrawTextLine(HDC hdc, int x, int y, const std::wstring& strTextLine, UINT format)
{
	SIZE sz;
	GetTextExtentPoint32(hdc, strTextLine.c_str(), strTextLine.length(), &sz);
	RECT rc = { x, y, x + sz.cx + 4, y + sz.cy + 4 };
	DrawTextLine(hdc, rc, strTextLine.c_str(), format);
}

void DrawTextLine(HDC hdc, RECT& rc, const std::wstring& strTextLine, UINT format)
{
	DrawText(hdc, strTextLine.c_str(), -1, &rc, format);
}

void AddLog(HWND hWnd, const LogType type, const std::wstring& text)
{
	HWND hList = GetDlgItem(hWnd, IDC_LIST_LOG);
	if (!hList) return;

	int count = (int)SendMessage(hList, LB_GETCOUNT, 0, 0);
	if (count >= MAX_LOG_COUNT)
	{
		LPLOGITEM lpli = reinterpret_cast<LPLOGITEM>(SendMessage(hList, LB_GETITEMDATA, 0, 0));
		if (lpli) delete lpli;
		SendMessage(hList, LB_DELETESTRING, 0, 0);
	}

	LPLOGITEM lpli = new LOGITEM;
	if (lpli) {
		lpli->type = type;
		GetLocalTime(&lpli->st);
		lpli->text = text;
		int index = (int)SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)lpli);
		if (index == LB_ERR || index == LB_ERRSPACE)
		{
			delete lpli;
			return;
		}
		SendMessage(hList, LB_SETTOPINDEX, index, 0);
	}
}

COLORREF GetLogColor(LogType type)
{
	switch (type)
	{
	case LogType::Server:
		return RGB(10, 70, 240);

	case LogType::Input:
		return RGB(10, 150, 10);

	case LogType::Error:
		return RGB(220, 50, 50);

	case LogType::Info:
	default:
		return RGB(80, 80, 80);
	}
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
	wcex.style = 0;
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
	wcex.hbrBackground = NULL;
	wcex.lpszClassName = APP_CLASS;
	wcex.lpszMenuName = NULL;
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

	AdjustWindowRectEx(&rc,
		style,
		TRUE,
		0);

	int windowWidth =
		rc.right - rc.left;
	int windowHeight =
		rc.bottom - rc.top;

	hWnd = CreateWindowEx(0,
		APP_CLASS,
		(LPCTSTR)APP_CLASS,
		style,
		GetSystemMetrics(SM_CXFULLSCREEN) - windowWidth,
		GetSystemMetrics(SM_CYFULLSCREEN) - windowHeight,
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
	gwstrVer = _T("ver ") + GetVersionString(L"ProductVersion");

	AddLog(hWnd, LogType::Info, L"Flicksy started.");

	CTrayIcon* lpCTrayIcon = (CTrayIcon*)GetProp(hWnd, CTRAYICON);
	if (lpCTrayIcon)
	{
		HICON hIcon = (HICON)LoadImage((HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
			MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
		lpCTrayIcon->Create(hWnd, hIcon, UM_TRAYICON);
	}

	CFlicksyConfig* lpCFlicksyConfig = (CFlicksyConfig*)GetProp(hWnd, CFLICKSYCONFIG);
	if (lpCFlicksyConfig)
	{
		if (lpCFlicksyConfig->AlwaysOnTop()) SendMessage(hWnd, WM_COMMAND, IDC_CHECK_TOPMOST, 0L);
		if (lpCFlicksyConfig->AutoStartServer()) {
			SendMessage(hWnd, WM_COMMAND, ID_HTTP_START, 0L);
		}
		else {
			ShowWindow(hWnd, nCmdShow);
			UpdateWindow(hWnd);
		}
	}

	return 0;
}

LRESULT CALLBACK MainWndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg)
	{
	case WM_CREATE:
		return (OnCreateWindow(hWnd, msg, wp, lp));
	case WM_ERASEBKGND:
		return 1;
	case WM_PAINT:
		return (OnPaint(hWnd, msg, wp, lp));
	case WM_MEASUREITEM:
		return (OnMeasureItem(hWnd, msg, wp, lp));
	case WM_DRAWITEM:
		return (OnDrawItem(hWnd, msg, wp, lp));
	case WM_COMMAND:
		return (OnCommand(hWnd, msg, wp, lp));
	case WM_SIZE:
		if (wp == SIZE_MINIMIZED)
		{
			ShowWindow(hWnd, SW_HIDE);
			break;
		}
		return (DefWindowProc(hWnd, msg, wp, lp));
	//case WM_CTLCOLORBTN:
	case WM_CTLCOLORSTATIC:
		return (OnCtlColor(hWnd, msg, wp, lp));
	case UM_HTTPPOPMSG:
		return (OnHttpPopMessage(hWnd, msg, wp, lp));
	case UM_HTTPPOPKEY:
		return (OnHttpPopKey(hWnd, msg, wp, lp));
	case UM_HTTPSTATE:
		return (OnHttpState(hWnd, msg, wp, lp));
	case UM_HTTPLOG:
		return (OnHttpLog(hWnd, msg, wp, lp));
	case UM_TRAYICON:
		return (OnTrayIcon(hWnd, msg, wp, lp));
	break;
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
	INITCOMMONCONTROLSEX icc;
	icc.dwSize = sizeof(INITCOMMONCONTROLSEX);
	icc.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&icc);

	HINSTANCE hInst = (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE);

	CSimpleHttpServer* lpCSimpleHttpServer = (CSimpleHttpServer*)new CSimpleHttpServer();
	SetProp(hWnd, CSIMPLEHTTPSERVER, lpCSimpleHttpServer);

	CAppColorTheme* lpCAppColorTheme = (CAppColorTheme*)new CAppColorTheme();
	SetProp(hWnd, CAPPCOLORTHEME, lpCAppColorTheme);

	CFlicksyConfig* lpCFlicksyConfig = (CFlicksyConfig*)new CFlicksyConfig();
	SetProp(hWnd, CFLICKSYCONFIG, lpCFlicksyConfig);

	CTrayIcon* lpCTrayIcon = (CTrayIcon*)new CTrayIcon();
	SetProp(hWnd, CTRAYICON, lpCTrayIcon);

	wchar_t path[MAX_PATH] = {};
	GetModuleFileName(nullptr, path, _countof(path));
	gflicksytoml = std::filesystem::path(path).parent_path() / L"flicksy.toml";
	lpCFlicksyConfig->Load(gflicksytoml);

	if (CToggleSwitch::RegisterWndClass(hInst)) {
		HWND hSwitch = CToggleSwitch::Create(hWnd, IDC_SWITCH_THEME,
			720, 508, 40, 24,
			lpCAppColorTheme->Colors().windowBg,
			lpCFlicksyConfig->GetTheme() == CFlicksyConfig::Theme::Dark ? true : false);
		ApplyTheme(hWnd);
	}

	if (Gdiplus::GdiplusStartup(&ggdiplusToken, &ggdiplusStartupInput, nullptr) != Gdiplus::Ok)
	{
		return 0L;
	}

	CImgListPng cilp;
	gpBitmapBanner = cilp.LoadPngBitmap(hInst, IDB_PNG1);
	ghImgListBtn = cilp.MakeImageListPng(98, cilp.LoadPngResource(hInst, IDB_PNG2));
	ghImgListBtnHover = cilp.MakeImageListPng(98, cilp.LoadPngResource(hInst, IDB_PNG3));
	ghImgListBtnPressed = cilp.MakeImageListPng(98, cilp.LoadPngResource(hInst, IDB_PNG4));
	ghImgListBtnGrayed = cilp.MakeImageListPng(98, cilp.LoadPngResource(hInst, IDB_PNG5));
	ghImgListBtnPin = cilp.MakeImageListPng(16, cilp.LoadPngResource(hInst, IDB_PNG6));
	gpBitmapBannerDark = cilp.LoadPngBitmap(hInst, IDB_PNG7);

	HDC hdc = GetDC(hWnd);
	ghFontBold = CreateFont(-MulDiv((INT_PTR)14, GetDeviceCaps(hdc, LOGPIXELSY), 72),
		0, 0, 0, FW_BOLD,
		FALSE, FALSE, FALSE,
		DEFAULT_CHARSET,
		OUT_DEFAULT_PRECIS,
		CLIP_DEFAULT_PRECIS,
		CLEARTYPE_NATURAL_QUALITY,
		DEFAULT_PITCH | FF_DONTCARE,
		_T("Segoe UI"));
	ghFont = CreateFont(-MulDiv((INT_PTR)11, GetDeviceCaps(hdc, LOGPIXELSY), 72),
		0, 0, 0, FW_NORMAL,
		FALSE, FALSE, FALSE,
		DEFAULT_CHARSET,
		OUT_DEFAULT_PRECIS,
		CLIP_DEFAULT_PRECIS,
		CLEARTYPE_NATURAL_QUALITY,
		DEFAULT_PITCH | FF_DONTCARE,
		_T("Segoe UI"));
	ReleaseDC(hWnd, hdc);

	HWND hEditPort = CreateWindowEx(
		WS_EX_CLIENTEDGE,
		_T("EDIT"),
		_T("10000"),
		WS_CHILD | WS_VISIBLE | ES_CENTER | ES_NUMBER,
		105, 165, 90, 26,
		hWnd,
		(HMENU)IDC_EDIT_PORT,
		hInst,
		nullptr);
	SendMessage(hEditPort, WM_SETFONT, (WPARAM)ghFont, (LPARAM)TRUE);
	SendMessage(hEditPort, EM_LIMITTEXT, (WPARAM)5, 0L);

	HWND hSpinPort = CreateWindowEx(0,
		UPDOWN_CLASS,
		nullptr,
		WS_CHILD | WS_VISIBLE | UDS_ALIGNRIGHT | UDS_SETBUDDYINT | UDS_ARROWKEYS | UDS_HOTTRACK | UDS_WRAP | UDS_NOTHOUSANDS,
		0, 0, 0, 0,
		hWnd,
		(HMENU)IDC_SPIN_PORT,
		hInst,
		nullptr);
	SendMessage(hSpinPort, UDM_SETBUDDY, (WPARAM)hEditPort, 0);
	SendMessage(hSpinPort, UDM_SETRANGE32, 1, 65535);
	SendMessage(hSpinPort, UDM_SETPOS32, 0, 10000);

	int port = lpCFlicksyConfig->ServerPort();
	SetDlgItemInt(hWnd, IDC_EDIT_PORT, port, FALSE);

	HWND hEdiRoot = CreateWindowEx(
		WS_EX_CLIENTEDGE,
		_T("EDIT"),
		lpCFlicksyConfig->WebRoot().c_str(),
		WS_CHILD | WS_VISIBLE,
		105, 196, 136, 26,
		hWnd,
		(HMENU)IDC_EDIT_ROOT,
		hInst,
		nullptr);
	SendMessage(hEdiRoot, WM_SETFONT, (WPARAM)ghFont, (LPARAM)TRUE);
	SendMessage(hEdiRoot, EM_LIMITTEXT, (WPARAM)MAX_PATH, 0L);

	HWND hBtnStart = CreateWindowEx(0,
		_T("BUTTON"),
		_T("START"),
		WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_OWNERDRAW,
		32, 238, 98, 44,
		hWnd,
		(HMENU)ID_HTTP_START,
		hInst,
		nullptr);
	EnterSubclassButton(hBtnStart);

	HWND hBtnStop = CreateWindowEx(0,
		_T("BUTTON"),
		_T("STOP"),
		WS_CHILD | WS_VISIBLE | WS_DISABLED | BS_PUSHBUTTON | BS_OWNERDRAW,
		142, 238, 98, 44,
		hWnd,
		(HMENU)ID_HTTP_STOP,
		hInst,
		nullptr);
	EnterSubclassButton(hBtnStop);

	HWND hRadio1 = CreateWindowEx(0,
		_T("BUTTON"),
		_T("&SendInput (Default)"),
		WS_CHILD | WS_VISIBLE | WS_GROUP | BS_AUTORADIOBUTTON,
		296, 160, 180, 24,
		hWnd,
		(HMENU)IDC_RADIO_SENDINPUT,
		hInst,
		nullptr);
	SendMessage(hRadio1, WM_SETFONT, (WPARAM)ghFont, (LPARAM)TRUE);

	HWND hRadio2 = CreateWindowEx(0,
		_T("BUTTON"),
		_T("&Clipboard (Paste)"),
		WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
		296, 184, 180, 24,
		hWnd,
		(HMENU)IDC_RADIO_CLIPBOARD,
		hInst,
		nullptr);
	SendMessage(hRadio2, WM_SETFONT, (WPARAM)ghFont, (LPARAM)TRUE);

	CheckRadioButton(hWnd,
		IDC_RADIO_SENDINPUT,
		IDC_RADIO_CLIPBOARD,
		(lpCFlicksyConfig->GetInputMode() == CFlicksyConfig::InputMode::SendInput
			? IDC_RADIO_SENDINPUT : IDC_RADIO_CLIPBOARD));

	HWND hTopMost = CreateWindowEx(0,
		_T("BUTTON"),
		_T("&Always on Top"),
		WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
		750, 11, 16, 16,
		hWnd,
		(HMENU)IDC_CHECK_TOPMOST,
		hInst,
		nullptr);

	HWND hChk1 = CreateWindowEx(0,
		_T("BUTTON"),
		_T("&Start server automatically"),
		WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
		296, 238, 200, 24,
		hWnd,
		(HMENU)IDC_CHECK_AUTOSTART,
		hInst,
		nullptr);
	SendMessage(hChk1, WM_SETFONT, (WPARAM)ghFont, (LPARAM)TRUE);
	CheckDlgButton(hWnd, IDC_CHECK_AUTOSTART, lpCFlicksyConfig->AutoStartServer() ? BST_CHECKED : BST_UNCHECKED);

	HWND hChk2 = CreateWindowEx(0,
		_T("BUTTON"),
		_T("&Run Flicksy at startup"),
		WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
		296, 262, 200, 24,
		hWnd,
		(HMENU)IDC_CHECK_STARTUP,
		hInst,
		nullptr);
	SendMessage(hChk2, WM_SETFONT, (WPARAM)ghFont, (LPARAM)TRUE);
	CheckDlgButton(hWnd, IDC_CHECK_STARTUP, IsRunAtStartup() ? BST_CHECKED : BST_UNCHECKED);

	HWND hList = CreateWindowEx(WS_EX_CLIENTEDGE,
		_T("LISTBOX"),
		nullptr,
		WS_CHILD | WS_VISIBLE | WS_VSCROLL |
		LBS_OWNERDRAWFIXED | LBS_NOINTEGRALHEIGHT | LBS_NOSEL,
		32, 351, 716, 128,
		hWnd,
		(HMENU)IDC_LIST_LOG,
		hInst,
		nullptr);

	return (0L);
}

LRESULT OnPaint(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	PAINTSTRUCT ps;
	BeginPaint(hWnd, &ps);

	RECT rc;
	GetClientRect(hWnd, &rc);

	CAppColorTheme* lpCAppColorTheme = (CAppColorTheme*)GetProp(hWnd, CAPPCOLORTHEME);
	if (!lpCAppColorTheme) {
		EndPaint(hWnd, &ps);
		return 0L;
	}

	HDC hmdc = CreateCompatibleDC(ps.hdc);
	HBITMAP hbmp = CreateCompatibleBitmap(ps.hdc, rc.right, rc.bottom);
	HBITMAP holdbmp = (HBITMAP)SelectObject(hmdc, hbmp);

	DrawBackground(hWnd, hmdc, rc, *lpCAppColorTheme);
	DrawHeader(hWnd, hmdc, rc, *lpCAppColorTheme);
	DrawPanels(hWnd, hmdc, *lpCAppColorTheme);
	DrawServerPanel(hWnd, hmdc, *lpCAppColorTheme);
	DrawOptionPanel(hWnd, hmdc, *lpCAppColorTheme);
	DrawQrPanel(hWnd, hmdc, *lpCAppColorTheme);
	DrawLogPanel(hWnd, hmdc, *lpCAppColorTheme);

	BitBlt(ps.hdc, 0, 0, rc.right, rc.bottom, hmdc, 0, 0, SRCCOPY);

	SelectObject(hmdc, holdbmp);
	DeleteObject(hbmp);
	DeleteDC(hmdc);

	EndPaint(hWnd, &ps);

	return 0L;
}

LRESULT OnMeasureItem(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	LPMEASUREITEMSTRUCT p = (LPMEASUREITEMSTRUCT)lp;
	if (p->CtlID == IDC_LIST_LOG)
	{
		p->itemHeight = 22;
		return TRUE;
	}
	return 0L;
}

LRESULT OnDrawItem(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT)lp;
	switch (lpdis->CtlID)
	{
	case ID_HTTP_START:
	case ID_HTTP_STOP:
		return OnDrawStartStopBtn(hWnd, lpdis);
	case IDC_CHECK_TOPMOST:
		return OnDrawTopmost(hWnd, lpdis);
	case IDC_LIST_LOG:
		return OnDrawListLogs(hWnd, lpdis);
	}
	return (DefWindowProc(hWnd, msg, wp, lp));
}

LRESULT OnDrawStartStopBtn(HWND hWnd, LPDRAWITEMSTRUCT lpdis)
{
	CAppColorTheme* pTheme = (CAppColorTheme*)GetProp(hWnd, CAPPCOLORTHEME);
	if (!pTheme) return 0L;
	FillRect(lpdis->hDC, &(lpdis->rcItem), pTheme->PanelBrush(0));
	int img = (lpdis->CtlID == ID_HTTP_START ? 0 : 1);
	bool hover = GetWindowLongPtr(lpdis->hwndItem,GWLP_USERDATA) != 0;
	if (lpdis->itemState & ODS_DISABLED)
	{
		ImageList_Draw(ghImgListBtnGrayed, img, lpdis->hDC, lpdis->rcItem.left, lpdis->rcItem.top, ILD_TRANSPARENT);
	}
	else {
		if (hover) {
			ImageList_Draw(ghImgListBtnHover, img, lpdis->hDC, lpdis->rcItem.left, lpdis->rcItem.top, ILD_TRANSPARENT);
		}
		else {
			if (lpdis->itemState & ODS_SELECTED) {
				if (!ghImgListBtnPressed) return 0L;
				ImageList_Draw(ghImgListBtnPressed, img, lpdis->hDC, lpdis->rcItem.left, lpdis->rcItem.top, ILD_TRANSPARENT);
			}
			else {
				if (!ghImgListBtn) return 0L;
				ImageList_Draw(ghImgListBtn, img, lpdis->hDC, lpdis->rcItem.left, lpdis->rcItem.top, ILD_TRANSPARENT);
			}

		}
	}
	return 0L;
}

LRESULT OnDrawTopmost(HWND hWnd, LPDRAWITEMSTRUCT lpdis)
{
	CAppColorTheme* pTheme = (CAppColorTheme*)GetProp(hWnd, CAPPCOLORTHEME);
	if (!pTheme) return 0L;
	FillRect(lpdis->hDC, &(lpdis->rcItem), pTheme->HeaderBrush());
	if (!ghImgListBtnPin) return 0L;
	BOOL checked = (BOOL)GetWindowLongPtr(lpdis->hwndItem, GWLP_USERDATA);
	int imageIndex = checked ? 0 : 1;
	ImageList_Draw(ghImgListBtnPin, imageIndex, lpdis->hDC, lpdis->rcItem.left, lpdis->rcItem.top, ILD_TRANSPARENT);
	return 0L;
}

LRESULT OnDrawListLogs(HWND hWnd, LPDRAWITEMSTRUCT lpdis)
{
	CAppColorTheme* pTheme = (CAppColorTheme*)GetProp(hWnd, CAPPCOLORTHEME);
	if (!pTheme) return 0L;

	LPLOGITEM lpli = reinterpret_cast<LPLOGITEM>(lpdis->itemData);
	if (!lpli) return 0L;

	TCHAR text[1024];
	_sntprintf_s(text, _countof(text), _TRUNCATE,
		_T("[%04d/%02d/%02d %02d:%02d:%02d] %s"),
		lpli->st.wYear, lpli->st.wMonth, lpli->st.wDay, 
		lpli->st.wHour, lpli->st.wMinute, lpli->st.wSecond,
		lpli->text.c_str());

	SetBkMode(lpdis->hDC, TRANSPARENT);
	int colOldText = SetTextColor(lpdis->hDC, GetLogColor(lpli->type));
	HFONT hOldFont = (HFONT)SelectObject(lpdis->hDC, ghFont);

	RECT rc = lpdis->rcItem;
	rc.left += 8;

	DrawText(lpdis->hDC,
		text,
		-1,
		&rc,
		DT_SINGLELINE |
		DT_VCENTER |
		DT_NOPREFIX);

	SelectObject(lpdis->hDC, hOldFont);
	SetTextColor(lpdis->hDC, colOldText);

	return TRUE;
}

LRESULT OnCtlColor(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	HDC hdc = (HDC)wp;
	HWND hCtrl = (HWND)lp;
	int id = GetDlgCtrlID(hCtrl);
	CAppColorTheme* pTheme = (CAppColorTheme*)GetProp(hWnd, CAPPCOLORTHEME);
	if (!pTheme) return (DefWindowProc(hWnd, msg, wp, lp));

	switch (id)
	{
	case IDC_RADIO_SENDINPUT:
	case IDC_RADIO_CLIPBOARD:
	case IDC_CHECK_AUTOSTART:
	case IDC_CHECK_STARTUP:
		SetTextColor(hdc, pTheme->Colors().panelText[1]);
		SetBkColor(hdc, pTheme->Colors().panelBg[1]);
		return (LRESULT)pTheme->PanelBrush(1);
	}

	return (DefWindowProc(hWnd, msg, wp, lp));
}

LRESULT OnCommand(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	CSimpleHttpServer* lpCSimpleHttpServer;
	switch (LOWORD(wp))
	{
	case ID_HTTP_START:
		{
			int port = GetDlgItemInt(hWnd, IDC_EDIT_PORT, nullptr, FALSE);
			if (port <= 0 || port > 65535)
			{
				MessageBox(hWnd, _T("Enter a port number from 1 to 65535"), APP_CLASS, MB_OK | MB_ICONWARNING);
				break;
			}
			TCHAR szRoot[MAX_PATH];
			if (GetDlgItemText(hWnd, IDC_EDIT_ROOT, szRoot, _countof(szRoot)) < 1) {
				_tcscpy_s(szRoot, _countof(szRoot), _T("./web"));
			}
			lpCSimpleHttpServer =
				(CSimpleHttpServer*)GetProp(hWnd, CSIMPLEHTTPSERVER);
			if (lpCSimpleHttpServer) {
				lpCSimpleHttpServer->Start(hWnd, port, szRoot);
			}
		}
		break;
	case ID_HTTP_STOP:
		lpCSimpleHttpServer =
			(CSimpleHttpServer*)GetProp(hWnd, CSIMPLEHTTPSERVER);
		if (lpCSimpleHttpServer){
			lpCSimpleHttpServer->Stop();
		}
		break;
	case ID_ABOUT:
		MessageBox(hWnd,
			_T("Flicksy\ncopyright 2026 Pol."),
			_T("About Flicksy"),
			MB_OK | MB_ICONINFORMATION
		);
		break;
	case ID_FILE_QUIT:
		SendMessage(hWnd, WM_CLOSE, 0L, 0L);
		break;
	case IDC_CHECK_TOPMOST:
		{
			HWND hCheck = GetDlgItem(hWnd, IDC_CHECK_TOPMOST);
			BOOL checked = (BOOL)GetWindowLongPtr(hCheck, GWLP_USERDATA);
			checked = !checked;
			SetWindowLongPtr(hCheck, GWLP_USERDATA, checked);
			SetWindowPos(hWnd,
				checked ? HWND_TOPMOST : HWND_NOTOPMOST,
				0, 0, 0, 0,
				SWP_NOMOVE |
				SWP_NOSIZE |
				SWP_NOACTIVATE);
			InvalidateRect(hCheck, nullptr, TRUE);
		}
		break;
	case IDC_SWITCH_THEME:
		ApplyTheme(hWnd);
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
	std::wstring wstr;
	if (lpCSimpleHttpServer->PopMessage(strPop)) {
		std::wstring strPopW = Utf8ToUtf16(strPop);
		if (SendMessage(GetDlgItem(hWnd, IDC_RADIO_SENDINPUT), BM_GETCHECK, 0, 0) == BST_CHECKED) {
			SendUnicodeText(strPopW);
			wstr = L"Send : " + strPopW + L" (SendInput)";
		}
		else {
			SendClipboardText(strPopW);
			wstr = L"Send : " + strPopW + L" (Clipboard)";;
		}
		AddLog(hWnd, LogType::Input, wstr);
	}

	return (0L);
}

LRESULT OnHttpPopKey(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	CSimpleHttpServer* pServer = (CSimpleHttpServer*)GetProp(hWnd, CSIMPLEHTTPSERVER);
	if (!pServer)return 0L;

	std::wstring wstr;
	WORD vk;
	if (pServer->PopKey(vk))
	{
		SendKey(vk);
	}
	return (0L);
}

LRESULT OnHttpState(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	CSimpleHttpServer* pServer = (CSimpleHttpServer*)GetProp(hWnd, CSIMPLEHTTPSERVER);
	if (pServer) {
		std::wstring wstr;
		switch (pServer->GetState())
		{
		case ServerState::Starting:
			wstr = L"Starting HTTP Server ... (PORT " + std::to_wstring(pServer->GetPort()) + L")";
			break;

		case ServerState::Running:
			EnableWindow(GetDlgItem(hWnd, ID_HTTP_START), FALSE);
			EnableWindow(GetDlgItem(hWnd, ID_HTTP_STOP), TRUE);
			wstr = L"HTTP Server Running";
			break;

		case ServerState::Stopping:
			wstr = L"HTTP Server Stopping ...";
			break;

		case ServerState::Stopped:
			EnableWindow(GetDlgItem(hWnd, ID_HTTP_START), TRUE);
			EnableWindow(GetDlgItem(hWnd, ID_HTTP_STOP), FALSE);
			wstr = L"HTTP Server Stopped";
			break;

		case ServerState::Error:
			wstr = L"HTTP Server Error";
			break;

		}
		AddLog(hWnd, LogType::Server, wstr);
	}

	RECT rcStatus = { 19, 115, 230, 296 };
	RECT rcQr = { 526, 96, 764, 296 };

	InvalidateRect(hWnd, &rcStatus, FALSE);
	InvalidateRect(hWnd, &rcQr, FALSE);

	return 0L;
}

LRESULT OnHttpLog(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	CSimpleHttpServer* pServer = (CSimpleHttpServer*)GetProp(hWnd, CSIMPLEHTTPSERVER);
	if (pServer) {
		std::string str;
		pServer->PopLog(str);
		std::wstring wstr = Utf8ToUtf16(str);
		AddLog(hWnd, LogType::Info, wstr);
	}

	return 0L;
}

LRESULT OnTrayIcon(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	UINT traymsg = LOWORD(lp);
	switch (traymsg)
	{
	case WM_LBUTTONDBLCLK:
		ShowWindow(hWnd, SW_RESTORE);
		SetForegroundWindow(hWnd);
		break;
	case WM_CONTEXTMENU:
	case WM_RBUTTONUP:
		{
			CSimpleHttpServer* lpCSimpleHttpServer = (CSimpleHttpServer*)GetProp(hWnd, CSIMPLEHTTPSERVER);
			if (lpCSimpleHttpServer)
			{
				POINT pt;
				GetCursorPos(&pt);
				UINT disabledId = lpCSimpleHttpServer->IsRunning() ? ID_HTTP_START : ID_HTTP_STOP;
				CTrayIcon* lpCTrayIcon = (CTrayIcon*)GetProp(hWnd, CTRAYICON);
				if (lpCTrayIcon) lpCTrayIcon->ShowContextMenu(pt, { disabledId });
			}
			break;
		}
	}
	return 0L;
}

LRESULT OnClose(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	CFlicksyConfig* lpCFlicksyConfig = (CFlicksyConfig*)GetProp(hWnd, CFLICKSYCONFIG);
	if (lpCFlicksyConfig)
	{
		lpCFlicksyConfig->SetServerPort(GetDlgItemInt(hWnd, IDC_EDIT_PORT, nullptr, FALSE));

		TCHAR szRoot[MAX_PATH];
		GetDlgItemText(hWnd, IDC_EDIT_ROOT, szRoot, _countof(szRoot));
		lpCFlicksyConfig->SetWebRoot(szRoot);

		lpCFlicksyConfig->SetInputMode(IsDlgButtonChecked(hWnd, IDC_RADIO_CLIPBOARD) == BST_CHECKED
			? CFlicksyConfig::InputMode::Clipboard : CFlicksyConfig::InputMode::SendInput);

		lpCFlicksyConfig->SetAlwaysOnTop(static_cast<bool>(GetWindowLongPtr(GetDlgItem(hWnd, IDC_CHECK_TOPMOST), GWLP_USERDATA)));

		lpCFlicksyConfig->SetAutoStartServer(IsDlgButtonChecked(hWnd, IDC_CHECK_AUTOSTART) == BST_CHECKED);

		if (IsDlgButtonChecked(hWnd, IDC_CHECK_STARTUP) == BST_CHECKED) {
			SetRunAtStartup(true);
		}
		else {
			SetRunAtStartup(false);
		}

		HWND hSwitch = GetDlgItem(hWnd, IDC_SWITCH_THEME);
		lpCFlicksyConfig->SetTheme(CToggleSwitch::IsChecked(hSwitch)
			? CFlicksyConfig::Theme::Dark : CFlicksyConfig::Theme::Light);

		lpCFlicksyConfig->Save(gflicksytoml);

		RemoveProp(hWnd, CFLICKSYCONFIG);
		delete lpCFlicksyConfig;
	}

	LeaveSubclassButton(GetDlgItem(hWnd, ID_HTTP_START));
	LeaveSubclassButton(GetDlgItem(hWnd, ID_HTTP_STOP));

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

	HWND hList = GetDlgItem(hWnd, IDC_LIST_LOG);
	if (hList)
	{
		int count = (int)SendMessage(hList, LB_GETCOUNT, 0, 0);
		for(int i = 0; i < count; i++)
		{
			LPLOGITEM lpli = reinterpret_cast<LPLOGITEM>(SendMessage(hList, LB_GETITEMDATA, 0, 0));
			if (lpli) delete lpli;
			SendMessage(hList, LB_DELETESTRING, 0, 0);
		}
	}

	CTrayIcon* lpCTrayIcon = (CTrayIcon*)GetProp(hWnd, CTRAYICON);
	if (lpCTrayIcon)
	{
		RemoveProp(hWnd, CTRAYICON);
		delete lpCTrayIcon;
	}

	if (gpBitmapBanner) delete gpBitmapBanner;
	if (ghImgListBtn) ImageList_Destroy(ghImgListBtn);
	if (ghImgListBtnHover) ImageList_Destroy(ghImgListBtnHover);
	if (ghImgListBtnPressed) ImageList_Destroy(ghImgListBtnPressed);
	if (ghImgListBtnGrayed) ImageList_Destroy(ghImgListBtnGrayed);
	if (ghImgListBtnPin) ImageList_Destroy(ghImgListBtnPin);
	if (gpBitmapBannerDark) delete gpBitmapBannerDark;
	if (ghFontBold) DeleteObject(ghFontBold);
	if (ghFont) DeleteObject(ghFont);

	Gdiplus::GdiplusShutdown(ggdiplusToken);

	DestroyWindow(hWnd);

	return (0L);
}

LRESULT OnDestroy(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	PostQuitMessage(0);
	return (0L);
}

void EnterSubclassButton(HWND hButton)
{
	WNDPROC oldProc = reinterpret_cast<WNDPROC>(SetWindowLongPtr(hButton, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(StartStopBtnProc)));
	SetProp(hButton, BUTTONSUBCLASS, reinterpret_cast<HANDLE>(oldProc));
	SetWindowLongPtr(hButton, GWLP_USERDATA, FALSE);
}

void LeaveSubclassButton(HWND hButton)
{
	WNDPROC oldProc = reinterpret_cast<WNDPROC>(GetProp(hButton, BUTTONSUBCLASS));
	if (oldProc) {
		SetWindowLongPtr(hButton, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(oldProc));
	}
}

LRESULT CALLBACK StartStopBtnProc(HWND hBtn, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_MOUSEMOVE:
		{
			bool hover = (GetWindowLongPtr(hBtn, GWLP_USERDATA) != 0);
			if (!hover)
			{
				SetWindowLongPtr(hBtn, GWLP_USERDATA, TRUE);
				TRACKMOUSEEVENT tme{};
				tme.cbSize = sizeof(tme);
				tme.dwFlags = TME_LEAVE;
				tme.hwndTrack = hBtn;
				TrackMouseEvent(&tme);
				InvalidateRect(hBtn, NULL, TRUE);
			}
		}
		break;

	case WM_MOUSELEAVE:
		SetWindowLongPtr(hBtn, GWLP_USERDATA, FALSE);
		InvalidateRect(hBtn, NULL, TRUE);
		break;
	}

	WNDPROC oldProc = reinterpret_cast<WNDPROC>(GetProp(hBtn, BUTTONSUBCLASS));
	return CallWindowProc(oldProc, hBtn, uMsg, wParam, lParam);
}

bool SetRunAtStartup(bool enabled)
{
	HKEY hKey = nullptr;

	if (RegOpenKeyEx(HKEY_CURRENT_USER,
		REG_STARTUP,
		0,
		KEY_SET_VALUE,
		&hKey) != ERROR_SUCCESS)
	{
		return false;
	}

	LONG result;
	if (enabled)
	{
		TCHAR exePath[MAX_PATH] = {};
		GetModuleFileName(
			nullptr,
			exePath,
			_countof(exePath));

		std::basic_string<TCHAR> value = _T("\"");
		value += exePath;
		value += _T("\"");

		result = RegSetValueEx(hKey,
			REG_VALUE,
			0,
			REG_SZ,
			reinterpret_cast<const BYTE*>(value.c_str()),
			static_cast<DWORD>((value.size() + 1) * sizeof(TCHAR)));
	}
	else
	{
		result = RegDeleteValue(hKey,
			REG_VALUE);
		if (result == ERROR_FILE_NOT_FOUND) result = ERROR_SUCCESS;
	}

	RegCloseKey(hKey);

	return result == ERROR_SUCCESS;
}

bool IsRunAtStartup()
{
	HKEY hKey = nullptr;

	if (RegOpenKeyEx(HKEY_CURRENT_USER,
		REG_STARTUP,
		0,
		KEY_QUERY_VALUE,
		&hKey) != ERROR_SUCCESS)
	{
		return false;
	}

	TCHAR exePath[MAX_PATH] = {};
	DWORD type = 0;
	DWORD size = sizeof(exePath);

	LONG result = RegQueryValueEx(hKey,
		REG_VALUE,
		nullptr,
		&type,
		reinterpret_cast<BYTE*>(exePath),
		&size);

	RegCloseKey(hKey);

	if (result != ERROR_SUCCESS || type != REG_SZ)
		return false;

	TCHAR cmpPath[MAX_PATH] = {};
	GetModuleFileName(nullptr, cmpPath, _countof(cmpPath));
	std::basic_string<TCHAR> value = _T("\"");
	value += cmpPath;
	value += _T("\"");

	return _tcscmp(exePath, value.c_str()) == 0;
}

void ApplyTheme(HWND hWnd)
{
	CAppColorTheme* lpCAppColorTheme = (CAppColorTheme*)GetProp(hWnd, CAPPCOLORTHEME);
	if (!lpCAppColorTheme) return;

	HWND hSwitch = GetDlgItem(hWnd, IDC_SWITCH_THEME);
	if (!hSwitch) return;

	bool bDark = CToggleSwitch::IsChecked(hSwitch);

	if (bDark && lpCAppColorTheme->Mode() == ColorMode::Light) {
		lpCAppColorTheme->SetMode(ColorMode::Dark);
	}
	else {
		if (!bDark && lpCAppColorTheme->Mode() == ColorMode::Dark) {
			lpCAppColorTheme->SetMode(ColorMode::Light);
		}
	}
	CToggleSwitch::SetBgColor(hSwitch, lpCAppColorTheme->Colors().windowBg);
	InvalidateRect(hWnd, nullptr, TRUE);
	return;
}

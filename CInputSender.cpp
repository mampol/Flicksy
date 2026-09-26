#include "CInputSender.h"

void CInputSender::SendUnicodeText(const std::wstring& text)
{
	for (wchar_t ch : text)
	{
		SendUnicodeChar(ch);
		if (m_inputInterval > 0) Sleep(m_inputInterval);
	}
}

void CInputSender::SendClipboardText(HWND hWndOwner, const std::wstring& text)
{
	if (text.empty()) return;

	if (!OpenClipboard(hWndOwner)) return;

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

void CInputSender::SendKey(WORD vk)
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

void CInputSender::SendUnicodeChar(wchar_t ch)
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

bool CInputSender::IsExtendedKey(WORD vk) const
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

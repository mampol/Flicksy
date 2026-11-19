#pragma once

#include <Windows.h>
#include <string>
#include <initializer_list>
#include <vector>

#define MAKESENDKEY(l, h)      ((DWORD)MAKELONG(l, h))

class CInputSender
{
public:
    void SetInputInterval(int interval)
    {
        m_inputInterval = interval;
    }

    void SendUnicodeText(const std::wstring& text);
    void SendClipboardText(HWND hWndOwner, const std::wstring& text);
    void SendKey(WORD vk);
    void SendHotkey(std::initializer_list<DWORD> hotkey = {});

    bool IsAltActive() const { return m_altActive; }
    void SetAltActive(bool active) { m_altActive = active; }

private:
    void SendUnicodeChar(wchar_t ch);
    bool IsExtendedKey(WORD vk) const;

private:
    bool m_altActive = false;
    int m_inputInterval = 5;
};

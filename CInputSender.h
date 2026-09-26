#pragma once

#include <Windows.h>
#include <string>

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

private:
    void SendUnicodeChar(wchar_t ch);
    bool IsExtendedKey(WORD vk) const;

private:
    int m_inputInterval = 5;
};

#pragma once

#include <Windows.h>
#include <tchar.h>
#include <string>
#include <initializer_list>

#include "resource.h"

#define UM_TRAYICON         (WM_APP+100)

class CTrayIcon
{
public:
    CTrayIcon() = default;
    ~CTrayIcon();

    bool Create(HWND hWnd, HICON hIcon, UINT callbackMsg);
    void Remove();

    bool IsVisible() { return m_visible; };

    void ShowContextMenu(POINT pt, 
        std::initializer_list<UINT> disabledItems = {});

    bool SetToolTip(const std::wstring& tip);

    bool SetTrayIcon(const HICON hIcon);

    bool ShowBalloon(const std::wstring& title, const std::wstring& message, DWORD iconType);

private:
    HWND m_hWnd = nullptr;
    HICON m_hIcon = nullptr;
    NOTIFYICONDATA m_nid = {};
    bool m_visible = false;
};

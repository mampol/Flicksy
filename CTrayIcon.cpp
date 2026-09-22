#include "CTrayIcon.h"

CTrayIcon::~CTrayIcon()
{
    if (m_hicon) DestroyIcon(m_hicon);
}

bool CTrayIcon::Create(HWND hWnd, HICON hIcon, UINT callbackMsg)
{
    m_hicon = hIcon;
    m_hWnd = hWnd;

    ZeroMemory(&m_nid, sizeof(m_nid));
    m_nid.cbSize = sizeof(NOTIFYICONDATA);
    m_nid.hWnd = m_hWnd;
    m_nid.uID = 1;
    m_nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    m_nid.uCallbackMessage = callbackMsg;
    m_nid.hIcon = m_hicon;
    _tcscpy_s(m_nid.szTip, _T("Flicksy"));
    if (!Shell_NotifyIcon(NIM_ADD, &m_nid)) return false;

    m_nid.uVersion = NOTIFYICON_VERSION_4;
    Shell_NotifyIcon(NIM_SETVERSION, &m_nid);

    m_visible = true;

    return true;
}

void CTrayIcon::Remove()
{
    if (!m_visible) return;

    Shell_NotifyIcon(NIM_DELETE, &m_nid);
    m_visible = false;

    if (m_hicon) {
        DestroyIcon(m_hicon);
        m_hicon = nullptr;
    }
}

void CTrayIcon::ShowContextMenu(POINT pt,
    std::initializer_list<UINT> disabledItems)
{
    HMENU hMenu = LoadMenu((HINSTANCE)GetWindowLongPtr(m_hWnd, GWLP_HINSTANCE), MAKEINTRESOURCE(IDR_MENU1));
    if (hMenu) {
        HMENU hTrayMenu = GetSubMenu(hMenu, 0);
        if (hTrayMenu) {
            for (UINT id : disabledItems)
            {
                EnableMenuItem(
                    hTrayMenu,
                    id,
                    MF_BYCOMMAND | MF_GRAYED);
            }
            SetForegroundWindow(m_hWnd);
            UINT cmd = TrackPopupMenu(hTrayMenu,
                TPM_RIGHTBUTTON | TPM_LEFTALIGN,
                pt.x,
                pt.y,
                0,
                m_hWnd,
                nullptr);
        }
        PostMessage(m_hWnd, WM_NULL, 0, 0);
        DestroyMenu(hMenu);
    }
}

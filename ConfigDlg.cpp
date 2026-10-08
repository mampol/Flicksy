#include <windows.h>
#include "CFlicksyConfig.h"
#include "ConfigDlg.h"
#include "resource.h"

struct ConfigDialogState
{
    HWND hAction = nullptr;
    HWND hMemo = nullptr;
    HWND hTodo = nullptr;
    HWND hSystem = nullptr;

    HWND hCurrent = nullptr;
};

static INT_PTR CALLBACK ConfigDlgProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp);
static INT_PTR CALLBACK ActionDlgProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp);
static INT_PTR CALLBACK MemoDlgProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp);
static INT_PTR CALLBACK TodoDlgProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp);
static INT_PTR CALLBACK SystemDlgProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp);

void ShowConfigDlg(HWND hParent, CFlicksyConfig* lpCFlicksyConfig)
{
    HINSTANCE hInst =
        reinterpret_cast<HINSTANCE>(
            GetWindowLongPtr(hParent, GWLP_HINSTANCE));

    DialogBoxParam(
        hInst,
        MAKEINTRESOURCE(IDD_CONFIG),
        hParent,
        ConfigDlgProc,
        reinterpret_cast<LPARAM>(lpCFlicksyConfig));
}

INT_PTR CALLBACK ConfigDlgProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp)
{
    static CFlicksyConfig* lpCFlicksyConfig = nullptr;
    static ConfigDialogState cds;

    switch (msg)
    {
    case WM_INITDIALOG:
    {
        lpCFlicksyConfig =
            reinterpret_cast<CFlicksyConfig*>(lp);

        HINSTANCE hInst =
            reinterpret_cast<HINSTANCE>(
                GetWindowLongPtr(hDlg, GWLP_HINSTANCE));

        cds.hAction = CreateDialogParam(hInst,
            MAKEINTRESOURCE(IDD_CONFIG_ACTION),
            hDlg,
            ActionDlgProc,
            reinterpret_cast<LPARAM>(lpCFlicksyConfig));

        cds.hMemo = CreateDialogParam(hInst,
            MAKEINTRESOURCE(IDD_CONFIG_MEMO),
            hDlg,
            MemoDlgProc,
            reinterpret_cast<LPARAM>(lpCFlicksyConfig));

        cds.hTodo = CreateDialogParam(hInst,
            MAKEINTRESOURCE(IDD_CONFIG_TODO),
            hDlg,
            TodoDlgProc,
            reinterpret_cast<LPARAM>(lpCFlicksyConfig));

        cds.hSystem = CreateDialogParam(hInst,
            MAKEINTRESOURCE(IDD_CONFIG_SYSTEM),
            hDlg,
            SystemDlgProc,
            reinterpret_cast<LPARAM>(lpCFlicksyConfig));

        ShowWindow(cds.hMemo, SW_HIDE);
        ShowWindow(cds.hTodo, SW_HIDE);
        ShowWindow(cds.hSystem, SW_HIDE);

        ShowWindow(cds.hAction, SW_SHOW);
        cds.hCurrent = cds.hAction;

        return TRUE;
    }
    case WM_COMMAND:
        switch (LOWORD(wp))
        {
        case IDCANCEL:
            EndDialog(hDlg, IDCANCEL);
            return TRUE;
        }
        break;
    }

    return FALSE;
}

INT_PTR ActionDlgProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
    case WM_INITDIALOG:
        return TRUE;
    }

    return FALSE;
}

INT_PTR MemoDlgProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
    case WM_INITDIALOG:
        return TRUE;
    }

    return FALSE;
}

INT_PTR TodoDlgProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
    case WM_INITDIALOG:
        return TRUE;
    }

    return FALSE;
}

INT_PTR SystemDlgProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
    case WM_INITDIALOG:
        return TRUE;
    }

    return FALSE;
}

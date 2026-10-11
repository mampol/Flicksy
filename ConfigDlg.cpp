#include <windows.h>
#include <ShObjIdl.h>
#include <ShlObj.h>
#include <string>
#include "DpiAware.h"
#include "CToggleSwitch.h"
#include "CFlicksyConfig.h"
#include "ConfigDlg.h"
#include "resource.h"

#define IDC_CHK_LOGENABLED          1501

struct ConfigDialogState
{
    HWND hAction = nullptr;
    HWND hMemo = nullptr;
    HWND hTodo = nullptr;
    HWND hSystem = nullptr;
    HWND hSecurity = nullptr;
    HWND hInfo = nullptr;

    HWND hCurrent = nullptr;
};

static INT_PTR CALLBACK ConfigDlgProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp);
static void LayoutControls(HWND hParent, HWND hDlg);
static INT_PTR CALLBACK ActionDlgProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp);
static INT_PTR CALLBACK MemoDlgProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp);
static INT_PTR CALLBACK TodoDlgProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp);
static INT_PTR CALLBACK SecurityDlgProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp);
static INT_PTR CALLBACK SystemDlgProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp);
static INT_PTR CALLBACK InfoDlgProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp);

static bool SelectFile(HWND hParent, std::wstring& file);
static bool SelectFolder(HWND hParent, std::wstring& folder);
static bool SetInitialFolder(IFileOpenDialog* pDialog, const std::wstring& folderPath);

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
        LayoutControls(hDlg, cds.hAction);

        cds.hMemo = CreateDialogParam(hInst,
            MAKEINTRESOURCE(IDD_CONFIG_MEMO),
            hDlg,
            MemoDlgProc,
            reinterpret_cast<LPARAM>(lpCFlicksyConfig));
        LayoutControls(hDlg, cds.hMemo);

        cds.hTodo = CreateDialogParam(hInst,
            MAKEINTRESOURCE(IDD_CONFIG_TODO),
            hDlg,
            TodoDlgProc,
            reinterpret_cast<LPARAM>(lpCFlicksyConfig));
        LayoutControls(hDlg, cds.hTodo);

        cds.hSecurity = CreateDialogParam(hInst,
            MAKEINTRESOURCE(IDD_CONFIG_SECURITY),
            hDlg,
            SecurityDlgProc,
            reinterpret_cast<LPARAM>(lpCFlicksyConfig));
        LayoutControls(hDlg, cds.hSecurity);

        cds.hSystem = CreateDialogParam(hInst,
            MAKEINTRESOURCE(IDD_CONFIG_SYSTEM),
            hDlg,
            SystemDlgProc,
            reinterpret_cast<LPARAM>(lpCFlicksyConfig));
        LayoutControls(hDlg, cds.hSystem);

        cds.hInfo= CreateDialogParam(hInst,
            MAKEINTRESOURCE(IDD_CONFIG_INFO),
            hDlg,
            InfoDlgProc,
            reinterpret_cast<LPARAM>(lpCFlicksyConfig));
        LayoutControls(hDlg, cds.hInfo);

        ShowWindow(cds.hAction, SW_SHOW);
        ShowWindow(cds.hMemo, SW_HIDE);
        ShowWindow(cds.hTodo, SW_HIDE);
        ShowWindow(cds.hSecurity, SW_HIDE);
        ShowWindow(cds.hSystem, SW_HIDE);
        ShowWindow(cds.hInfo, SW_HIDE);
        cds.hCurrent = cds.hAction;

        return TRUE;
    }
    case WM_COMMAND:
        switch (LOWORD(wp))
        {
        case IDD_BTN_ACTION:
            ShowWindow(cds.hAction, SW_SHOW);
            ShowWindow(cds.hMemo, SW_HIDE);
            ShowWindow(cds.hTodo, SW_HIDE);
            ShowWindow(cds.hSecurity, SW_HIDE);
            ShowWindow(cds.hSystem, SW_HIDE);
            ShowWindow(cds.hInfo, SW_HIDE);
            cds.hCurrent = cds.hAction;
            break;
        case IDD_BTN_MEMO:
            ShowWindow(cds.hAction, SW_HIDE);
            ShowWindow(cds.hMemo, SW_SHOW);
            ShowWindow(cds.hTodo, SW_HIDE);
            ShowWindow(cds.hSecurity, SW_HIDE);
            ShowWindow(cds.hSystem, SW_HIDE);
            ShowWindow(cds.hInfo, SW_HIDE);
            cds.hCurrent = cds.hMemo;
            break;
        case IDD_BTN_TODO:
            ShowWindow(cds.hAction, SW_HIDE);
            ShowWindow(cds.hMemo, SW_HIDE);
            ShowWindow(cds.hTodo, SW_SHOW);
            ShowWindow(cds.hSecurity, SW_HIDE);
            ShowWindow(cds.hSystem, SW_HIDE);
            ShowWindow(cds.hInfo, SW_HIDE);
            cds.hCurrent = cds.hTodo;
            break;
        case IDD_BTN_SYSTEM:
            ShowWindow(cds.hAction, SW_HIDE);
            ShowWindow(cds.hMemo, SW_HIDE);
            ShowWindow(cds.hTodo, SW_HIDE);
            ShowWindow(cds.hSecurity, SW_HIDE);
            ShowWindow(cds.hSystem, SW_SHOW);
            ShowWindow(cds.hInfo, SW_HIDE);
            cds.hCurrent = cds.hSecurity;
            break;
        case IDD_BTN_SECURITY:
            ShowWindow(cds.hAction, SW_HIDE);
            ShowWindow(cds.hMemo, SW_HIDE);
            ShowWindow(cds.hTodo, SW_HIDE);
            ShowWindow(cds.hSecurity, SW_SHOW);
            ShowWindow(cds.hSystem, SW_HIDE);
            ShowWindow(cds.hInfo, SW_HIDE);
            cds.hCurrent = cds.hSystem;
            break;
        case IDD_BTN_INFO:
            ShowWindow(cds.hAction, SW_HIDE);
            ShowWindow(cds.hMemo, SW_HIDE);
            ShowWindow(cds.hTodo, SW_HIDE);
            ShowWindow(cds.hSecurity, SW_HIDE);
            ShowWindow(cds.hSystem, SW_HIDE);
            ShowWindow(cds.hInfo, SW_SHOW);
            cds.hCurrent = cds.hInfo;
            break;
        case IDCANCEL:
            EndDialog(hDlg, IDCANCEL);
            return TRUE;
        }
        break;
    }

    return FALSE;
}

void LayoutControls(HWND hParent, HWND hDlg)
{
    RECT rc;
    GetClientRect(hParent, &rc);
    MoveWindow(hDlg,
        150,
        10,
        rc.right - 160,
        rc.bottom - 20,
        TRUE);
}

INT_PTR ActionDlgProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp)
{
    static CFlicksyConfig* lpCFlicksyConfig = nullptr;
    switch (msg)
    {
    case WM_INITDIALOG:
        lpCFlicksyConfig =
            reinterpret_cast<CFlicksyConfig*>(lp);
        return TRUE;
    }

    return FALSE;
}

INT_PTR MemoDlgProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp)
{
    static CFlicksyConfig* lpCFlicksyConfig = nullptr;
    switch (msg)
    {
    case WM_INITDIALOG:
        lpCFlicksyConfig =
            reinterpret_cast<CFlicksyConfig*>(lp);
        return TRUE;
    }

    return FALSE;
}

INT_PTR TodoDlgProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp)
{
    static CFlicksyConfig* lpCFlicksyConfig = nullptr;
    switch (msg)
    {
    case WM_INITDIALOG:
        lpCFlicksyConfig =
            reinterpret_cast<CFlicksyConfig*>(lp);
        return TRUE;
    }

    return FALSE;
}

INT_PTR SecurityDlgProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp)
{
    static CFlicksyConfig* lpCFlicksyConfig = nullptr;
    switch (msg)
    {
    case WM_INITDIALOG:
        lpCFlicksyConfig =
            reinterpret_cast<CFlicksyConfig*>(lp);
        return TRUE;
    }

    return FALSE;
}

INT_PTR SystemDlgProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp)
{
    static CFlicksyConfig* lpCFlicksyConfig = nullptr;
    switch (msg)
    {
    case WM_INITDIALOG:
        lpCFlicksyConfig =
            reinterpret_cast<CFlicksyConfig*>(lp);
        SetDlgItemText(hDlg, IDC_EDIT_WEBROOT, (lpCFlicksyConfig->WebRoot()).c_str());
        {
            HINSTANCE hInst = reinterpret_cast<HINSTANCE>(GetWindowLongPtr(hDlg, GWLP_HINSTANCE));
            if (CToggleSwitch::RegisterWndClass(hInst)) {
                HWND hSwitchNormal = CToggleSwitch::Create(hDlg,
                    IDC_CHK_LOGENABLED,
                    DpiScale(hDlg, 180),
                    DpiScale(hDlg, 86),
                    DpiScale(hDlg, 40),
                    DpiScale(hDlg, 23),
                    GetSysColor(COLOR_BTNFACE),
                    lpCFlicksyConfig->GetLogEnable());
            }
        }
        SetDlgItemText(hDlg, IDC_EDIT_LOGFILE, (lpCFlicksyConfig->GetLogFile()).c_str());
        return TRUE;
    case WM_COMMAND:
        switch (LOWORD(wp)) {
        case IDC_BTN_WEBROOT:
            {
                wchar_t szFolder[MAX_PATH] = {};
                GetDlgItemText(hDlg, IDC_EDIT_WEBROOT, szFolder, _countof(szFolder));
                std::wstring folder = szFolder;
                if(SelectFolder(hDlg, folder))
                    SetDlgItemText(hDlg, IDC_EDIT_WEBROOT, folder.c_str());
                return TRUE;
            }
            break;
        case IDC_BTN_LOGFILE:
            {
                wchar_t szFile[MAX_PATH] = {};
                GetDlgItemText(hDlg, IDC_EDIT_LOGFILE, szFile, _countof(szFile));
                std::wstring file = szFile;
                if (SelectFile(hDlg, file))
                    SetDlgItemText(hDlg, IDC_EDIT_LOGFILE, file.c_str());
                return TRUE;
            }
            break;
        case IDC_BTN_DATAFOLDER:
            {
                wchar_t szFolder[MAX_PATH] = {};
                GetDlgItemText(hDlg, IDC_EDIT_DATAFOLDER, szFolder, _countof(szFolder));
                std::wstring folder = szFolder;
                if (SelectFolder(hDlg, folder))
                    SetDlgItemText(hDlg, IDC_EDIT_DATAFOLDER, folder.c_str());
                return TRUE;
            }
            break;
        }
    }

    return FALSE;
}

INT_PTR InfoDlgProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp)
{
    static CFlicksyConfig* lpCFlicksyConfig = nullptr;
    switch (msg)
    {
    case WM_INITDIALOG:
        lpCFlicksyConfig =
            reinterpret_cast<CFlicksyConfig*>(lp);
        return TRUE;
    }

    return FALSE;
}

bool SelectFile(HWND hParent, std::wstring& file)
{
    IFileSaveDialog* pDialog = nullptr;

    HRESULT hr = CoCreateInstance(
        CLSID_FileSaveDialog,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&pDialog));
    if (FAILED(hr)) 
        return false;

    pDialog->SetFileName(file.c_str());
    pDialog->SetDefaultExtension(L"log");
    COMDLG_FILTERSPEC filters[] =
    {
        { L"Log files", L"*.log" },
        { L"All files", L"*.*" }
    };
    pDialog->SetFileTypes(_countof(filters), filters);

    DWORD options = 0;

    hr = pDialog->GetOptions(&options);
    if (SUCCEEDED(hr))
    {
        hr = pDialog->SetOptions(
            options |
            FOS_FILEMUSTEXIST |
            FOS_FORCEFILESYSTEM);
    }

    if (SUCCEEDED(hr))
        hr = pDialog->Show(hParent);

    if (SUCCEEDED(hr))
    {
        IShellItem* pItem = nullptr;

        hr = pDialog->GetResult(&pItem);

        if (SUCCEEDED(hr))
        {
            PWSTR pszPath = nullptr;

            hr = pItem->GetDisplayName(
                SIGDN_FILESYSPATH,
                &pszPath);

            if (SUCCEEDED(hr))
            {
                file = pszPath;
                CoTaskMemFree(pszPath);
            }

            pItem->Release();
        }
    }

    pDialog->Release();

    return SUCCEEDED(hr);
}

bool SelectFolder(HWND hParent, std::wstring& folder)
{
    IFileOpenDialog* pDialog = nullptr;

    HRESULT hr = CoCreateInstance(
        CLSID_FileOpenDialog,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&pDialog));
    if (FAILED(hr))
        return false;

    SetInitialFolder(pDialog, folder);

    DWORD options = 0;

    hr = pDialog->GetOptions(&options);
    if (SUCCEEDED(hr))
    {
        hr = pDialog->SetOptions(
            options |
            FOS_PICKFOLDERS |
            FOS_FORCEFILESYSTEM |
            FOS_PATHMUSTEXIST);
    }

    if (SUCCEEDED(hr))
        hr = pDialog->Show(hParent);

    if (SUCCEEDED(hr))
    {
        IShellItem* pItem = nullptr;

        hr = pDialog->GetResult(&pItem);

        if (SUCCEEDED(hr))
        {
            PWSTR pszPath = nullptr;

            hr = pItem->GetDisplayName(
                SIGDN_FILESYSPATH,
                &pszPath);

            if (SUCCEEDED(hr))
            {
                folder = pszPath;
                CoTaskMemFree(pszPath);
            }

            pItem->Release();
        }
    }

    pDialog->Release();

    return SUCCEEDED(hr);
}

bool SetInitialFolder(IFileOpenDialog* pDialog, const std::wstring& folderPath)
{
    IShellItem* pFolder = nullptr;

    HRESULT hr = SHCreateItemFromParsingName(
        folderPath.c_str(),
        nullptr,
        IID_PPV_ARGS(&pFolder));

    if (FAILED(hr))
        return false;

    hr = pDialog->SetFolder(pFolder);

    pFolder->Release();

    return SUCCEEDED(hr);
}
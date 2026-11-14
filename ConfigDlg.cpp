#include <windows.h>
#include <ShObjIdl.h>
#include <ShlObj.h>
#include <string>
#include "DpiAware.h"
#include "CToggleSwitch.h"
#include "ConfigDlg.h"
#include "CPngResource.h"
#include "GetFileVersion.h"
#include "resource.h"

static Gdiplus::Bitmap* gBmpBanner = nullptr;
static Gdiplus::Bitmap* gBmpIcon = nullptr;
static HFONT ghFontBold = nullptr;
static HFONT ghFont = nullptr;

struct ConfigDialogState
{
    HWND hAction = nullptr;
    HWND hMemo = nullptr;
    HWND hSystem = nullptr;
    HWND hSecurity = nullptr;
    HWND hInfo = nullptr;

    HWND hCurrent = nullptr;
    UINT uCurrentId = 0;
};

struct MemoEditContext
{
    ConfigContext* configContext = nullptr;
    size_t memoIndex = static_cast<size_t>(-1);
    bool isNew = true;
};

static bool SelectFile(HWND hParent, std::wstring& file);
static bool SelectFolder(HWND hParent, std::wstring& folder);
static bool SetInitialFolder(IFileOpenDialog* pDialog, const std::wstring& folderPath);

static INT_PTR CALLBACK ConfigDlgProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp);
static LRESULT OnPaintConfigDlg(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp, CAppColorTheme& appTheme);
static void LayoutControls(HWND hParent, HWND hDlg);
static LRESULT OnDrawItemConfigDlg(HWND hDlg, ConfigDialogState& cds, LPDRAWITEMSTRUCT dis, CAppColorTheme& apptheme);

static LRESULT OnPaintConfigPage(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp, CAppColorTheme& appTheme, UINT pageId);
static LRESULT OnCtlColorConfigPage(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp, CAppColorTheme& appTheme);

static INT_PTR CALLBACK ActionDlgProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp);
static const wchar_t* ActionTypeToDisplayString(ActionType type);
static const ActionType GetSelActionType(HWND hCombo);
static void ShowActionEditor(HWND hDlg, HWND hList, int sel, CFlicksyConfig* config);
static void HideActionEditor(HWND hDlg, HWND hList);
static void SaveActionEditor(HWND hDlg, HWND hList, int sel, CFlicksyConfig* config);
static void ReloadActionList(HWND hDlg, CFlicksyConfig* config);

static INT_PTR CALLBACK MemoDlgProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp);
static int AddMemoItem(HWND hListView, const size_t memoIndex, const struct MemoItem& memo);
static void ReloadMemoList(HWND hListView, CMemoManager* memoManager);

static INT_PTR CALLBACK MemoEditDlgProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp);

static INT_PTR CALLBACK SecurityDlgProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp);

static INT_PTR CALLBACK SystemDlgProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp);

static INT_PTR CALLBACK InfoDlgProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp);

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
        pDialog->SetOptions(
            options |
            FOS_FORCEFILESYSTEM |
            FOS_PATHMUSTEXIST |
            FOS_OVERWRITEPROMPT);
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

void ShowConfigDlg(HWND hParent, ConfigContext* lpConfigContext)
{
    HINSTANCE hInst =
        reinterpret_cast<HINSTANCE>(
            GetWindowLongPtr(hParent, GWLP_HINSTANCE));

    CPngResource png;
    gBmpBanner = png.LoadPngBitmap(hInst, IDB_PNG1);
    gBmpIcon = png.LoadPngBitmap(hInst, IDB_PNG8);

    HDC hdc = GetDC(hParent);
    ghFontBold = CreateFont(-MulDiv((INT_PTR)14, GetDeviceCaps(hdc, LOGPIXELSY), 72),
        0, 0, 0, FW_BOLD,
        FALSE, FALSE, FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        CLEARTYPE_NATURAL_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE,
        _T("Segoe UI"));
    ghFont = CreateFont(-MulDiv((INT_PTR)9, GetDeviceCaps(hdc, LOGPIXELSY), 72),
        0, 0, 0, FW_NORMAL,
        FALSE, FALSE, FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        CLEARTYPE_NATURAL_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE,
        _T("Segoe UI"));
    ReleaseDC(hParent, hdc);

    DialogBoxParam(
        hInst,
        MAKEINTRESOURCE(IDD_CONFIG),
        hParent,
        ConfigDlgProc,
        reinterpret_cast<LPARAM>(lpConfigContext));

    delete gBmpBanner;
    gBmpBanner = nullptr;
    delete gBmpIcon;
    gBmpIcon = nullptr;

    DeleteObject(ghFontBold);
    DeleteObject(ghFont);
}

INT_PTR CALLBACK ConfigDlgProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp)
{
    static ConfigContext* lpConfigContext = nullptr;
    static ConfigDialogState cds;

    switch (msg)
    {
    case WM_INITDIALOG:
    {
        lpConfigContext =
            reinterpret_cast<ConfigContext*>(lp);
        if (!lpConfigContext ||
            !lpConfigContext->config ||
            !lpConfigContext->memoManager ||
            !lpConfigContext->appColorTheme)
        {
            return FALSE;
        }

        HINSTANCE hInst =
            reinterpret_cast<HINSTANCE>(
                GetWindowLongPtr(hDlg, GWLP_HINSTANCE));

        cds.hSystem = CreateDialogParam(hInst,
            MAKEINTRESOURCE(IDD_CONFIG_SYSTEM),
            hDlg,
            SystemDlgProc,
            reinterpret_cast<LPARAM>(lpConfigContext));
        LayoutControls(hDlg, cds.hSystem);

        cds.hAction = CreateDialogParam(hInst,
            MAKEINTRESOURCE(IDD_CONFIG_ACTION),
            hDlg,
            ActionDlgProc,
            reinterpret_cast<LPARAM>(lpConfigContext));
        LayoutControls(hDlg, cds.hAction);

        cds.hMemo = CreateDialogParam(hInst,
            MAKEINTRESOURCE(IDD_CONFIG_MEMO),
            hDlg,
            MemoDlgProc,
            reinterpret_cast<LPARAM>(lpConfigContext));
        LayoutControls(hDlg, cds.hMemo);

        cds.hSecurity = CreateDialogParam(hInst,
            MAKEINTRESOURCE(IDD_CONFIG_SECURITY),
            hDlg,
            SecurityDlgProc,
            reinterpret_cast<LPARAM>(lpConfigContext));
        LayoutControls(hDlg, cds.hSecurity);

        cds.hInfo= CreateDialogParam(hInst,
            MAKEINTRESOURCE(IDD_CONFIG_INFO),
            hDlg,
            InfoDlgProc,
            reinterpret_cast<LPARAM>(lpConfigContext));
        LayoutControls(hDlg, cds.hInfo);

        ShowWindow(cds.hSystem, SW_SHOW);
        ShowWindow(cds.hAction, SW_HIDE);
        ShowWindow(cds.hMemo, SW_HIDE);
        ShowWindow(cds.hSecurity, SW_HIDE);
        ShowWindow(cds.hInfo, SW_HIDE);
        cds.hCurrent = cds.hSystem;
        cds.uCurrentId = IDD_BTN_SYSTEM;

        return TRUE;
    }
    case WM_COMMAND:
        switch (LOWORD(wp))
        {
        case IDD_BTN_SYSTEM:
            ShowWindow(cds.hAction, SW_HIDE);
            ShowWindow(cds.hMemo, SW_HIDE);
            ShowWindow(cds.hSecurity, SW_HIDE);
            ShowWindow(cds.hSystem, SW_SHOW);
            ShowWindow(cds.hInfo, SW_HIDE);
            cds.hCurrent = cds.hSystem;
            cds.uCurrentId = LOWORD(wp);
            InvalidateRect(hDlg, NULL, TRUE);
            break;
        case IDD_BTN_ACTION:
            ShowWindow(cds.hAction, SW_SHOW);
            ShowWindow(cds.hMemo, SW_HIDE);
            ShowWindow(cds.hSecurity, SW_HIDE);
            ShowWindow(cds.hSystem, SW_HIDE);
            ShowWindow(cds.hInfo, SW_HIDE);
            cds.hCurrent = cds.hAction;
            cds.uCurrentId = LOWORD(wp);
            InvalidateRect(hDlg, NULL, TRUE);
            break;
        case IDD_BTN_MEMO:
            ReloadMemoList(
                GetDlgItem(cds.hMemo, IDC_LIST_MEMO),
                lpConfigContext->memoManager);
            ShowWindow(cds.hAction, SW_HIDE);
            ShowWindow(cds.hMemo, SW_SHOW);
            ShowWindow(cds.hSecurity, SW_HIDE);
            ShowWindow(cds.hSystem, SW_HIDE);
            ShowWindow(cds.hInfo, SW_HIDE);
            cds.hCurrent = cds.hMemo;
            cds.uCurrentId = LOWORD(wp);
            InvalidateRect(hDlg, NULL, TRUE);
            break;
        case IDD_BTN_SECURITY:
            ShowWindow(cds.hAction, SW_HIDE);
            ShowWindow(cds.hMemo, SW_HIDE);
            ShowWindow(cds.hSecurity, SW_SHOW);
            ShowWindow(cds.hSystem, SW_HIDE);
            ShowWindow(cds.hInfo, SW_HIDE);
            cds.hCurrent = cds.hSecurity;
            cds.uCurrentId = LOWORD(wp);
            InvalidateRect(hDlg, NULL, TRUE);
            break;
        case IDD_BTN_INFO:
            ShowWindow(cds.hAction, SW_HIDE);
            ShowWindow(cds.hMemo, SW_HIDE);
            ShowWindow(cds.hSecurity, SW_HIDE);
            ShowWindow(cds.hSystem, SW_HIDE);
            ShowWindow(cds.hInfo, SW_SHOW);
            cds.hCurrent = cds.hInfo;
            cds.uCurrentId = LOWORD(wp);
            InvalidateRect(hDlg, NULL, TRUE);
            break;
        case IDCANCEL:
            EndDialog(hDlg, IDCANCEL);
            return TRUE;
        }
        break;

    case WM_DRAWITEM:
        return OnDrawItemConfigDlg(hDlg, cds, reinterpret_cast<LPDRAWITEMSTRUCT>(lp), *(lpConfigContext->appColorTheme));

    case WM_PAINT:
        return OnPaintConfigDlg(hDlg, msg, wp, lp, *(lpConfigContext->appColorTheme));
    }

    return FALSE;
}

LRESULT OnPaintConfigDlg(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp, CAppColorTheme& appTheme)
{
    PAINTSTRUCT ps;
    BeginPaint(hDlg, &ps);

    CPngResource png;
    Gdiplus::Graphics g(ps.hdc);
    png.DrawPng(g, gBmpBanner,
        DpiScale(hDlg, 10),
        DpiScale(hDlg, 10),
        DpiScale(hDlg, 132),
        DpiScale(hDlg, 52));

    EndPaint(hDlg, &ps);

    return TRUE;
}

void LayoutControls(HWND hParent, HWND hDlg)
{
    RECT rc;
    GetClientRect(hParent, &rc);
    MoveWindow(hDlg,
        DpiScale(hDlg, 150),
        DpiScale(hDlg, 10),
        rc.right - DpiScale(hDlg, 160),
        rc.bottom - DpiScale(hDlg, 20),
        TRUE);
}

LRESULT OnDrawItemConfigDlg(HWND hDlg, ConfigDialogState& cds, LPDRAWITEMSTRUCT dis, CAppColorTheme& apptheme)
{
    COLORREF colBtn = apptheme.Colors().buttonBg;
    COLORREF colText = apptheme.Colors().buttonText;
    if (cds.uCurrentId == dis->CtlID)
    {
        colBtn = apptheme.Colors().buttonSelBg;
        colText = apptheme.Colors().buttonSelText;;
    }

    HBRUSH hBrush = CreateSolidBrush(colBtn);
    HPEN hPen = CreatePen(PS_SOLID, 0, colBtn);
    HBRUSH oldBrush = static_cast<HBRUSH>(SelectObject(dis->hDC, hBrush));
    HPEN oldPen = static_cast<HPEN>(SelectObject(dis->hDC, hPen));
    int padding = DpiScale(dis->hwndItem, 12);
    int radius = DpiScale(dis->hwndItem, 8);
    int accent = DpiScale(dis->hwndItem, 4);
    RECT rc = dis->rcItem;
    RoundRect(
        dis->hDC,
        rc.left,
        rc.top,
        rc.right,
        rc.bottom,
        radius,
        radius);
    SelectObject(dis->hDC, oldBrush);
    SelectObject(dis->hDC, oldPen);
    DeleteObject(hBrush);
    DeleteObject(hPen);

    CPngResource png;
    int iconSize = DpiScale(hDlg, 38);
    Gdiplus::Graphics g(dis->hDC);
    int iconIndex = 0;

    switch (dis->CtlID)
    {
    case IDD_BTN_SYSTEM:   iconIndex = 0; break;
    case IDD_BTN_ACTION:   iconIndex = 1; break;
    case IDD_BTN_MEMO:     iconIndex = 2; break;
    case IDD_BTN_SECURITY: iconIndex = 3; break;
    case IDD_BTN_INFO:     iconIndex = 4; break;
    }

    bool selected =
        cds.uCurrentId == dis->CtlID;

    COLORREF iconColor =
        selected
        ? RGB(255, 255, 255)
        : RGB(55, 70, 90);

    png.DrawSpriteIcon(
        g,
        gBmpIcon,
        iconIndex,
        dis->rcItem.left,
        dis->rcItem.top,
        iconSize,
        128,
        iconColor);

    HFONT hFont = reinterpret_cast<HFONT>(SendMessage(dis->hwndItem, WM_GETFONT, 0, 0));
    HFONT hOldFont = nullptr;
    if (hFont)
    {
        hOldFont = static_cast<HFONT>(SelectObject(dis->hDC, hFont));
    }
    RECT rcText = rc;
    rcText.left += DpiScale(hDlg, 40);
    rcText.right -= DpiScale(hDlg, 4);
    wchar_t btnText[64];
    GetWindowText(dis->hwndItem, btnText, _countof(btnText));
    SetTextColor(dis->hDC, colText);
    SetBkColor(dis->hDC, colBtn);
    DrawText(dis->hDC, btnText, -1, &rcText, DT_SINGLELINE | DT_VCENTER);
    if (hFont)
    {
        SelectObject(dis->hDC, hOldFont);
    }

    return TRUE;
}

LRESULT OnPaintConfigPage(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp, CAppColorTheme& appTheme, UINT pageId)
{
    PAINTSTRUCT ps;
    BeginPaint(hDlg, &ps);

    HBRUSH hOldBrush = reinterpret_cast<HBRUSH>(SelectObject(ps.hdc, appTheme.PanelBrush(1)));
    HPEN hOldPen = reinterpret_cast<HPEN>(SelectObject(ps.hdc, appTheme.PanelPen(1)));

    RECT rc{};
    GetClientRect(hDlg, &rc);

    RoundRect(ps.hdc, rc.left + 2, rc.top + 2, rc.right - 2, rc.bottom - 2, 8, 8);

    std::wstring ver;
    const wchar_t* title = L"";
    const wchar_t* description = L"";
    bool japanese =
        PRIMARYLANGID(GetUserDefaultUILanguage()) == LANG_JAPANESE;
    switch (pageId)
    {
    case IDD_CONFIG_SYSTEM:
        title = japanese ? L"SYSTEM" : L"System";
        description = japanese
            ? L"Flicksy の基本動作や保存先を設定します。"
            : L"Configure Flicksy's basic behavior and storage settings.";
        break;

    case IDD_CONFIG_ACTION:
        title = japanese ? L"ACTION" : L"Action";
        description = japanese
            ? L"スマートフォンから実行する操作を登録・編集します。"
            : L"Create and edit actions that can be triggered from your smartphone.";
        break;

    case IDD_CONFIG_MEMO:
        title = japanese ? L"MEMO" : L"Memo";
        description = japanese
            ? L"スマートフォンから送信されたメモや添付データを管理します。"
            : L"Manage memos and attachments sent from your smartphone.";
        break;

    case IDD_CONFIG_SECURITY:
        title = japanese ? L"SECURITY" : L"Security";
        description = japanese
            ? L"Flicksy への接続や認証に関する設定を管理します。"
            : L"Manage connection and authentication settings for Flicksy.";
        break;

    case IDD_CONFIG_INFO:
        title = japanese ? L"INFOMATION" : L"Infomation";
        wchar_t path[MAX_PATH]{};
        GetModuleFileName(
            nullptr,
            path,
            MAX_PATH);
        ver = L"Flicksy Version " + GetFileVersion(path) + L"\nCopyright (c) 2026 Pol.";
        description = ver.c_str();
        break;
    }

    HFONT hOldFont = static_cast<HFONT>(SelectObject(ps.hdc, ghFontBold));

    SetBkMode(ps.hdc, TRANSPARENT);
    SetTextColor(ps.hdc, appTheme.Colors().panelText[1]);

    RECT rcTitle{
        DpiScale(hDlg, 20),
        DpiScale(hDlg, 16),
        rc.right - DpiScale(hDlg, 20),
        DpiScale(hDlg, 44)
    };

    DrawText(
        ps.hdc,
        title,
        -1,
        &rcTitle,
        DT_LEFT | DT_SINGLELINE);

    static_cast<HFONT>(SelectObject(ps.hdc, ghFont));

    RECT rcDesc{
        DpiScale(hDlg, 20),
        DpiScale(hDlg, 44),
        rc.right - DpiScale(hDlg, 20),
        DpiScale(hDlg, 96)
    };

    DrawText(
        ps.hdc,
        description,
        -1,
        &rcDesc,
        DT_LEFT | DT_WORDBREAK);

    EndPaint(hDlg, &ps);

    SelectObject(ps.hdc, reinterpret_cast<HBRUSH>(hOldFont));
    SelectObject(ps.hdc, reinterpret_cast<HPEN>(hOldPen));
    SelectObject(ps.hdc, reinterpret_cast<HBRUSH>(hOldBrush));

    return TRUE;
}

LRESULT OnCtlColorConfigPage(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp, CAppColorTheme& appTheme)
{
    HDC hdc = (HDC)wp;
    HWND hCtrl = (HWND)lp;
    int id = GetDlgCtrlID(hCtrl);

    SetTextColor(hdc, appTheme.Colors().panelText[1]);
    SetBkColor(hdc, appTheme.Colors().panelBg[1]);

    return (LRESULT)appTheme.PanelBrush(1);
}

INT_PTR ActionDlgProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp)
{
    static ConfigContext* lpConfigContext = nullptr;
    static int s_prevSel = LB_ERR;
    switch (msg)
    {
    case WM_INITDIALOG:
        {
            lpConfigContext =
                reinterpret_cast<ConfigContext*>(lp);
            if (!lpConfigContext ||
                !lpConfigContext->config ||
                !lpConfigContext->memoManager ||
                !lpConfigContext->appColorTheme)
            {
                return FALSE;
            }

            s_prevSel = LB_ERR;

            SendDlgItemMessage(hDlg, IDC_CHK_ENABLE,
                TSM_SETBGCOLOR, 0, static_cast<LPARAM>(GetSysColor(COLOR_WINDOW)));

            SendDlgItemMessage(
                hDlg,
                IDC_CMB_TYPE,
                CB_ADDSTRING,
                0,
                reinterpret_cast<LPARAM>(ActionTypeToDisplayString(ActionType::Url)));
            SendDlgItemMessage(
                hDlg,
                IDC_CMB_TYPE,
                CB_ADDSTRING,
                0,
                reinterpret_cast<LPARAM>(ActionTypeToDisplayString(ActionType::Application)));
            SendDlgItemMessage(
                hDlg,
                IDC_CMB_TYPE,
                CB_ADDSTRING,
                0,
                reinterpret_cast<LPARAM>(ActionTypeToDisplayString(ActionType::Folder)));
            SendDlgItemMessage(
                hDlg,
                IDC_CMB_TYPE,
                CB_ADDSTRING,
                0,
                reinterpret_cast<LPARAM>(ActionTypeToDisplayString(ActionType::Lock)));
            SendDlgItemMessage(
                hDlg,
                IDC_CMB_TYPE,
                CB_ADDSTRING,
                0,
                reinterpret_cast<LPARAM>(ActionTypeToDisplayString(ActionType::Shutdown)));
            SendDlgItemMessage(
                hDlg,
                IDC_CMB_TYPE,
                CB_ADDSTRING,
                0,
                reinterpret_cast<LPARAM>(ActionTypeToDisplayString(ActionType::None)));

            HWND hList = GetDlgItem(hDlg, IDC_LIST_ACTION);

            LONG_PTR style = GetWindowLongPtr(hList, GWL_STYLE);

            SetWindowLongPtr(
                hList,
                GWL_STYLE,
                style | WS_CLIPSIBLINGS);

            const size_t size = lpConfigContext->config->GetActionSize();
            for (size_t index = 0; index < size; index++) {
                int n = static_cast<int>(SendDlgItemMessage(
                    hDlg, IDC_LIST_ACTION, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"")));
                SendDlgItemMessage(
                    hDlg, IDC_LIST_ACTION, LB_SETITEMDATA, n, index);
            }
        }
        break;

    case WM_PAINT:
        return OnPaintConfigPage(hDlg, msg, wp, lp, *(lpConfigContext->appColorTheme), IDD_CONFIG_ACTION);

    case WM_CTLCOLORSTATIC:
        return OnCtlColorConfigPage(hDlg, msg, wp, lp, *(lpConfigContext->appColorTheme));

    case WM_MEASUREITEM:
        {
            auto* mis =
                reinterpret_cast<MEASUREITEMSTRUCT*>(lp);
            mis->itemHeight = DpiScale(hDlg, 40);
        }
        break;

    case WM_DRAWITEM:
        {
            auto* dis =
                reinterpret_cast<DRAWITEMSTRUCT*>(lp);

            if (dis->CtlID != IDC_LIST_ACTION)
                return FALSE;

            if (dis->itemID == static_cast<UINT>(-1))
                return TRUE;

            FillRect(dis->hDC, &(dis->rcItem), GetSysColorBrush(COLOR_WINDOW));

            if (dis->itemState & ODS_SELECTED)
                return TRUE;

            const size_t actionIndex =
                static_cast<size_t>(
                    SendMessage(
                        dis->hwndItem,
                        LB_GETITEMDATA,
                        dis->itemID,
                        0));

            const ActionConfig& action =
                lpConfigContext->config->GetAction(actionIndex);

            std::wstring wstraction = (action.name).c_str() + 
                lpConfigContext->config->ActionTypeToString(action.type) +
                (action.value).c_str();
            DrawText(dis->hDC,
                wstraction.c_str(),
                static_cast<int>(wstraction.size()),
                &(dis->rcItem),
                DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);

        }
        break;

    case WM_COMMAND:
        {
            switch (LOWORD(wp))
            {
            case IDC_BTN_ADD:
                {
                    HWND hList = GetDlgItem(hDlg, IDC_LIST_ACTION);

                    if (s_prevSel != LB_ERR)
                    {
                        SaveActionEditor(
                            hDlg,
                            hList,
                            s_prevSel,
                            lpConfigContext->config);
                    }

                    ActionConfig action{
                        ActionType::None,
                        false,
                        L"NewAction",
                        L""
                    };

                    size_t actionIndex =
                        lpConfigContext->config->AddAction(action);

                    lpConfigContext->config->Save();

                    ReloadActionList(hDlg, lpConfigContext->config);

                    int newSel =
                        static_cast<int>(actionIndex);

                    SendMessage(
                        hList,
                        LB_SETCURSEL,
                        newSel,
                        0);

                    ShowActionEditor(
                        hDlg,
                        hList,
                        newSel,
                        lpConfigContext->config);

                    s_prevSel = newSel;

                    return TRUE;
                }
                break;

            case IDC_BTN_DELETE:
                {
                    HWND hList = GetDlgItem(hDlg, IDC_LIST_ACTION);

                    int sel = static_cast<int>(
                        SendMessage(hList, LB_GETCURSEL, 0, 0));

                    if (sel == LB_ERR)
                        break;

                    if (s_prevSel != LB_ERR)
                    {
                        SaveActionEditor(
                            hDlg,
                            hList,
                            s_prevSel,
                            lpConfigContext->config);
                    }

                    size_t actionIndex =
                        static_cast<size_t>(
                            SendMessage(
                                hList,
                                LB_GETITEMDATA,
                                sel,
                                0));

                    lpConfigContext->config->DeleteAction(actionIndex);

                    lpConfigContext->config->Save();

                    ReloadActionList(hDlg, lpConfigContext->config);

                    size_t count = lpConfigContext->config->GetActionSize();

                    if (count > 0)
                    {
                        int newSel =
                            (sel >= static_cast<int>(count))
                            ? static_cast<int>(count) - 1
                            : sel;

                        SendMessage(
                            hList,
                            LB_SETCURSEL,
                            newSel,
                            0);

                        ShowActionEditor(
                            hDlg,
                            hList,
                            newSel,
                            lpConfigContext->config);

                        s_prevSel = newSel;
                    }
                    else
                    {
                        s_prevSel = LB_ERR;

                        HideActionEditor(hDlg, hList);
                    }

                    return TRUE;
                }
                break;

            case IDC_LIST_ACTION:
                if (HIWORD(wp) == LBN_SELCHANGE) {
                    HWND hList = reinterpret_cast<HWND>(lp);

                    int sel = static_cast<int>(SendMessage(hList, LB_GETCURSEL, 0, 0));

                    if (s_prevSel != LB_ERR)
                    {
                        SaveActionEditor(
                            hDlg,
                            hList,
                            s_prevSel,
                            lpConfigContext->config);
                    }

                    if (sel != LB_ERR)
                    {
                        ShowActionEditor(hDlg, hList, sel, lpConfigContext->config);
                    }

                    s_prevSel = sel;

                    return TRUE;
                }
                break;

            case IDC_CMB_TYPE:
                if (HIWORD(wp) == CBN_SELCHANGE) {
                    HWND hCombo = reinterpret_cast<HWND>(lp);

                    switch (GetSelActionType(hCombo)) {
                    case ActionType::Url:
                        EnableWindow(GetDlgItem(hDlg, IDC_EDIT_VALUE), TRUE);
                        EnableWindow(GetDlgItem(hDlg, IDC_BTN_VALUE), FALSE);
                        break;

                    case ActionType::Folder:
                    case ActionType::Application:
                        EnableWindow(GetDlgItem(hDlg, IDC_EDIT_VALUE), TRUE);
                        EnableWindow(GetDlgItem(hDlg, IDC_BTN_VALUE), TRUE);
                        break;

                    case ActionType::Lock:
                    case ActionType::Shutdown:
                    case ActionType::None:
                        EnableWindow(GetDlgItem(hDlg, IDC_EDIT_VALUE), FALSE);
                        EnableWindow(GetDlgItem(hDlg, IDC_BTN_VALUE), FALSE);
                        break;
                    }
                }

                return TRUE;

            case IDC_BTN_VALUE:
                HWND hCombo = reinterpret_cast<HWND>(GetDlgItem(hDlg, IDC_CMB_TYPE));

                switch (GetSelActionType(hCombo)) {
                case ActionType::Folder:
                    {
                        wchar_t buf[MAX_PATH] = {};
                        GetDlgItemText(hDlg, IDC_EDIT_VALUE, buf, _countof(buf));
                        std::wstring value = buf;
                        if (SelectFolder(hDlg, value)) {
                            SetDlgItemText(hDlg, IDC_EDIT_VALUE, value.c_str());
                        }
                    }
                    break;

                case ActionType::Application:
                    {
                        wchar_t buf[MAX_PATH] = {};
                        GetDlgItemText(hDlg, IDC_EDIT_VALUE, buf, _countof(buf));
                        std::wstring value = buf;
                        if (SelectFile(hDlg, value)) {
                            SetDlgItemText(hDlg, IDC_EDIT_VALUE, value.c_str());
                        }
                    }
                    break;

                }

                break;
            }
        }
        break;

    case WM_CLOSE:
    case WM_SHOWWINDOW:
        if (wp == FALSE)
        {
            HWND hList = GetDlgItem(hDlg, IDC_LIST_ACTION);

            if (s_prevSel != LB_ERR)
            {
                SaveActionEditor(
                    hDlg,
                    hList,
                    s_prevSel,
                    lpConfigContext->config);
            }

            HideActionEditor(hDlg, hList);
            SendMessage(hList, LB_SETCURSEL, static_cast<WPARAM>(-1), 0);
            s_prevSel = LB_ERR;
        }
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

static const wchar_t* ActionTypeToDisplayString(ActionType type)
{
    switch (type)
    {
    case ActionType::Url:
        return L"Open URL";

    case ActionType::Application:
        return L"Execute App";

    case ActionType::Folder:
        return L"Open Folder";

    case ActionType::Lock:
        return L"Windows Lock";

    case ActionType::Shutdown:
        return L"Shutdown";

    default:
        return L"None";
    }
}

static const ActionType GetSelActionType(HWND hCombo)
{
    int sel = static_cast<int>(SendMessage(hCombo, CB_GETCURSEL, 0, 0));

    switch (sel)
    {
    case 0:
        return ActionType::Url;

    case 1:
        return ActionType::Application;

    case 2:
        return ActionType::Folder;

    case 3:
        return ActionType::Lock;

    case 4:
        return ActionType::Shutdown;
    }

    return ActionType::None;
}

static void ShowActionEditor(
    HWND hDlg,
    HWND hList,
    int sel,
    CFlicksyConfig* config)
{
    RECT rc{};

    if (SendMessage(
        hList,
        LB_GETITEMRECT,
        sel,
        reinterpret_cast<LPARAM>(&rc)) == LB_ERR)
    {
        return;
    }

    MapWindowPoints(
        hList,
        hDlg,
        reinterpret_cast<POINT*>(&rc),
        2);

    int nCtlLeft = 10;
    RECT rcCtl{};
    GetWindowRect(GetDlgItem(hDlg, IDC_CHK_ENABLE), &rcCtl);
    SetWindowPos(
        GetDlgItem(hDlg, IDC_CHK_ENABLE),
        HWND_TOP,
        rc.left + DpiScale(hDlg, nCtlLeft),
        rc.top + (((rc.bottom - rc.top) - (rcCtl.bottom - rcCtl.top)) / 2),
        rcCtl.right- rcCtl.left,
        rcCtl.bottom - rcCtl.top,
        SWP_SHOWWINDOW | SWP_NOACTIVATE);
    nCtlLeft += (rcCtl.right - rcCtl.left) + DpiScale(hDlg, 6);

    GetWindowRect(GetDlgItem(hDlg, IDC_EDIT_NAME), &rcCtl);
    SetWindowPos(
        GetDlgItem(hDlg, IDC_EDIT_NAME),
        HWND_TOP,
        rc.left + nCtlLeft,
        rc.top + (((rc.bottom - rc.top) - (rcCtl.bottom - rcCtl.top)) / 2),
        rcCtl.right - rcCtl.left,
        rcCtl.bottom - rcCtl.top,
        SWP_SHOWWINDOW | SWP_NOACTIVATE);
    nCtlLeft += (rcCtl.right - rcCtl.left) + DpiScale(hDlg, 6);

    GetWindowRect(GetDlgItem(hDlg, IDC_CMB_TYPE), &rcCtl);
    SetWindowPos(
        GetDlgItem(hDlg, IDC_CMB_TYPE),
        HWND_TOP,
        rc.left + nCtlLeft,
        rc.top + (((rc.bottom - rc.top) - (rcCtl.bottom - rcCtl.top)) / 2),
        rcCtl.right - rcCtl.left,
        DpiScale(hDlg, 100),
        SWP_SHOWWINDOW | SWP_NOACTIVATE);
    nCtlLeft += (rcCtl.right - rcCtl.left) + DpiScale(hDlg, 6);

    GetWindowRect(GetDlgItem(hDlg, IDC_EDIT_VALUE), &rcCtl);
    SetWindowPos(
        GetDlgItem(hDlg, IDC_EDIT_VALUE),
        HWND_TOP,
        rc.left + nCtlLeft,
        rc.top + (((rc.bottom - rc.top) - (rcCtl.bottom - rcCtl.top)) / 2),
        rcCtl.right - rcCtl.left,
        rcCtl.bottom - rcCtl.top,
        SWP_SHOWWINDOW | SWP_NOACTIVATE);
    nCtlLeft += (rcCtl.right - rcCtl.left) + DpiScale(hDlg, 6);

    GetWindowRect(GetDlgItem(hDlg, IDC_BTN_VALUE), &rcCtl);
    SetWindowPos(
        GetDlgItem(hDlg, IDC_BTN_VALUE),
        HWND_TOP,
        rc.left + nCtlLeft,
        rc.top + (((rc.bottom - rc.top) - (rcCtl.bottom - rcCtl.top)) / 2),
        rcCtl.right - rcCtl.left,
        rcCtl.bottom - rcCtl.top,
        SWP_SHOWWINDOW | SWP_NOACTIVATE);
    nCtlLeft += (rcCtl.right - rcCtl.left) + DpiScale(hDlg, 6);

    const auto actionIndex =
        static_cast<size_t>(
            SendMessage(hList, LB_GETITEMDATA, sel, 0));

    const ActionConfig& action =
        config->GetAction(actionIndex);

    CheckDlgButton(hDlg, IDC_CHK_ENABLE, (action.enabled ? BST_CHECKED : BST_UNCHECKED));
    SetDlgItemText(hDlg, IDC_EDIT_NAME, (action.name).c_str());
    SendDlgItemMessage(hDlg, IDC_CMB_TYPE, CB_SETCURSEL, static_cast<WPARAM>(action.type), 0);
    switch (action.type) {
    case ActionType::Url:
        EnableWindow(GetDlgItem(hDlg, IDC_EDIT_VALUE), TRUE);
        EnableWindow(GetDlgItem(hDlg, IDC_BTN_VALUE), FALSE);
        break;

    case ActionType::Folder:
    case ActionType::Application:
        EnableWindow(GetDlgItem(hDlg, IDC_EDIT_VALUE), TRUE);
        EnableWindow(GetDlgItem(hDlg, IDC_BTN_VALUE), TRUE);
        break;

    case ActionType::Lock:
    case ActionType::Shutdown:
    case ActionType::None:
        EnableWindow(GetDlgItem(hDlg, IDC_EDIT_VALUE), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_BTN_VALUE), FALSE);
        break;
    }
    SetDlgItemText(hDlg, IDC_EDIT_VALUE, (action.value).c_str());

}

static void HideActionEditor(
    HWND hDlg,
    HWND hList)
{
    ShowWindow(GetDlgItem(hDlg, IDC_CHK_ENABLE), SW_HIDE);

    ShowWindow(GetDlgItem(hDlg, IDC_EDIT_NAME), SW_HIDE);

    ShowWindow(GetDlgItem(hDlg, IDC_CMB_TYPE), SW_HIDE);

    ShowWindow(GetDlgItem(hDlg, IDC_EDIT_VALUE), SW_HIDE);

    ShowWindow(GetDlgItem(hDlg, IDC_BTN_VALUE), SW_HIDE);
}

static void SaveActionEditor(
    HWND hDlg,
    HWND hList,
    int sel,
    CFlicksyConfig* config)
{
    if (sel == LB_ERR)
        return;

    size_t actionIndex =
        static_cast<size_t>(
            SendMessage(
                hList,
                LB_GETITEMDATA,
                sel,
                0));

    ActionConfig action = config->GetAction(actionIndex);

    wchar_t buf[512]{};

    action.type = static_cast<ActionType>(SendDlgItemMessage(
        hDlg,
        IDC_CMB_TYPE,
        CB_GETCURSEL,
        0, 0));

    action.enabled = static_cast<bool>(IsDlgButtonChecked(
        hDlg,
        IDC_CHK_ENABLE));

    GetDlgItemText(
        hDlg,
        IDC_EDIT_NAME,
        buf,
        _countof(buf));
    action.name = buf;

    GetDlgItemText(
        hDlg,
        IDC_EDIT_VALUE,
        buf,
        _countof(buf));
    action.value = buf;

    config->SetAction(actionIndex, action);

    config->Save();
}

static void ReloadActionList(
    HWND hDlg,
    CFlicksyConfig* config)
{
    HWND hList = GetDlgItem(hDlg, IDC_LIST_ACTION);

    SendMessage(hList, LB_RESETCONTENT, 0, 0);

    const size_t count = config->GetActionSize();

    for (size_t i = 0; i < count; ++i)
    {
        int item = static_cast<int>(
            SendMessage(
                hList,
                LB_ADDSTRING,
                0,
                reinterpret_cast<LPARAM>(L"")));

        if (item != LB_ERR && item != LB_ERRSPACE)
        {
            SendMessage(
                hList,
                LB_SETITEMDATA,
                item,
                static_cast<LPARAM>(i));
        }
    }

    InvalidateRect(hList, nullptr, TRUE);
}

INT_PTR MemoDlgProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp)
{
    static ConfigContext* lpConfigContext = nullptr;
    switch (msg)
    {
    case WM_INITDIALOG:
        {
            lpConfigContext =
                reinterpret_cast<ConfigContext*>(lp);
            if (!lpConfigContext ||
                !lpConfigContext->config ||
                !lpConfigContext->memoManager ||
                !lpConfigContext->appColorTheme)
            {
                return FALSE;
            }

            HWND hListView = GetDlgItem(hDlg, IDC_LIST_MEMO);

            LVCOLUMN col{};
            col.mask = LVCF_TEXT | LVCF_WIDTH;

            col.cx = 160;
            col.pszText = const_cast<LPWSTR>(L"Date");
            ListView_InsertColumn(hListView, 0, &col);

            col.cx = 500;
            col.pszText = const_cast<LPWSTR>(L"Memo");
            ListView_InsertColumn(hListView, 1, &col);

            for (size_t i = 0;
                i < lpConfigContext->memoManager->GetMemoCount();
                ++i) {
                AddMemoItem(
                    hListView,
                    i,
                    lpConfigContext->memoManager->GetMemo(i));
            }

            return TRUE;
        }
        break;

    case WM_PAINT:
        return OnPaintConfigPage(hDlg, msg, wp, lp, *(lpConfigContext->appColorTheme), IDD_CONFIG_MEMO);

    case WM_CTLCOLORSTATIC:
        return OnCtlColorConfigPage(hDlg, msg, wp, lp, *(lpConfigContext->appColorTheme));

    case WM_COMMAND:

        switch (LOWORD(wp)) {
        case IDC_BTN_ADDMEMO:
            {
                HINSTANCE hInst =
                    reinterpret_cast<HINSTANCE>(
                        GetWindowLongPtr(hDlg, GWLP_HINSTANCE));

                HWND hListView = GetDlgItem(hDlg, IDC_LIST_MEMO);

                MemoEditContext ctx;
                ctx.configContext = lpConfigContext;
                ctx.isNew = true;

                int ret = DialogBoxParam(
                    hInst,
                    MAKEINTRESOURCE(IDD_MEMO_EDIT),
                    hDlg,
                    MemoEditDlgProc,
                    reinterpret_cast<LPARAM>(&ctx));

                if (ret == IDOK)
                {
                    ReloadMemoList(
                        hListView,
                        lpConfigContext->memoManager);
                }
            }
            break;

        case IDC_BTN_EDITMEMO:
            {
                HINSTANCE hInst =
                    reinterpret_cast<HINSTANCE>(
                        GetWindowLongPtr(hDlg, GWLP_HINSTANCE));

                HWND hListView = GetDlgItem(hDlg, IDC_LIST_MEMO);

                int sel = ListView_GetNextItem(
                    hListView,
                    -1,
                    LVNI_SELECTED);

                if (sel == -1)
                    break;

                LVITEM item{};
                item.mask = LVIF_PARAM;
                item.iItem = sel;
                item.iSubItem = 0;

                if (!ListView_GetItem(hListView, &item))
                    break;

                MemoEditContext ctx;
                ctx.configContext = lpConfigContext;
                ctx.memoIndex = static_cast<size_t>(item.lParam);
                ctx.isNew = false;

                int ret = DialogBoxParam(
                    hInst,
                    MAKEINTRESOURCE(IDD_MEMO_EDIT),
                    hDlg,
                    MemoEditDlgProc,
                    reinterpret_cast<LPARAM>(&ctx));

                if (ret == IDOK)
                {
                    ReloadMemoList(
                        hListView,
                        lpConfigContext->memoManager);
                }

            }
            break;

        case IDC_BTN_DELMEMO:
            {
                HWND hListView = GetDlgItem(hDlg, IDC_LIST_MEMO);

                int sel = ListView_GetNextItem(
                    hListView,
                    -1,
                    LVNI_SELECTED);

                if (sel == -1)
                    break;

                LVITEM item{};
                item.mask = LVIF_PARAM;
                item.iItem = sel;
                item.iSubItem = 0;

                if (!ListView_GetItem(hListView, &item))
                    break;

                size_t memoIndex =
                    static_cast<size_t>(item.lParam);

                if (MessageBox(
                    hDlg,
                    L"Delete the selected memo ?",
                    L"Flicksy",
                    MB_YESNO | MB_ICONQUESTION) != IDYES)
                {
                    break;
                }

                lpConfigContext->memoManager
                    ->DeleteMemo(memoIndex);

                std::wstring dataFolder = lpConfigContext->config->GetDataFolder();
                if (!lpConfigContext->memoManager->Save(dataFolder))
                {
                    lpConfigContext->memoManager->Load(dataFolder);

                    MessageBox(
                        hDlg,
                        L"Failed to save the memo.",
                        L"Flicksy",
                        MB_OK | MB_ICONERROR);

                    break;
                }

                ReloadMemoList(
                    hListView,
                    lpConfigContext->memoManager);

                return TRUE;
            }
            break;

        }
        break;

    default:
        return FALSE;
    }
    return TRUE;
}

static int AddMemoItem(
    HWND hListView,
    const size_t memoIndex,
    const struct MemoItem& memo)
{
    int row = ListView_GetItemCount(hListView);

    LVITEM item{};
    item.mask = LVIF_TEXT | LVIF_PARAM;
    item.iItem = row;
    item.iSubItem = 0;
    item.pszText = const_cast<LPWSTR>(memo.created.c_str());
    item.lParam = static_cast<LPARAM>(memoIndex);

    int index = ListView_InsertItem(hListView, &item);

    ListView_SetItemText(
        hListView,
        index,
        1,
        const_cast<LPWSTR>(memo.text.c_str()));

    return index;
}

static void ReloadMemoList(
    HWND hListView,
    CMemoManager* memoManager)
{
    ListView_DeleteAllItems(hListView);

    for (size_t i = 0;
        i < memoManager->GetMemoCount();
        ++i)
    {
        AddMemoItem(
            hListView,
            i,
            memoManager->GetMemo(i));
    }
}

INT_PTR MemoEditDlgProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg) {
    case WM_INITDIALOG:
        {
            auto* ctx =
                reinterpret_cast<MemoEditContext*>(lp);

            SetWindowLongPtr(
                hDlg,
                GWLP_USERDATA,
                reinterpret_cast<LONG_PTR>(ctx));

            if (ctx->isNew)
            {
                SetDlgItemText(
                    hDlg,
                    IDC_EDIT_MEMO_DATE,
                    L"(New Memo)");
            }
            else{
                const MemoItem& memo =
                    ctx->configContext->memoManager
                    ->GetMemo(ctx->memoIndex);

                SetDlgItemText(
                    hDlg,
                    IDC_EDIT_MEMO_TEXT,
                    memo.text.c_str());

                SetDlgItemText(
                    hDlg,
                    IDC_EDIT_MEMO_DATE,
                    memo.created.c_str());
            }

            return TRUE;
        }
        break;

    case WM_COMMAND:

        switch (LOWORD(wp)) {
        case IDOK:
            {
                auto* ctx =
                    reinterpret_cast<MemoEditContext*>(
                        GetWindowLongPtr(hDlg, GWLP_USERDATA));

                wchar_t buf[8192]{};

                GetDlgItemText(
                    hDlg,
                    IDC_EDIT_MEMO_TEXT,
                    buf,
                    _countof(buf));

                std::wstring text = buf;

                if (ctx->isNew)
                {
                    MemoItem memo;
                    
                    memo.id = ctx->configContext->memoManager->MakeMemoId();
                    memo.text = text;
                    memo.created = ctx->configContext->memoManager->GetCurrentDateTimeString();
                    memo.updated = memo.created;

                    ctx->configContext->memoManager
                        ->AddMemo(memo);
                }
                else
                {
                    MemoItem memo =
                        ctx->configContext->memoManager
                        ->GetMemo(ctx->memoIndex);

                    memo.text = text;
                    memo.updated = ctx->configContext->memoManager->GetCurrentDateTimeString();

                    ctx->configContext->memoManager
                        ->SetMemo(ctx->memoIndex, memo);
                }

                if (!ctx->configContext->memoManager->Save(
                    ctx->configContext->config->GetDataFolder()))
                {
                    MessageBox(
                        hDlg,
                        L"Failed to save the memo.",
                        L"Flicksy",
                        MB_OK | MB_ICONERROR);

                    return TRUE;
                }

                EndDialog(hDlg, IDOK);
                return TRUE;
            }
            break;

        case IDCANCEL:
            EndDialog(hDlg, IDCANCEL);
            return TRUE;

        default:
            return FALSE;
        }
        break;

    default:
        return FALSE;
    }
    return TRUE;
}

INT_PTR SecurityDlgProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp)
{
    static ConfigContext* lpConfigContext = nullptr;
    switch (msg)
    {
    case WM_INITDIALOG:
        {
            lpConfigContext =
                reinterpret_cast<ConfigContext*>(lp);

            return TRUE;
        }
    case WM_PAINT:
        return OnPaintConfigPage(hDlg, msg, wp, lp, *(lpConfigContext->appColorTheme), IDD_CONFIG_SECURITY);

    case WM_CTLCOLORSTATIC:
        return OnCtlColorConfigPage(hDlg, msg, wp, lp, *(lpConfigContext->appColorTheme));

    default:
        return FALSE;
    }
    return TRUE;
}

INT_PTR SystemDlgProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp)
{
    static ConfigContext* lpConfigContext = nullptr;
    switch (msg)
    {
    case WM_INITDIALOG:
        {
            lpConfigContext =
                reinterpret_cast<ConfigContext*>(lp);

            if (!lpConfigContext ||
                !lpConfigContext->config ||
                !lpConfigContext->memoManager)
            {
                return FALSE;
            }

            SetDlgItemInt(hDlg, IDC_EDIT_PORT, lpConfigContext->config->ServerPort(), FALSE);
            SetDlgItemText(hDlg, IDC_EDIT_WEBROOT, (lpConfigContext->config->WebRoot()).c_str());
            if (lpConfigContext->config->GetLogEnable()) {
                SendDlgItemMessage(hDlg, IDC_CHK_LOGENABLED, BM_SETCHECK, static_cast<WPARAM>(BST_CHECKED), 0);
            }
            if (lpConfigContext->config->AutoStartServer()) {
                SendDlgItemMessage(hDlg, IDC_CHK_AUTOSTART, BM_SETCHECK, static_cast<WPARAM>(BST_CHECKED), 0);
            }
            if (IsRunAtStartup()) {
                SendDlgItemMessage(hDlg, IDC_CHK_STARTUP, BM_SETCHECK, static_cast<WPARAM>(BST_CHECKED), 0);
            }
            SetDlgItemText(hDlg, IDC_EDIT_LOGFILE, (lpConfigContext->config->GetLogFile()).c_str());
            SetDlgItemText(hDlg, IDC_EDIT_DATAFOLDER, (lpConfigContext->config->GetDataFolder()).c_str());

            SendDlgItemMessage(hDlg, IDC_CHK_LOGENABLED, TSM_SETBGCOLOR, 0, static_cast<LPARAM>(lpConfigContext->appColorTheme->Colors().panelBg[1]));
            SendDlgItemMessage(hDlg, IDC_CHK_AUTOSTART, TSM_SETBGCOLOR, 0, static_cast<LPARAM>(lpConfigContext->appColorTheme->Colors().panelBg[1]));
            SendDlgItemMessage(hDlg, IDC_CHK_STARTUP, TSM_SETBGCOLOR, 0, static_cast<LPARAM>(lpConfigContext->appColorTheme->Colors().panelBg[1]));
        }
        return TRUE;

    case WM_PAINT:
        return OnPaintConfigPage(hDlg, msg, wp, lp, *(lpConfigContext->appColorTheme), IDD_CONFIG_SYSTEM);

    case WM_CTLCOLORSTATIC:
        return OnCtlColorConfigPage(hDlg, msg, wp, lp, *(lpConfigContext->appColorTheme));

    case WM_COMMAND:
        switch (LOWORD(wp)) {
        case IDC_EDIT_PORT:
            {
                if (HIWORD(wp) == EN_KILLFOCUS) {
                    lpConfigContext->config->SetServerPort(GetDlgItemInt(hDlg, IDC_EDIT_PORT, NULL, FALSE));
                    lpConfigContext->config->Save();
                }
            }
            break;
        case IDC_EDIT_WEBROOT:
            {
                wchar_t buf[MAX_PATH] = {};
                if (HIWORD(wp) == EN_KILLFOCUS) {
                    GetDlgItemText(hDlg, IDC_EDIT_WEBROOT, buf, _countof(buf));
                    std::wstring webroot = buf;
                    lpConfigContext->config->SetWebRoot(webroot);
                    lpConfigContext->config->Save();
                }
            }
            break;
        case IDC_BTN_WEBROOT:
            {
                wchar_t buf[MAX_PATH] = {};
                GetDlgItemText(hDlg, IDC_EDIT_WEBROOT, buf, _countof(buf));
                std::wstring webroot = buf;
                if (SelectFolder(hDlg, webroot)) {
                    SetDlgItemText(hDlg, IDC_EDIT_WEBROOT, webroot.c_str());
                    SetFocus(GetDlgItem(hDlg, IDC_EDIT_WEBROOT));
                }
            }
            break;
        case IDC_CHK_LOGENABLED:
            if (HIWORD(wp) == BN_CLICKED)
            {
                HWND hSwitch = reinterpret_cast<HWND>(lp);
                lpConfigContext->config->SetLogEnable(CToggleSwitch::IsChecked(hSwitch));
                lpConfigContext->config->Save();
            }
            break;
        case IDC_EDIT_LOGFILE:
            {
                wchar_t buf[MAX_PATH] = {};
                if (HIWORD(wp) == EN_KILLFOCUS) {
                    GetDlgItemText(hDlg, IDC_EDIT_LOGFILE, buf, _countof(buf));
                    std::wstring logfile = buf;
                    lpConfigContext->config->SetLogFile(logfile);
                    lpConfigContext->config->Save();
                }
            }
            break;
        case IDC_BTN_LOGFILE:
            {
                wchar_t buf[MAX_PATH] = {};
                GetDlgItemText(hDlg, IDC_EDIT_LOGFILE, buf, _countof(buf));
                std::wstring logfile = buf;
                if (SelectFile(hDlg, logfile)) {
                    SetDlgItemText(hDlg, IDC_EDIT_LOGFILE, logfile.c_str());
                    SetFocus(GetDlgItem(hDlg, IDC_EDIT_LOGFILE));
                }
            }
            break;
        case IDC_EDIT_DATAFOLDER:
            {
                wchar_t buf[MAX_PATH] = {};
                if (HIWORD(wp) == EN_KILLFOCUS) {
                    GetDlgItemText(hDlg, IDC_EDIT_DATAFOLDER, buf, _countof(buf));
                    std::wstring datafolder = buf;
                    lpConfigContext->config->SetDataFolder(datafolder);
                    lpConfigContext->config->Save();

                    lpConfigContext->memoManager->Load(lpConfigContext->config->GetDataFolder());
                }
            }
            break;
        case IDC_BTN_DATAFOLDER:
            {
                wchar_t szFolder[MAX_PATH] = {};
                GetDlgItemText(hDlg, IDC_EDIT_DATAFOLDER, szFolder, _countof(szFolder));
                std::wstring folder = szFolder;
                if (SelectFolder(hDlg, folder)) {
                    SetDlgItemText(hDlg, IDC_EDIT_DATAFOLDER, folder.c_str());
                    SetFocus(GetDlgItem(hDlg, IDC_EDIT_DATAFOLDER));
                }
            }
            break;
        case IDC_CHK_AUTOSTART:
            if (HIWORD(wp) == BN_CLICKED)
            {
                HWND hSwitch = reinterpret_cast<HWND>(lp);
                lpConfigContext->config->SetAutoStartServer(CToggleSwitch::IsChecked(hSwitch));
                lpConfigContext->config->Save();
            }
            break;
        case IDC_CHK_STARTUP:
            if (HIWORD(wp) == BN_CLICKED)
            {
                HWND hSwitch = reinterpret_cast<HWND>(lp);
                if (CToggleSwitch::IsChecked(hSwitch)) {
                    SetRunAtStartup(true);
                }
                else {
                    SetRunAtStartup(false);
                }
            }
            break;
        default:
            return FALSE;
        }
        break;

    default:
        return FALSE;
    }
    return TRUE;
}

INT_PTR InfoDlgProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp)
{
    static ConfigContext* lpConfigContext = nullptr;
    switch (msg)
    {
    case WM_INITDIALOG:
        {
            lpConfigContext =
                reinterpret_cast<ConfigContext*>(lp);

            return TRUE;
        }

    case WM_PAINT:
        return OnPaintConfigPage(hDlg, msg, wp, lp, *(lpConfigContext->appColorTheme), IDD_CONFIG_INFO);

    case WM_CTLCOLORSTATIC:
        return OnCtlColorConfigPage(hDlg, msg, wp, lp, *(lpConfigContext->appColorTheme));

    default:
        return FALSE;
    }
    return TRUE;
}

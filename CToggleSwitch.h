#pragma once

#include <Windows.h>
#include <tchar.h>
#include <gdiplus.h>

#pragma comment(lib, "gdiplus.lib")

#define TOGGLESWITCH_CLASS_NAME         _T("ToggleSwitch")
#define TSM_SETBGCOLOR                  WM_USER + 1
#define TSM_SETKNOBCOLOR                WM_USER + 2
#define TSM_SETPLATECOLOR               WM_USER + 3

namespace CToggleSwitch
{
    bool RegisterWndClass(HINSTANCE hInstance);

    HWND Create(HWND hParent,
        int id,
        int x,
        int y,
        int width,
        int height);

    bool IsChecked(HWND hSwitch);

    void Checked(HWND hSwitch);
    void UnChecked(HWND hSwitch);

    void Toggle(HWND hSwitch);

    void SetBgColor(HWND hSwitch, COLORREF colBg);
    void SetKnobColor(HWND hSwitch, COLORREF colKnob);
    void SetPlateColor(HWND hSwitch, COLORREF colPlate);
}

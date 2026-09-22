#pragma once

#include <Windows.h>
#include <tchar.h>
#include <gdiplus.h>

#pragma comment(lib, "gdiplus.lib")

namespace CToggleSwitch
{
    bool RegisterWndClass(HINSTANCE hInstance);

    HWND Create(HWND hParent,
        int id,
        int x,
        int y,
        int width,
        int height,
        COLORREF colBg = RGB(255, 255, 255),
        bool checked = false);

    bool IsChecked(HWND hSwitch);

    void Toggle(HWND hSwitch);

    void SetBgColor(HWND hSwitch, COLORREF colBg);
    void SetKnobColor(HWND hSwitch, COLORREF colKnob);
    void SetPlateColor(HWND hSwitch, COLORREF colPlate);
}

#pragma once

#include <Windows.h>
#include <tchar.h>
#include <gdiplus.h>

#pragma comment(lib, "gdiplus.lib")

constexpr UINT_PTR TIMER_POP = 1001;
constexpr UINT_PTR TIMER_OUT = 1002;
constexpr int POP_INTERVAL = 18;

using DRAW_CONTENTS_PROC = void (*)(Gdiplus::Graphics& g, int width, int height, LPVOID pAnim);

namespace CPopupLayerWnd
{
    constexpr double POP_MIN_SCALE = 0.45;
    constexpr DWORD  POP_DURATION = 180;

    enum class PopAnim {
        Show,
        Hide
    };

    struct POP_ANIM_DATA
    {
        bool running = false;

        UINT  outinterval = 1000 * 10;

        enum PopAnim Anim;

        ULONGLONG startTick = 0;

        int finalX = 0;
        int finalY = 0;
        int finalW = 360;
        int finalH = 420;

        POINT anchor{};

        DRAW_CONTENTS_PROC drawContents = nullptr;

        Gdiplus::Color bgColor{ 255, 255, 255, 255 };
        Gdiplus::Color borderColor{ 240, 45, 45, 45 };
        Gdiplus::Color textColor{ 240, 65, 65, 65 };

        LPVOID userData = nullptr;
    };

    bool RegisterWndClass(HINSTANCE hInstance);

    HWND Create(HINSTANCE hInstance);

    void StartPopupAnimation(HWND hPopup,
        int x, int y, int width, int height,
        PopAnim popanim,
        UINT outinterval = 0);
    void SetPopupBackgroundColor(HWND hPopup,
        const COLORREF bgColor,
        const BYTE bgAlpha);
    void SetPopupBorderColor(HWND hPopup,
        const COLORREF bgColor,
        const BYTE bgAlpha);
    void SetPopupTextColor(HWND hPopup,
        const COLORREF bgColor,
        const BYTE bgAlpha);
    void SetPopupUserData(HWND hPopup,
        LPVOID userData);
    void UpdatePopupAnimation(HWND hWnd);
    void DrawLayeredWindow(HWND hPopup, int x, int y, int width, int height,
        const Gdiplus::Color& bgColor);
    void DrawPopupBackground(Gdiplus::Graphics& g, int width, int height,
        const int tailHeight, const Gdiplus::Color& bgColor);
    void SetDrawContentsProc(HWND hPopup, DRAW_CONTENTS_PROC proc);
    double EaseOutBack(double t);
    double EaseInBack(double t);
}

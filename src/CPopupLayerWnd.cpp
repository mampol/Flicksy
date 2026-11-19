#include "CPopupLayerWnd.h"

#include <algorithm>
#include <cmath>

namespace CPopupLayerWnd
{
    LRESULT OnCreate(HWND hPopup, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT OnTimer(HWND hPopup, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT OnLButtonDown(HWND hPopup, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT OnDestroy(HWND hPopup, UINT msg, WPARAM wParam, LPARAM lParam);

    LRESULT CALLBACK WndProc(HWND hPopup, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        switch (msg) {
        case WM_CREATE:
            return (OnCreate(hPopup, msg, wParam, lParam));
        case WM_TIMER:
            return OnTimer(hPopup, msg, wParam, lParam);
        case WM_LBUTTONUP:
            return (OnLButtonDown(hPopup, msg, wParam, lParam));
        case WM_DESTROY:
            return (OnDestroy(hPopup, msg, wParam, lParam));
        }
        return (DefWindowProc(hPopup, msg, wParam, lParam));
    }

    LRESULT OnCreate(HWND hPopup, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        return 0;
    }

    LRESULT OnTimer(HWND hPopup, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        if (wParam == TIMER_POP)
        {
            UpdatePopupAnimation(hPopup);
            return 0;
        }

        if (wParam == TIMER_OUT)
        {
            KillTimer(hPopup, TIMER_OUT);
            CPopupLayerWnd::StartPopupAnimation(hPopup, 0, 0, 0, 0,
                CPopupLayerWnd::PopAnim::Hide);
            return 0;
        }

        return 0;
    }

    LRESULT OnLButtonDown(HWND hPopup, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        CPopupLayerWnd::StartPopupAnimation(hPopup, 0, 0, 0, 0,
            CPopupLayerWnd::PopAnim::Hide);

        return 0;
    }

    LRESULT OnDestroy(HWND hPopup, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        auto* pAnim = reinterpret_cast<POP_ANIM_DATA*>(GetWindowLongPtr(hPopup, GWLP_USERDATA));
        if (pAnim) {
            delete pAnim;
        }

        KillTimer(hPopup, TIMER_POP);
        KillTimer(hPopup, TIMER_OUT);

        return 0;
    }
}

bool CPopupLayerWnd::RegisterWndClass(HINSTANCE hInstance)
{
    WNDCLASSEX wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = nullptr;
    wc.lpszClassName = _T("PopupRgnWnd");
    return RegisterClassEx(&wc) != 0;
}

HWND CPopupLayerWnd::Create(HINSTANCE hInstance)
{
    HWND hPopup = CreateWindowEx(
        WS_EX_LAYERED |
        WS_EX_TOOLWINDOW |
        WS_EX_TOPMOST |
        WS_EX_NOACTIVATE,
        _T("PopupRgnWnd"),
        nullptr,
        WS_POPUP,
        0, 0,
        0, 0,
        nullptr,
        nullptr,
        hInstance,
        nullptr);

    if (!hPopup)
    {
        return nullptr;
    }

    auto* pAnim = new POP_ANIM_DATA{};
    SetWindowLongPtr(hPopup,
        GWLP_USERDATA,
        reinterpret_cast<LONG_PTR>(pAnim));

    return hPopup;
}

void CPopupLayerWnd::StartPopupAnimation(HWND hPopup,
    int x, int y,
    int width, int height,
    PopAnim popanim,
    UINT outinterval)
{
    auto* pAnim = reinterpret_cast<POP_ANIM_DATA*>( GetWindowLongPtr(hPopup, GWLP_USERDATA));
    if (!pAnim) return;

    KillTimer(hPopup, TIMER_POP);
    KillTimer(hPopup, TIMER_OUT);

    pAnim->running = true;
    pAnim->Anim = popanim;
    pAnim->startTick = GetTickCount64();

    if (popanim == PopAnim::Show)
    {
        pAnim->finalX = x;
        pAnim->finalY = y;
        pAnim->finalW = width;
        pAnim->finalH = height;
        pAnim->outinterval = outinterval;
    }

    pAnim->anchor.x = pAnim->finalX + pAnim->finalW - 40;
    pAnim->anchor.y = pAnim->finalY + pAnim->finalH;

    if (popanim == PopAnim::Show)
    {
        int w = static_cast<int>(
            pAnim->finalW * POP_MIN_SCALE);

        int h = static_cast<int>(
            pAnim->finalH * POP_MIN_SCALE);

        int startX =
            pAnim->anchor.x - (w - 40);

        int startY =
            pAnim->anchor.y - h;

        DrawLayeredWindow(hPopup,
            startX, startY,
            w, h,
            pAnim->bgColor);

        ShowWindow(hPopup,
            SW_SHOWNOACTIVATE);
    }

    SetTimer(hPopup,
        TIMER_POP,
        POP_INTERVAL,
        nullptr);
}

void CPopupLayerWnd::SetPopupBackgroundColor(HWND hPopup,
    const COLORREF bgColor,
    const BYTE bgAlpha)
{
    auto* pAnim = reinterpret_cast<POP_ANIM_DATA*>(GetWindowLongPtr(hPopup, GWLP_USERDATA));
    if (!pAnim) return;
    pAnim->bgColor = Gdiplus::Color(bgAlpha, GetRValue(bgColor), GetGValue(bgColor), GetBValue(bgColor));
}

void CPopupLayerWnd::SetPopupBorderColor(HWND hPopup, const COLORREF bgColor, const BYTE bgAlpha)
{
    auto* pAnim = reinterpret_cast<POP_ANIM_DATA*>(GetWindowLongPtr(hPopup, GWLP_USERDATA));
    if (!pAnim) return;
    pAnim->borderColor = Gdiplus::Color(bgAlpha, GetRValue(bgColor), GetGValue(bgColor), GetBValue(bgColor));
}

void CPopupLayerWnd::SetPopupTextColor(HWND hPopup, const COLORREF bgColor, const BYTE bgAlpha)
{
    auto* pAnim = reinterpret_cast<POP_ANIM_DATA*>(GetWindowLongPtr(hPopup, GWLP_USERDATA));
    if (!pAnim) return;
    pAnim->textColor = Gdiplus::Color(bgAlpha, GetRValue(bgColor), GetGValue(bgColor), GetBValue(bgColor));
}

void CPopupLayerWnd::SetPopupUserData(HWND hPopup, LPVOID userData)
{
    auto* pAnim = reinterpret_cast<POP_ANIM_DATA*>(GetWindowLongPtr(hPopup, GWLP_USERDATA));
    if (!pAnim) return;
    pAnim->userData = userData;
}

void CPopupLayerWnd::UpdatePopupAnimation(HWND hPopup)
{
    auto* pAnim = reinterpret_cast<POP_ANIM_DATA*>(GetWindowLongPtr(hPopup, GWLP_USERDATA));
    if (!pAnim || !pAnim->running) return;

    ULONGLONG elapsed = GetTickCount64() - pAnim->startTick;

    double t = (std::min)(1.0,elapsed / static_cast<double>(POP_DURATION));

    double scale;

    if (pAnim->Anim == PopAnim::Show)
    {
        double eased = EaseOutBack(t);
        scale = POP_MIN_SCALE + (1.0 - POP_MIN_SCALE) * eased;
    }
    else
    {
        double eased = EaseInBack(t);
        scale = 1.0 + (POP_MIN_SCALE - 1.0) * eased;
    }

    int w = static_cast<int>(pAnim->finalW * scale);
    int h = static_cast<int>(pAnim->finalH * scale);

    constexpr int TAIL_OFFSET_X = 40;

    int x = pAnim->anchor.x - (w - TAIL_OFFSET_X);
    int y = pAnim->anchor.y - h;

    DrawLayeredWindow(hPopup, x, y, w, h, pAnim->bgColor);

    if (t >= 1.0)
    {
        KillTimer(hPopup, TIMER_POP);
        pAnim->running = false;

        if (pAnim->Anim == PopAnim::Show)
        {
            DrawLayeredWindow(hPopup,
                pAnim->finalX, pAnim->finalY,
                pAnim->finalW, pAnim->finalH,
                pAnim->bgColor);

            if (pAnim->outinterval > 1000) {
                SetTimer(hPopup,
                    TIMER_OUT,
                    pAnim->outinterval,
                    nullptr);
            }
        }
        else
        {
            ShowWindow(hPopup, SW_HIDE);
        }
    }
}

void CPopupLayerWnd::DrawLayeredWindow(HWND hPopup,
    int x, int y,
    int width, int height,
    const Gdiplus::Color& bgColor)
{
    if (width <= 0 || height <= 0) return;

    HDC hScreenDC = GetDC(nullptr);
    HDC hMemDC = CreateCompatibleDC(hScreenDC);

    BITMAPINFO bmi{};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* pBits = nullptr;

    HBITMAP hBitmap = CreateDIBSection(hScreenDC,
            &bmi,
            DIB_RGB_COLORS,
            &pBits,
            nullptr,
            0);
    if (!hBitmap) return;

    HBITMAP hOldBitmap = static_cast<HBITMAP>(SelectObject(hMemDC, hBitmap));

    memset(pBits, 0, static_cast<size_t>(width) * height * 4);

    Gdiplus::Bitmap bitmap(width, height,
        width * 4,
        PixelFormat32bppPARGB,
        static_cast<BYTE*>(pBits));

    Gdiplus::Graphics g(&bitmap);

    g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);

    const int tailHeight = 20;

    DrawPopupBackground(g, width, height, tailHeight, bgColor);

    auto* pAnim = reinterpret_cast<POP_ANIM_DATA*>(GetWindowLongPtr(hPopup, GWLP_USERDATA));
    if (pAnim && pAnim->drawContents)
    {
        pAnim->drawContents(g, width, height - tailHeight, reinterpret_cast<LPVOID>(pAnim));
    }

    POINT ptDst{ x, y };
    POINT ptSrc{ 0, 0 };

    SIZE sizeWnd{
        width,
        height
    };

    BLENDFUNCTION blend{};
    blend.BlendOp = AC_SRC_OVER;
    blend.SourceConstantAlpha = 255;
    blend.AlphaFormat = AC_SRC_ALPHA;

    UpdateLayeredWindow(hPopup,
        hScreenDC,
        &ptDst,
        &sizeWnd,
        hMemDC,
        &ptSrc,
        0,
        &blend,
        ULW_ALPHA);

    SelectObject(hMemDC, hOldBitmap);

    DeleteObject(hBitmap);
    DeleteDC(hMemDC);

    ReleaseDC(nullptr, hScreenDC);
}

void CPopupLayerWnd::DrawPopupBackground(Gdiplus::Graphics& g,
    int width, int height, const int tailHeight,
    const Gdiplus::Color& bgColor)
{
    Gdiplus::SolidBrush bgBrush(bgColor);

    g.FillRectangle(&bgBrush,
        0,
        0,
        width,
        height - tailHeight);

    const int tailTop = height - tailHeight - 2;

    Gdiplus::Point pts[] =
    {
        Gdiplus::Point(width - 58, tailTop),
        Gdiplus::Point(width - 22, tailTop),
        Gdiplus::Point(width - 40, height)
    };

    g.FillPolygon(&bgBrush, pts, _countof(pts));
}

void CPopupLayerWnd::SetDrawContentsProc(HWND hPopup, DRAW_CONTENTS_PROC proc)
{
    auto* pAnim = reinterpret_cast<POP_ANIM_DATA*>(GetWindowLongPtr(hPopup, GWLP_USERDATA));
    if (!pAnim) return;

    pAnim->drawContents = proc;
}

double CPopupLayerWnd::EaseOutBack(double t)
{
    constexpr double c1 = 1.70158;
    constexpr double c3 = c1 + 1.0;
    return 1.0 + c3 * std::pow(t - 1.0, 3.0) + c1 * std::pow(t - 1.0, 2.0);
}

double CPopupLayerWnd::EaseInBack(double t)
{
    constexpr double c1 = 1.70158;
    constexpr double c3 = c1 + 1.0;
    return c3 * t * t * t - c1 * t * t;
}

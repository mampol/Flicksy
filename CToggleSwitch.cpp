#include "CToggleSwitch.h"

namespace CToggleSwitch
{
    namespace
    {
        struct ToggleState
        {
            bool checked = false;
            float pos = 0.0f;
            float target = 0.0f;
            COLORREF colBg = RGB(255, 255, 255);
            COLORREF colKnob = RGB(255, 255, 255);
            COLORREF colPlate = RGB(70, 160, 230);
        };

        LRESULT OnCreate(HWND hSwitch, UINT msg, WPARAM wParam, LPARAM lParam);
        LRESULT OnPaint(HWND hSwitch, UINT msg, WPARAM wParam, LPARAM lParam);
        LRESULT OnTimer(HWND hSwitch, UINT msg, WPARAM wParam, LPARAM lParam);
        LRESULT OnLButtonUp(HWND hSwitch, UINT msg, WPARAM wParam, LPARAM lParam);
        LRESULT OnDestroy(HWND hSwitch, UINT msg, WPARAM wParam, LPARAM lParam);

        LRESULT CALLBACK WndProc(HWND hSwitch, UINT msg, WPARAM wParam, LPARAM lParam)
        {
            switch (msg) {
            case WM_CREATE:
                return (OnCreate(hSwitch, msg, wParam, lParam));
            case WM_ERASEBKGND:
                return 1;
            case WM_TIMER:
                return OnTimer(hSwitch, msg, wParam, lParam);
            case WM_PAINT:
                return (OnPaint(hSwitch, msg, wParam, lParam));
            case WM_LBUTTONUP:
                return (OnLButtonUp(hSwitch, msg, wParam, lParam));
            case WM_DESTROY:
                return (OnDestroy(hSwitch, msg, wParam, lParam));
            }
            return (DefWindowProc(hSwitch, msg, wParam, lParam));
        }

        LRESULT OnCreate(HWND hSwitch, UINT msg, WPARAM wParam, LPARAM lParam)
        {
            return 0;
        }

        LRESULT OnTimer(HWND hSwitch, UINT msg, WPARAM wParam, LPARAM lParam)
        {
            ToggleState* state = reinterpret_cast<ToggleState*>(GetWindowLongPtr(hSwitch, GWLP_USERDATA));

            if (!state) return 0;

            const float speed = 0.10f;

            if (state->pos < state->target)
            {
                state->pos += speed;

                if (state->pos >= state->target)
                    state->pos = state->target;
            }
            else if (state->pos > state->target)
            {
                state->pos -= speed;

                if (state->pos <= state->target)
                    state->pos = state->target;
            }

            InvalidateRect(hSwitch, nullptr, FALSE);

            if (state->pos == state->target)
                KillTimer(hSwitch, 1);

            return 0;
        }

        LRESULT OnPaint(HWND hSwitch, UINT msg, WPARAM wParam, LPARAM lParam)
        {
            PAINTSTRUCT ps{};
            HDC hdc = BeginPaint(hSwitch, &ps);

            ToggleState* state = reinterpret_cast<ToggleState*>(GetWindowLongPtr(hSwitch, GWLP_USERDATA));
            if (!state)
            {
                EndPaint(hSwitch, &ps);
                return 0;
            }

            RECT rc{};
            GetClientRect(hSwitch, &rc);

            int width = rc.right - rc.left;
            int height = rc.bottom - rc.top;

            int margin = 1;
            int diameter = height - margin * 2;

            HDC memDC = CreateCompatibleDC(hdc);
            HBITMAP hBitmap = CreateCompatibleBitmap(hdc, width, height);
            HBITMAP hOldBitmap = static_cast<HBITMAP>(SelectObject(memDC, hBitmap));

            HBRUSH hBk = CreateSolidBrush(state->colBg);
            FillRect(memDC, &rc, hBk);
            DeleteObject(hBk);

            Gdiplus::Graphics g(memDC);

            g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
            g.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);

            bool checked = CToggleSwitch::IsChecked(hSwitch);

            Gdiplus::Color plateColor = checked ?
                Gdiplus::Color(GetRValue(state->colPlate), GetGValue(state->colPlate), GetBValue(state->colPlate))
                : Gdiplus::Color(180, 180, 180);

            Gdiplus::SolidBrush plateBrush(plateColor);

            Gdiplus::GraphicsPath path;

            float x = 0.5f;
            float y = 0.5f;
            float w = static_cast<float>(width - 1);
            float h = static_cast<float>(height - 1);
            float r = h / 2.0f;

            path.AddArc(x, y, h, h, 90.0f, 180.0f);
            path.AddArc(x + w - h, y, h, h, 270.0f, 180.0f);
            path.CloseFigure();

            g.FillPath(&plateBrush, &path);

            float leftPos = static_cast<float>(margin + 1);
            float rightPos = static_cast<float>(width - diameter - margin - 1);

            float knobLeft = leftPos + (rightPos - leftPos) * state->pos;

            Gdiplus::SolidBrush shadowBrush(Gdiplus::Color(45, 0, 0, 0));

            Gdiplus::SolidBrush knobBrush(Gdiplus::Color(GetRValue(state->colKnob), GetGValue(state->colKnob), GetBValue(state->colKnob)));

            g.FillEllipse(&shadowBrush,
                static_cast<Gdiplus::REAL>(knobLeft) + 1.0f,
                static_cast<Gdiplus::REAL>(margin) + 1.0f,
                static_cast<Gdiplus::REAL>(diameter),
                static_cast<Gdiplus::REAL>(diameter));

            g.FillEllipse(&knobBrush,
                static_cast<Gdiplus::REAL>(knobLeft) + 0.5f,
                static_cast<Gdiplus::REAL>(margin) + 0.5f,
                static_cast<Gdiplus::REAL>(diameter - 1),
                static_cast<Gdiplus::REAL>(diameter - 1));

            if (checked)
            {
                Gdiplus::Pen linePen(Gdiplus::Color(220, 255, 255, 255), 1.5f);

                float x = 11.0f;
                float y1 = 9.0f;
                float y2 = height - 9.0f;

                g.DrawLine(&linePen,
                    x, y1,
                    x, y2);
            }

            BitBlt(hdc,
                0, 0,
                width, height,
                memDC,
                0, 0,
                SRCCOPY);

            SelectObject(memDC, hOldBitmap);
            DeleteObject(hBitmap);
            DeleteDC(memDC);

            EndPaint(hSwitch, &ps);

            return 0;
        }

        LRESULT OnLButtonUp(HWND hSwitch, UINT msg, WPARAM wParam, LPARAM lParam)
        {
            CToggleSwitch::Toggle(hSwitch);

            SendMessage(GetParent(hSwitch),
                WM_COMMAND,
                MAKEWPARAM(GetDlgCtrlID(hSwitch), BN_CLICKED),
                reinterpret_cast<LPARAM>(hSwitch));

            return 0;
        }

        LRESULT OnDestroy(HWND hSwitch, UINT msg, WPARAM wParam, LPARAM lParam)
        {
            ToggleState* state = reinterpret_cast<ToggleState*>(GetWindowLongPtr(hSwitch, GWLP_USERDATA));
            if (!state) return 0;

            delete state;

            return 0;
        }
    }
}

bool CToggleSwitch::RegisterWndClass(HINSTANCE hInstance)
{
    WNDCLASSEX wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_HAND);
    wc.hbrBackground = nullptr;
    wc.lpszClassName = _T("ToggleSwitch");
    return RegisterClassEx(&wc) != 0;
}

HWND CToggleSwitch::Create(HWND hParent, int id,
    int x, int y,
    int width, int height,
    COLORREF colBg,
    bool checked)
{
    HINSTANCE hInstance = reinterpret_cast<HINSTANCE>(GetWindowLongPtr(hParent, GWLP_HINSTANCE));

    HWND hSwitch = CreateWindowEx(0,
        _T("ToggleSwitch"),
        nullptr,
        WS_CHILD | WS_VISIBLE,
        x, y,
        width, height,
        hParent,
        reinterpret_cast<HMENU>(id),
        hInstance,
        nullptr);

    if (hSwitch) {
        ToggleState* state = new ToggleState;
        if (!state)
        {
            DestroyWindow(hSwitch);
            return nullptr;
        }
        state->checked = checked;
        state->pos = checked ? 1.0f : 0.0f;
        state->target = state->pos;
        state->colBg = colBg;
        SetWindowLongPtr(hSwitch, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(state));
    }
    return hSwitch;
}

bool CToggleSwitch::IsChecked(HWND hSwitch)
{
    ToggleState* state = reinterpret_cast<ToggleState*>(GetWindowLongPtr(hSwitch, GWLP_USERDATA));
    if (!state) return false;
    return state->checked;
}

void CToggleSwitch::Toggle(HWND hSwitch)
{
    ToggleState *state = reinterpret_cast<ToggleState*>(GetWindowLongPtr(hSwitch, GWLP_USERDATA));
    if (!state) return;

    state->checked = !state->checked;
    state->target = state->checked ? 1.0f : 0.0f;
    SetTimer(hSwitch, 1, 15, nullptr);

    InvalidateRect(hSwitch, nullptr, FALSE);
}

void CToggleSwitch::SetBgColor(HWND hSwitch, COLORREF colBg)
{
    ToggleState* state = reinterpret_cast<ToggleState*>(GetWindowLongPtr(hSwitch, GWLP_USERDATA));
    if (!state) return;
    state->colBg = colBg;

    InvalidateRect(hSwitch, nullptr, FALSE);
}

void CToggleSwitch::SetKnobColor(HWND hSwitch, COLORREF colKnob)
{
    ToggleState* state = reinterpret_cast<ToggleState*>(GetWindowLongPtr(hSwitch, GWLP_USERDATA));
    if (!state) return;
    state->colKnob = colKnob;

    InvalidateRect(hSwitch, nullptr, FALSE);
}

void CToggleSwitch::SetPlateColor(HWND hSwitch, COLORREF colPlate)
{
    ToggleState* state = reinterpret_cast<ToggleState*>(GetWindowLongPtr(hSwitch, GWLP_USERDATA));
    if (!state) return;
    state->colPlate = colPlate;

    InvalidateRect(hSwitch, nullptr, FALSE);
}

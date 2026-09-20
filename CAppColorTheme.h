#pragma once

#include <windows.h>

enum class ColorMode
{
    Light,
    Dark
};

struct AppColors
{
    COLORREF windowBg;

    COLORREF headerBg;
    COLORREF headerBorder;

    COLORREF panelBg[4];
    COLORREF panelBorder[4];

    COLORREF text;
    COLORREF subText;

    COLORREF border;

    COLORREF buttonBg;
    COLORREF buttonText;
    COLORREF buttonHover;

    COLORREF accent;
};

static const AppColors LIGHT_COLORS =
{
    RGB(245, 247, 250),   // windowBg

    RGB(233, 245, 250),   // headerBg
    RGB(187, 206, 204),   // headerBorder

    RGB(243, 251, 244),   // panelBg1
    RGB(252, 249, 242),   // panelBg2
    RGB(248, 248, 254),   // panelBg3
    RGB(250, 251, 253),   // panelBg4

    RGB(114, 193, 125),   // panelBorder1
    RGB(199, 192, 177),   // panelBorder2
    RGB(162, 122, 235),   // panelBorder3
    RGB(188, 202, 215),   // panelBorder4

    RGB(35, 35, 35),      // text
    RGB(110, 115, 120),   // subText

    RGB(215, 220, 225),   // border

    RGB(240, 242, 245),   // buttonBg
    RGB(35, 35, 35),      // buttonText
    RGB(225, 228, 232),   // buttonHover

    RGB(40, 120, 220)     // accent
};

static const AppColors DARK_COLORS =
{
    RGB(30, 31, 34),      // windowBg

    RGB(233, 245, 250),   // headerBg
    RGB(187, 206, 204),   // headerBorder

    RGB(243, 251, 244),   // panelBg1
    RGB(252, 249, 242),   // panelBg2
    RGB(248, 248, 254),   // panelBg3
    RGB(250, 251, 253),   // panelBg4

    RGB(114, 193, 125),   // panelBorder1
    RGB(199, 192, 177),   // panelBorder2
    RGB(162, 122, 235),   // panelBorder3
    RGB(188, 202, 215),   // panelBorder4

    RGB(240, 240, 240),   // text
    RGB(165, 170, 180),   // subText

    RGB(65, 68, 74),      // border

    RGB(52, 55, 60),      // buttonBg
    RGB(245, 245, 245),   // buttonText
    RGB(65, 69, 75),      // buttonHover

    RGB(70, 145, 255)     // accent
};

class CAppColorTheme
{
    ColorMode m_mode = ColorMode::Light;

    AppColors m_colors;

    HBRUSH m_hWindowBrush;
    HBRUSH m_hHeaderBrush;
    HBRUSH m_hPanelBrush[4];
    HBRUSH m_hButtonBrush;

    HPEN m_hHeaderPen;
    HPEN m_hPanelPen[4];

public:

    CAppColorTheme()
    {
        m_hWindowBrush = nullptr;
        m_hHeaderBrush = nullptr;
        for (int i = 0; i < _countof(m_hPanelBrush); i++)
        {
            m_hPanelBrush[i] = nullptr;
        }
        m_hButtonBrush = nullptr;

        m_hHeaderPen = nullptr;
        for (int i = 0; i < _countof(m_hPanelPen); i++)
        {
            m_hPanelPen[i] = nullptr;
        }

        SetMode(ColorMode::Light);
    }

    ~CAppColorTheme()
    {
        if (m_hWindowBrush) DeleteObject(m_hWindowBrush);
        if (m_hHeaderBrush) DeleteObject(m_hHeaderBrush);
        for (int i = 0; i < _countof(m_hPanelBrush); i++)
        {
            if (m_hPanelBrush[i]) DeleteObject(m_hPanelBrush[i]);
        }
        if (m_hButtonBrush) DeleteObject(m_hButtonBrush);

        if (m_hHeaderPen) DeleteObject(m_hHeaderPen);
        for (int i = 0; i < _countof(m_hPanelPen); i++)
        {
            if (m_hPanelPen[i]) DeleteObject(m_hPanelPen[i]);
        }
    };

    void SetMode(ColorMode mode)
    {
        m_mode = mode;

        switch (m_mode)
        {
        case ColorMode::Dark:
            m_colors = DARK_COLORS;
            break;

        case ColorMode::Light:
        default:
            m_colors = LIGHT_COLORS;
            break;
        }

        if (m_hWindowBrush)
        {
            DeleteObject(m_hWindowBrush);
            m_hWindowBrush = nullptr;
        }
        if (m_hHeaderBrush)
        {
            DeleteObject(m_hHeaderBrush);
            m_hHeaderBrush = nullptr;
        }
        for (int i = 0; i < _countof(m_hPanelBrush); i++)
        {
            if (m_hPanelBrush[i])
            {
                DeleteObject(m_hPanelBrush[i]);
                m_hPanelBrush[i] = nullptr;
            }
        }
        if (m_hButtonBrush)
        {
            DeleteObject(m_hButtonBrush);
            m_hButtonBrush = nullptr;
        }

        if (m_hHeaderPen)
        {
            DeleteObject(m_hHeaderPen);
            m_hHeaderPen = nullptr;
        }
        for (int i = 0; i < _countof(m_hPanelPen); i++)
        {
            if (m_hPanelPen[i])
            {
                DeleteObject(m_hPanelPen[i]);
                m_hPanelPen[i] = nullptr;
            }
        }

        m_hWindowBrush = CreateSolidBrush(m_colors.windowBg);
        m_hHeaderBrush = CreateSolidBrush(m_colors.headerBg);
        for (int i = 0; i < _countof(m_hPanelBrush); i++)
        {
            m_hPanelBrush[i] = CreateSolidBrush(m_colors.panelBg[i]);
        }
        m_hButtonBrush = CreateSolidBrush(m_colors.buttonBg);

        m_hHeaderPen = CreatePen(PS_SOLID, 0, m_colors.headerBorder);
        for (int i = 0; i < _countof(m_hPanelPen); i++)
        {
            m_hPanelPen[i] = CreatePen(PS_SOLID, 0, m_colors.panelBorder[i]);
        }
    };

    ColorMode Mode() const
    {
        return m_mode;
    }

    const AppColors& Colors() const
    {
        return m_colors;
    }

    HBRUSH WindowBrush() const
    {
        return m_hWindowBrush;
    }

    HBRUSH HeaderBrush() const
    {
        return m_hHeaderBrush;
    }

    HBRUSH PanelBrush(int nPanel) const
    {
        if (nPanel >= 0 && nPanel < _countof(m_hPanelBrush))
            return m_hPanelBrush[nPanel];

        return nullptr;
    }

    HPEN HeaderPen() const
    {
        return m_hHeaderPen;
    }

    HPEN PanelPen(int nPanel) const
    {
        if (nPanel >= 0 && nPanel < _countof(m_hPanelPen))
            return m_hPanelPen[nPanel];

        return nullptr;
    }

    HBRUSH ButtonBrush() const
    {
        return m_hButtonBrush;
    }
};

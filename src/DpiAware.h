#pragma once

#include <windows.h>

static void DpiAware()
{
	SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
}

static int DpiScale(HWND hWnd, int value)
{
	UINT dpi = GetDpiForWindow(hWnd);
	return MulDiv(value, dpi, 96);
}

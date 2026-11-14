#pragma once

#include <windows.h>
#include <gdiplus.h>
#include <commctrl.h>
#include <tchar.h>

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "comctl32.lib")

class CPngResource
{
	Gdiplus::GdiplusStartupInput m_gdiplusStartupInput;
	ULONG_PTR m_gdiplusToken = 0;

	int m_nBtn = 0;
	SIZE m_SizeBtn = { 0, 0 };

public:

	CPngResource();
	~CPngResource();

	BOOL InitGdiPlus();
	void EndGdiPlus();

	HBITMAP LoadPngResource(
		HINSTANCE hInst,
		UINT uiPngRes);

	HIMAGELIST MakeImageListPng(
		int nWidth,
		HBITMAP hBmp);

	Gdiplus::Bitmap* LoadPngBitmap(
		HINSTANCE hInst,
		UINT uiPngRes);

	void DrawPng(
		Gdiplus::Graphics& g,
		Gdiplus::Bitmap* bitmap,
		int x,
		int y,
		int width,
		int height);

	void DrawSprite(Gdiplus::Graphics& g,
		Gdiplus::Bitmap* bitmap, 
		int index,
		int x,
		int y,
		int srcw,
		int srch,
		int destw,
		int desth);

	void DrawSpriteIcon(Gdiplus::Graphics& g,
		Gdiplus::Bitmap* bitmap,
		int index,
		int x,
		int y,
		int srcsize,
		int destsize,
		COLORREF color);

	int GetBtnCount() { return m_nBtn; };
	int GetBtnWith() { return m_SizeBtn.cx; };
	int GetBtnHeight() { return m_SizeBtn.cy; };
};

#pragma once

#include <windows.h>
#include <gdiplus.h>
#include <commctrl.h>
#include <tchar.h>

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "comctl32.lib")

class CImgListPng
{
	Gdiplus::GdiplusStartupInput m_gdiplusStartupInput;
	ULONG_PTR m_gdiplusToken = 0;

	int m_nBtn = 0;
	SIZE m_SizeBtn = { 0, 0 };

public:

	CImgListPng();
	~CImgListPng();

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

	int GetBtnCount() { return m_nBtn; };
	int GetBtnWith() { return m_SizeBtn.cx; };
	int GetBtnHeight() { return m_SizeBtn.cy; };
};

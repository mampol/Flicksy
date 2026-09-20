#include "CImgListPng.h"

CImgListPng::CImgListPng()
{
}

CImgListPng::~CImgListPng()
{
}

BOOL CImgListPng::InitGdiPlus()
{
	if (GdiplusStartup(&m_gdiplusToken, &m_gdiplusStartupInput, NULL) != Gdiplus::Ok)
	{
		return FALSE;
	}
	return TRUE;
}

void CImgListPng::EndGdiPlus()
{
	Gdiplus::GdiplusShutdown(m_gdiplusToken);
}

HBITMAP CImgListPng::LoadPngResource(HINSTANCE hInst, UINT uiPngRes)
{
	HBITMAP hBitmap = NULL;

	//--------------------------------------------------
	// PNGリソース取得
	//--------------------------------------------------
	HRSRC hRes = FindResource(hInst, MAKEINTRESOURCE(uiPngRes), _T("PNG"));
	if (!hRes)
	{
		return NULL;
	}

	HGLOBAL hResData = LoadResource(hInst, hRes);
	if (!hResData)
	{
		return NULL;
	}

	DWORD dwSize = SizeofResource(hInst, hRes);
	const void* pResData = LockResource(hResData);
	if (!pResData || dwSize == 0)
	{
		return NULL;
	}

	//--------------------------------------------------
	// IStream用メモリ作成
	//--------------------------------------------------
	HGLOBAL hBuffer = GlobalAlloc(GMEM_MOVEABLE, dwSize);
	if (!hBuffer) {
		return NULL;
	}

	void* pBuffer = GlobalLock(hBuffer);
	if (!pBuffer)
	{
		GlobalFree(hBuffer);
		return NULL;
	}
	CopyMemory(pBuffer, pResData, dwSize);
	GlobalUnlock(hBuffer);

	//--------------------------------------------------
	// メモリ → IStream
	//--------------------------------------------------
	IStream* pStream = NULL;
	HRESULT hr =
		CreateStreamOnHGlobal(
			hBuffer,
			TRUE,       // Stream解放時にhBufferも解放
			&pStream);
	if (FAILED(hr) || !pStream)
	{
		GlobalFree(hBuffer);
		return NULL;
	}

	//--------------------------------------------------
	// PNGロード
	//--------------------------------------------------
	Gdiplus::Bitmap bitmap(pStream, FALSE);
	if (bitmap.GetLastStatus() != Gdiplus::Ok)
	{
		pStream->Release();
		return NULL;
	}
	UINT width = bitmap.GetWidth();
	UINT height = bitmap.GetHeight();

	//--------------------------------------------------
	// 32bit top-down DIB作成
	//--------------------------------------------------
	BITMAPINFO bmi;

	ZeroMemory(&bmi, sizeof(bmi));
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = (LONG)width;

	// マイナスでtop-down
	bmi.bmiHeader.biHeight = -(LONG)height;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biCompression = BI_RGB;

	void* pDibBits = NULL;

	HDC hDC = GetDC(NULL);
	hBitmap = CreateDIBSection(hDC, &bmi, DIB_RGB_COLORS, &pDibBits, NULL, 0);

	ReleaseDC(NULL, hDC);

	if (!hBitmap || !pDibBits)
	{
		if (hBitmap)
		{
			DeleteObject(hBitmap);
		}
		pStream->Release();
		return NULL;
	}

	//--------------------------------------------------
	// GDI+から32bit PARGBで取得
	//--------------------------------------------------
	Gdiplus::Rect rc(0, 0, width, height);
	Gdiplus::BitmapData bmpData;
	ZeroMemory(&bmpData, sizeof(bmpData));

	if (bitmap.LockBits(&rc, Gdiplus::ImageLockModeRead, PixelFormat32bppPARGB, &bmpData) != Gdiplus::Ok)
	{
		DeleteObject(hBitmap);
		pStream->Release();
		return NULL;
	}

	//--------------------------------------------------
	// DIBへコピー
	//--------------------------------------------------
	BYTE* pDst = (BYTE*)pDibBits;
	BYTE* pSrc = (BYTE*)bmpData.Scan0;
	const UINT nLineBytes = width * 4;
	for (UINT y = 0; y < height; y++)
	{
		BYTE* pSrcLine;
		if (bmpData.Stride >= 0)
		{
			pSrcLine = pSrc + (y * bmpData.Stride);
		}
		else
		{
			pSrcLine = pSrc + ((height - 1 - y) * (-bmpData.Stride));
		}
		CopyMemory(pDst + (y * nLineBytes), pSrcLine, nLineBytes);
	}

	bitmap.UnlockBits(&bmpData);

	//--------------------------------------------------
	// 後始末
	//--------------------------------------------------
	pStream->Release();

	return hBitmap;
}

HIMAGELIST CImgListPng::MakeImageListPng(int nWidth, HBITMAP hBmp)
{
	if (!hBmp || nWidth <= 0)
	{
		return NULL;
	}

	BITMAP bmp;
	ZeroMemory(&bmp, sizeof(bmp));

	if (!GetObject(hBmp, sizeof(BITMAP), &bmp))
	{
		return NULL;
	}

	//--------------------------------------------------
	// 横一列のスプライト画像
	//--------------------------------------------------
	m_nBtn = bmp.bmWidth / nWidth;
	if (m_nBtn < 1) {
		return NULL;
	}

	m_SizeBtn.cx = nWidth;
	m_SizeBtn.cy = bmp.bmHeight;

	//--------------------------------------------------
	// 32bit alpha ImageList
	//--------------------------------------------------
	HIMAGELIST hImgList = ImageList_Create(m_SizeBtn.cx, m_SizeBtn.cy, ILC_COLOR32, m_nBtn, 0);
	if (!hImgList) {
		return NULL;
	}

	//--------------------------------------------------
	// mask無し
	//--------------------------------------------------
	if (ImageList_Add(hImgList, hBmp, NULL) < 0)
	{
		ImageList_Destroy(hImgList);
		return NULL;
	}

	return hImgList;
}

Gdiplus::Bitmap* CImgListPng::LoadPngBitmap(HINSTANCE hInst, UINT uiPngRes)
{
	HRSRC hRes = FindResource(hInst,
			MAKEINTRESOURCE(uiPngRes),
			_T("PNG"));
	if (!hRes) return nullptr;

	HGLOBAL hResData = LoadResource(hInst, hRes);
	if (!hResData) return nullptr;

	DWORD dwSize = SizeofResource(hInst, hRes);

	const void* pResData = LockResource(hResData);
	if (!pResData || dwSize == 0) return nullptr;

	HGLOBAL hBuffer = GlobalAlloc(GMEM_MOVEABLE, dwSize);
	if (!hBuffer) return nullptr;

	void* pBuffer = GlobalLock(hBuffer);
	if (!pBuffer)
	{
		GlobalFree(hBuffer);
		return nullptr;
	}
	CopyMemory(pBuffer, pResData, dwSize);
	GlobalUnlock(hBuffer);

	IStream* pStream = nullptr;
	HRESULT hr = CreateStreamOnHGlobal(hBuffer, TRUE, &pStream);
	if (FAILED(hr) || !pStream)
	{
		GlobalFree(hBuffer);
		return nullptr;
	}
	Gdiplus::Bitmap* pBitmap = Gdiplus::Bitmap::FromStream(pStream);
	pStream->Release();

	if (!pBitmap || pBitmap->GetLastStatus() != Gdiplus::Ok)
	{
		delete pBitmap;
		return nullptr;
	}

	return pBitmap;
}

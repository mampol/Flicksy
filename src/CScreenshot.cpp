#include "CScreenshot.h"
#include <gdiplus.h>
#include <objidl.h>

#pragma comment(lib, "gdiplus.lib")

CScreenshot::~CScreenshot()
{
    Clear();
}

void CScreenshot::Clear()
{
    if (m_hBitmap)
    {
        DeleteObject(m_hBitmap);
        m_hBitmap = nullptr;
    }
}

bool CScreenshot::CaptureForegroundWindow()
{
    Clear();

    HWND hWnd = GetForegroundWindow();
    if (!hWnd) return false;

    RECT rc{};
    if (!GetWindowRect(hWnd, &rc)) return false;

    const int width = rc.right - rc.left;
    const int height = rc.bottom - rc.top;

    if (width <= 0 || height <= 0) return false;

    HDC hScreenDC = GetDC(nullptr);
    if (!hScreenDC) return false;

    HDC hMemDC = CreateCompatibleDC(hScreenDC);
    if (!hMemDC)
    {
        ReleaseDC(nullptr, hScreenDC);
        return false;
    }

    HBITMAP hBitmap = CreateCompatibleBitmap(hScreenDC, width, height);
    if (!hBitmap)
    {
        DeleteDC(hMemDC);
        ReleaseDC(nullptr, hScreenDC);
        return false;
    }

    HBITMAP hOldBitmap = static_cast<HBITMAP>(SelectObject(hMemDC, hBitmap));
    BOOL result =BitBlt(hMemDC,
            0, 0,
            width, height,
            hScreenDC,
            rc.left, rc.top,
            SRCCOPY | CAPTUREBLT);

    SelectObject(hMemDC, hOldBitmap);

    DeleteDC(hMemDC);
    ReleaseDC(nullptr, hScreenDC);

    if (!result)
    {
        DeleteObject(hBitmap);
        return false;
    }

    m_hBitmap = hBitmap;

    return true;
}

bool CScreenshot::SavePng(const std::filesystem::path& path)
{
    if (!m_hBitmap) return false;

    CLSID encoderClsid{};

    if (!GetEncoderClsid(L"image/png", &encoderClsid))
    {
        return false;
    }

    Gdiplus::Bitmap bitmap(m_hBitmap, nullptr);

    return bitmap.Save(path.c_str(), &encoderClsid,  nullptr) == Gdiplus::Ok;
}

bool CScreenshot::GetPngData(std::vector<BYTE>& data)
{
    data.clear();

    if (!m_hBitmap) return false;

    CLSID encoderClsid{};
    if (!GetEncoderClsid(L"image/png", &encoderClsid))
    {
        return false;
    }

    Gdiplus::Bitmap bitmap(m_hBitmap, nullptr);

    IStream* pStream = nullptr;
    if (CreateStreamOnHGlobal(nullptr, TRUE, &pStream) != S_OK)
    {
        return false;
    }

    Gdiplus::Status status = bitmap.Save(pStream, &encoderClsid, nullptr);
    if (status != Gdiplus::Ok)
    {
        pStream->Release();
        return false;
    }

    HGLOBAL hGlobal = nullptr;
    if (GetHGlobalFromStream(pStream, &hGlobal) != S_OK)
    {
        pStream->Release();
        return false;
    }

    SIZE_T size = GlobalSize(hGlobal);
    void* pData = GlobalLock(hGlobal);

    if (!pData || size == 0)
    {
        if (pData) GlobalUnlock(hGlobal);

        pStream->Release();
        return false;
    }

    data.resize(size);

    memcpy(data.data(), pData, size);

    GlobalUnlock(hGlobal);

    pStream->Release();

    return true;
}

bool CScreenshot::GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
{
    UINT num = 0;
    UINT size = 0;

    Gdiplus::GetImageEncodersSize(&num, &size);
    if (size == 0) return false;

    auto pInfo = reinterpret_cast<Gdiplus::ImageCodecInfo*>(malloc(size));
    if (!pInfo)return false;

    if (Gdiplus::GetImageEncoders(num, size, pInfo) != Gdiplus::Ok)
    {
        free(pInfo);
        return false;
    }

    bool found = false;
    for (UINT i = 0; i < num; ++i)
    {
        if (wcscmp(pInfo[i].MimeType, format) == 0)
        {
            *pClsid = pInfo[i].Clsid;
            found = true;
            break;
        }
    }
    free(pInfo);

    return found;
}

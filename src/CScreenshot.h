#pragma once

#include <Windows.h>
#include <filesystem>
#include <vector>

class CScreenshot
{
public:
    CScreenshot() = default;
    ~CScreenshot();

    bool CaptureForegroundWindow();
    bool SavePng(const std::filesystem::path& path);
    bool GetPngData(std::vector<BYTE>& data);

    HBITMAP GetBitmap() const { return m_hBitmap; }

    void Clear();

private:
    bool GetEncoderClsid(const WCHAR* format, CLSID* pClsid);

private:
    HBITMAP m_hBitmap = nullptr;
};

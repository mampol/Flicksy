#include "GetFileVersion.h"

#include <vector>

#pragma comment(lib, "version.lib")

std::wstring GetFileVersion(const std::wstring& filePath)
{
    DWORD dummy = 0;

    DWORD size =
        GetFileVersionInfoSizeW(
            filePath.c_str(),
            &dummy);

    if (size == 0)
        return L"";

    std::vector<BYTE> buffer(size);

    if (!GetFileVersionInfoW(
        filePath.c_str(),
        0,
        size,
        buffer.data()))
    {
        return L"";
    }

    VS_FIXEDFILEINFO* info = nullptr;
    UINT infoSize = 0;

    if (!VerQueryValueW(
        buffer.data(),
        L"\\",
        reinterpret_cast<void**>(&info),
        &infoSize))
    {
        return L"";
    }

    if (!info)
        return L"";

    return std::to_wstring(HIWORD(info->dwFileVersionMS)) + L"." +
        std::to_wstring(LOWORD(info->dwFileVersionMS)) + L"." +
        std::to_wstring(HIWORD(info->dwFileVersionLS)) + L"." +
        std::to_wstring(LOWORD(info->dwFileVersionLS));
}
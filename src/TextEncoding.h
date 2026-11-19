#pragma once

#include <Windows.h>
#include <string>
#include <vector>

namespace TextEncoding
{
    std::wstring BytesToHex(const std::vector<UCHAR>& data);
    std::vector<UCHAR> HexToBytes(const std::wstring& hex);
    std::wstring Utf8ToWide(const std::string& src);
    std::wstring SJisToWide(const std::string& src);

    std::string WideToUtf8(const std::wstring& src);
    std::string SjisToUtf8(const std::string& src);
}
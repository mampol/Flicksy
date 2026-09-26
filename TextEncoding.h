#pragma once

#include <string>

namespace TextEncoding
{
    std::wstring Utf8ToUtf16(const std::string& src);
    std::wstring SjisToUtf16(const std::string& src);

    std::string Utf16ToUtf8(const std::wstring& src);
    std::string SjisToUtf8(const std::string& src);
}
#include "TextEncoding.h"

#include <Windows.h>

namespace TextEncoding
{
    std::wstring Utf8ToUtf16(const std::string& src)
    {
        if (src.empty()) return L"";

        int len = MultiByteToWideChar(CP_UTF8,
            0,
            src.c_str(),
            static_cast<int>(src.size()),
            nullptr,
            0
        );

        if (len <= 0)
            return L"";

        std::wstring dst(len, L'\0');

        MultiByteToWideChar(CP_UTF8,
            0,
            src.c_str(),
            static_cast<int>(src.size()),
            dst.data(),
            len
        );

        return dst;
    }

    std::wstring SjisToUtf16(const std::string& src)
    {
        if (src.empty()) return L"";

        constexpr UINT SJIS_CODEPAGE = 932;

        int len = MultiByteToWideChar(SJIS_CODEPAGE,
            0,
            src.c_str(),
            static_cast<int>(src.size()),
            nullptr,
            0
        );

        if (len <= 0)
            return L"";

        std::wstring dst(len, L'\0');

        MultiByteToWideChar(SJIS_CODEPAGE,
            0,
            src.c_str(),
            static_cast<int>(src.size()),
            dst.data(),
            len
        );

        return dst;
    }

    std::string Utf16ToUtf8(const std::wstring& src)
    {
        if (src.empty()) return "";

        int len = WideCharToMultiByte(CP_UTF8,
            0,
            src.c_str(),
            static_cast<int>(src.size()),
            nullptr,
            0,
            nullptr,
            nullptr
        );

        if (len <= 0)
            return "";

        std::string dst(len, '\0');

        WideCharToMultiByte(CP_UTF8,
            0,
            src.c_str(),
            static_cast<int>(src.size()),
            dst.data(),
            len,
            nullptr,
            nullptr
        );

        return dst;
    }

    std::string SjisToUtf8(const std::string& src)
    {
        return Utf16ToUtf8(SjisToUtf16(src));
    }
}

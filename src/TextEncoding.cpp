#include "TextEncoding.h"

namespace TextEncoding
{
    std::wstring BytesToHex(
        const std::vector<UCHAR>& data)
    {
        static constexpr wchar_t hex[] =
            L"0123456789ABCDEF";

        std::wstring result;
        result.reserve(data.size() * 2);

        for (UCHAR b : data)
        {
            result += hex[(b >> 4) & 0x0F];
            result += hex[b & 0x0F];
        }

        return result;
    }

    std::vector<UCHAR> HexToBytes(
        const std::wstring& hex)
    {
        auto hexValue = [](wchar_t c) -> int
            {
                if (c >= L'0' && c <= L'9')
                    return c - L'0';

                if (c >= L'A' && c <= L'F')
                    return c - L'A' + 10;

                if (c >= L'a' && c <= L'f')
                    return c - L'a' + 10;

                return -1;
            };

        std::vector<UCHAR> result;

        if ((hex.size() % 2) != 0)
            return result;

        result.reserve(hex.size() / 2);

        for (size_t i = 0; i < hex.size(); i += 2)
        {
            int hi = hexValue(hex[i]);
            int lo = hexValue(hex[i + 1]);

            if (hi < 0 || lo < 0)
                return {};

            result.push_back(
                static_cast<UCHAR>((hi << 4) | lo));
        }

        return result;
    }

    std::wstring Utf8ToWide(const std::string& src)
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

    std::wstring SJisToWide(const std::string& src)
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

    std::string WideToUtf8(const std::wstring& src)
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
        return WideToUtf8(SJisToWide(src));
    }
}

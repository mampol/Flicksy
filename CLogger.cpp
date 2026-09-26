#include "CLogger.h"

CLogger::CLogger()
{
    m_LogPath = GetLogFilePath();
}

CLogger::~CLogger()
{
}

#ifdef _UNICODE
void CLogger::Write(LogType type, const SYSTEMTIME& st, const std::wstring& text)
{
    if (!m_enabled) return;

    RotateIfNeeded();

    wchar_t timestamp[64] = {};
    swprintf_s(
        timestamp,
        L"%04d-%02d-%02d %02d:%02d:%02d",
        st.wYear,
        st.wMonth,
        st.wDay,
        st.wHour,
        st.wMinute,
        st.wSecond
    );

    std::wstring typetext = GetTypeText(type);
    std::wstring line =
        std::wstring(timestamp) +
        L" [" +
        typetext +
        L"] " +
        text +
        L"\r\n";

    std::string utf8 = TextEncoding::Utf16ToUtf8(line);

    std::ofstream ofs(m_LogPath,
        std::ios::binary | std::ios::app);

    if (!ofs) return;

    ofs.write(utf8.data(),
        static_cast<std::streamsize>(utf8.size())
    );
}
#else
void CLogger::Write(LogType type, const SYSTEMTIME& st, const std::string& text)
{
    if (!m_enabled) return;

    RotateIfNeeded();

    wchar_t timestamp[64] = {};
    swprintf_s(
        timestamp,
        L"%04d-%02d-%02d %02d:%02d:%02d",
        st.wYear,
        st.wMonth,
        st.wDay,
        st.wHour,
        st.wMinute,
        st.wSecond
    );

    std::string typetext = GetTypeText(type);
    std::string line =
        std::string(timestamp) +
        " [" +
        typetext +
        "] " +
        text +
        L"\r\n";

    std::string utf8 = TextEncoding::SjisToUtf8(line);

    std::ofstream ofs(m_LogPath,
        std::ios::binary | std::ios::app);

    if (!ofs) return;

    ofs.write(utf8.data(),
        static_cast<std::streamsize>(utf8.size())
    );
}
#endif

std::filesystem::path CLogger::GetLogFilePath() const
{
    TCHAR path[MAX_PATH] = {};
    DWORD len = GetEnvironmentVariable(_T("LOCALAPPDATA"),
        path,
        static_cast<DWORD>(_countof(path)));

    if (len == 0 || len >= _countof(path))
    {
        return std::filesystem::path(_T("Flicksy.log"));
    }

    std::filesystem::path logDir =
        std::filesystem::path(path) / _T("Flicksy") / _T("logs");

    std::error_code ec;
    std::filesystem::create_directories(logDir, ec);

    if (ec)
    {
        return std::filesystem::path(_T("Flicksy.log"));
    }

    return logDir / _T("Flicksy.log");
}

TCHAR* CLogger::GetTypeText(LogType type) const
{
    static TCHAR typeText[9];
    switch (type)
    {
    case LogType::Server:
        _tcscpy_s(typeText, _countof(typeText), _T("SERVER"));
        break;

    case LogType::Input:
        _tcscpy_s(typeText, _countof(typeText), _T("INPUT"));
        break;

    case LogType::Error:
        _tcscpy_s(typeText, _countof(typeText), _T("ERROR"));
        break;

    case LogType::Info:
    default:
        _tcscpy_s(typeText, _countof(typeText), _T("INFO"));
        break;
    }
    return typeText;
}

void CLogger::RotateIfNeeded()
{
    std::error_code ec;

    if (!std::filesystem::exists(m_LogPath, ec)) return;

    const auto size =
        std::filesystem::file_size(m_LogPath, ec);

    if (ec)
        return;

    if (size >= m_maxFileSize) Rotate();
}

void CLogger::Rotate()
{
    std::error_code ec;

    for (int i = m_maxBackupCount; i >= 1; --i)
    {
        std::filesystem::path src;

        if (i == 1)
        {
            src = m_LogPath;
        }
        else
        {
            src = m_LogPath.parent_path() /
                (m_LogPath.stem().wstring() +
                    L"." +
                    std::to_wstring(i - 1) +
                    m_LogPath.extension().wstring());
        }

        auto dst = m_LogPath.parent_path() /
            (m_LogPath.stem().wstring() +
                _T(".") +
                std::to_wstring(i) +
                m_LogPath.extension().wstring());

        if (!std::filesystem::exists(src, ec)) continue;

        if (std::filesystem::exists(dst, ec)) std::filesystem::remove(dst, ec);

        ec.clear();

        std::filesystem::rename(src, dst, ec);
    }
}
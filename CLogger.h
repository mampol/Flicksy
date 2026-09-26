#pragma once

#include <windows.h>
#include <tchar.h>
#include <string>
#include <fstream>
#include <filesystem>
#include "TextEncoding.h"

enum class LogType {
    Info,
    Server,
    Input,
    Error
};

class CLogger
{
public:
    CLogger();
    ~CLogger();

#ifdef _UNICODE
    void Write(LogType type, const SYSTEMTIME& st, const std::wstring& text);
#else
    void Write(LogType type, const SYSTEMTIME& st, const std::string& text);
#endif

    void SetEnabled(bool enabled) { m_enabled = enabled; }
    bool IsEnabled() const { return m_enabled; }

private:
    std::filesystem::path GetLogFilePath() const;
    TCHAR* GetTypeText(LogType type) const;
    void RotateIfNeeded();
    void Rotate();

private:

    std::filesystem::path m_LogPath;

    bool m_enabled = true;

#ifdef _DEBUG
    std::uintmax_t m_maxFileSize = 255;
#else
    std::uintmax_t m_maxFileSize = 10 * 1024 * 1024;
#endif
    int m_maxBackupCount = 3;
};
#pragma once

#include <filesystem>
#include <string>

class CFlicksyConfig
{
public:
    enum class InputMode
    {
        SendInput,
        Clipboard
    };

    enum class Theme
    {
        Light,
        Dark
    };

public:
    CFlicksyConfig();

    void SetDefault();

    bool Load(const std::filesystem::path& file);
    bool Save(const std::filesystem::path& file) const;

    int ServerPort() const { return m_port; }
    void SetServerPort(int port);

    const std::wstring& WebRoot() const { return m_webRoot; }
    void SetWebRoot(const std::wstring& webRoot) { m_webRoot = webRoot; }

    bool AutoStartServer() const { return m_autoStartServer; }
    void SetAutoStartServer(bool value) { m_autoStartServer = value; }

    InputMode GetInputMode() const { return m_inputMode; }
    void SetInputMode(InputMode mode) { m_inputMode = mode; }

    int GetInputInterval() { return m_inputinterval; }
    void SetInputInterval(const int ninputinterval) { m_inputinterval = ninputinterval; }

    bool AlwaysOnTop() const { return m_alwaysOnTop; }
    void SetAlwaysOnTop(bool value) { m_alwaysOnTop = value; }

    bool StartMinimized() const { return m_startMinimized; }
    void SetStartMinimized(bool value) { m_startMinimized = value; }

    Theme GetTheme() const { return m_theme; }
    void SetTheme(Theme theme) { m_theme = theme; }

private:
    static std::string WideToUtf8(const std::wstring& text);
    static std::wstring Utf8ToWide(const std::string& text);

    static const char* InputModeToString(InputMode mode);
    static InputMode StringToInputMode(const std::string& value);

    static const char* ThemeToString(Theme theme);
    static Theme StringToTheme(const std::string& value);

private:
    int          m_port;
    std::wstring m_webRoot;
    bool         m_autoStartServer;

    InputMode    m_inputMode;
    int          m_inputinterval;

    bool         m_alwaysOnTop;
    bool         m_startMinimized;

    Theme        m_theme;
};

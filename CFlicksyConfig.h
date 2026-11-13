#pragma once

#include <filesystem>
#include <vector>
#include <string>

#define REG_STARTUP				_T("Software\\Microsoft\\Windows\\CurrentVersion\\Run")
#define REG_VALUE				_T("Flicksy")

bool SetRunAtStartup(bool enabled);
bool IsRunAtStartup();

enum class ActionType
{
    Url, 
    Application,
    Folder,
    Lock,
    Shutdown,
    None
};

struct ActionConfig
{
    ActionType type = ActionType::Url;
    bool enabled = true;
    std::wstring name;
    std::wstring value;
};

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
    bool Save() const;

    int ServerPort() const { return m_port; }
    void SetServerPort(int port);

    static std::wstring ActionTypeToString(const ActionType type);
    ActionType StringToActionType(const std::string& type);

    const std::wstring& WebRoot() const { return m_webRoot; }
    void SetWebRoot(const std::wstring& webRoot) { m_webRoot = webRoot; }

    bool AutoStartServer() const { return m_autoStartServer; }
    void SetAutoStartServer(bool value) { m_autoStartServer = value; }

    const size_t GetActionSize() { return m_actions.size(); };
    const ActionConfig& GetAction(const size_t actionindex) const { return m_actions.at(actionindex); };
    const std::vector<ActionConfig>& GetActions() const { return m_actions; };
    void SetAction(const size_t actionindex, const ActionConfig& action) { m_actions.at(actionindex) = action; };
    size_t AddAction(const ActionConfig& action)
    {
        m_actions.push_back(action);
        return m_actions.size() - 1;
    };
    void DeleteAction(size_t index) { m_actions.erase(m_actions.begin() + index); };

    const std::wstring& GetDataFolder() const { return m_dataFolder; }
    void SetDataFolder(const std::wstring& dataFolder) { m_dataFolder = dataFolder; }

    InputMode GetInputMode() const { return m_inputMode; }
    void SetInputMode(InputMode mode) { m_inputMode = mode; }

    int GetInputInterval() const { return m_inputinterval; }
    void SetInputInterval(const int ninputinterval) { m_inputinterval = ninputinterval; }

    bool GetLogEnable() const { return m_log_enable; }
    void SetLogEnable(const bool log_enable) { m_log_enable = log_enable; }

    const std::wstring& GetLogFile() const { return m_log_file; }
    void SetLogFile(const std::wstring& log_file) { m_log_file = log_file; }

    bool GetLogInput() const { return m_log_input; }
    void SetLogInput(const bool log_input) { m_log_input = log_input; }

    bool AlwaysOnTop() const { return m_alwaysOnTop; }
    void SetAlwaysOnTop(bool value) { m_alwaysOnTop = value; }

    bool StartMinimized() const { return m_startMinimized; }
    void SetStartMinimized(bool value) { m_startMinimized = value; }

    Theme GetTheme() const { return m_theme; }
    void SetTheme(Theme theme) { m_theme = theme; }

private:
    static std::string WideToUtf8(const std::wstring& text);
    static std::wstring Utf8ToWide(const std::string& text);

    static const char* ThemeToString(Theme theme);
    static Theme StringToTheme(const std::string& value);

    static const char* InputModeToString(InputMode mode);
    static InputMode StringToInputMode(const std::string& value);

private:
    std::wstring m_FlicksyConfig;

    int          m_port;
    std::wstring m_webRoot;
    bool         m_autoStartServer;

    std::vector<ActionConfig> m_actions;

    std::wstring m_dataFolder;

    InputMode    m_inputMode;
    int          m_inputinterval;
    bool         m_log_enable;
    std::wstring m_log_file;
    bool         m_log_input;

    bool         m_alwaysOnTop;
    bool         m_startMinimized;

    Theme        m_theme;
};

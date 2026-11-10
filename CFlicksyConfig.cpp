#include "CFlicksyConfig.h"

#include <Windows.h>
#include <tchar.h>
#include <algorithm>
#include <cctype>
#include <fstream>
#include <string>
#include <toml.hpp>

bool SetRunAtStartup(bool enabled)
{
    HKEY hKey = nullptr;

    if (RegOpenKeyEx(HKEY_CURRENT_USER,
        REG_STARTUP,
        0,
        KEY_SET_VALUE,
        &hKey) != ERROR_SUCCESS)
    {
        return false;
    }

    LONG result;
    if (enabled)
    {
        TCHAR exePath[MAX_PATH] = {};
        GetModuleFileName(
            nullptr,
            exePath,
            _countof(exePath));

        std::basic_string<TCHAR> value = _T("\"");
        value += exePath;
        value += _T("\"");

        result = RegSetValueEx(hKey,
            REG_VALUE,
            0,
            REG_SZ,
            reinterpret_cast<const BYTE*>(value.c_str()),
            static_cast<DWORD>((value.size() + 1) * sizeof(TCHAR)));
    }
    else
    {
        result = RegDeleteValue(hKey,
            REG_VALUE);
        if (result == ERROR_FILE_NOT_FOUND) result = ERROR_SUCCESS;
    }

    RegCloseKey(hKey);

    return result == ERROR_SUCCESS;
}

bool IsRunAtStartup()
{
    HKEY hKey = nullptr;

    if (RegOpenKeyEx(HKEY_CURRENT_USER,
        REG_STARTUP,
        0,
        KEY_QUERY_VALUE,
        &hKey) != ERROR_SUCCESS)
    {
        return false;
    }

    TCHAR exePath[MAX_PATH] = {};
    DWORD type = 0;
    DWORD size = sizeof(exePath);

    LONG result = RegQueryValueEx(hKey,
        REG_VALUE,
        nullptr,
        &type,
        reinterpret_cast<BYTE*>(exePath),
        &size);

    RegCloseKey(hKey);

    if (result != ERROR_SUCCESS || type != REG_SZ)
        return false;

    TCHAR cmpPath[MAX_PATH] = {};
    GetModuleFileName(nullptr, cmpPath, _countof(cmpPath));
    std::basic_string<TCHAR> value = _T("\"");
    value += cmpPath;
    value += _T("\"");

    return _tcscmp(exePath, value.c_str()) == 0;
}

CFlicksyConfig::CFlicksyConfig()
{
    SetDefault();
}

void CFlicksyConfig::SetDefault()
{
    m_port = 10000;
    m_webRoot = L"./webroot";
    m_autoStartServer = false;

    m_actions.clear();

    m_dataFolder = L"./data";

    m_inputMode = InputMode::SendInput;
    m_inputinterval = 5;

    m_log_enable = true;
    m_log_file = L"./flicksy.log";
    m_log_input = false;

    m_alwaysOnTop = false;
    m_startMinimized = false;

    m_theme = Theme::Light;
}

void CFlicksyConfig::SetServerPort(int port)
{
    if (port < 1)
        port = 1;
    else if (port > 65535)
        port = 65535;

    m_port = port;
}

std::string CFlicksyConfig::ActionTypeToString(const ActionType type)
{
    switch (type)
    {
    case ActionType::Url:
        return "url";

    case ActionType::Application:
        return "application";

    case ActionType::Folder:
        return "folder";

    case ActionType::Lock:
        return "lock";

    case ActionType::Shutdown:
        return "shutdown";

    default:
        return "none";
    }
}

ActionType CFlicksyConfig::StringToActionType(const std::string& type)
{
    if (type == "url")
        return ActionType::Url;

    if (type == "application")
        return ActionType::Application;

    if (type == "folder")
        return ActionType::Folder;

    if (type == "lock")
        return ActionType::Lock;

    if (type == "shutdown")
        return ActionType::Shutdown;

    return ActionType::None;
}

bool CFlicksyConfig::Load(const std::filesystem::path& file)
{
    m_FlicksyConfig = file;

    SetDefault();

    if (!std::filesystem::exists(file))
    {
        return Save(file);
    }

    try
    {
        //const auto fullPath = std::filesystem::absolute(file);

        std::ifstream ifs(file, std::ios::binary);
        if (!ifs)
            return false;

        const std::string sourcePath = file.u8string();
        toml::table tbl = toml::parse(ifs, sourcePath);

        if (auto server = tbl["server"].as_table())
        {
            SetServerPort((int)(*server)["port"].value_or<int64_t>(10000));

            const std::string webRoot =
                (*server)["web_root"].value_or<std::string>("./webroot");
            m_webRoot = Utf8ToWide(webRoot);

            m_autoStartServer =
                (*server)["auto_start"].value_or(false);
        }

        m_actions.clear();
        if (auto* actions = tbl["action"].as_array())
        {
            for (auto&& node : *actions)
            {
                auto* item = node.as_table();
                if (!item)
                    continue;
                ActionConfig action{};
                action.enabled =
                    (*item)["enabled"].value_or(true);
                action.name =
                    Utf8ToWide(
                        (*item)["name"].value_or(std::string{}));
                action.type =
                    StringToActionType(
                        (*item)["type"].value_or(std::string{ "none" }));
                action.value =
                    Utf8ToWide(
                        (*item)["value"].value_or(std::string{}));
                m_actions.push_back(std::move(action));
            }
        }

        if (auto data = tbl["data"].as_table())
        {
            const std::string dataFolder =
                (*data)["data_folder"].value_or<std::string>("");
            m_dataFolder = Utf8ToWide(dataFolder);
        }

        if (auto input = tbl["input"].as_table())
        {
            const std::string mode =
                (*input)["mode"].value_or<std::string>("sendinput");
            m_inputMode = StringToInputMode(mode);

            SetInputInterval((int)(*input)["interval"].value_or<int64_t>(5));
        }

        if (auto input = tbl["log"].as_table())
        {
            m_log_enable =
                (*input)["log_enable"].value_or(true);

            const std::string logfile =
                (*input)["log_file"].value_or<std::string>("");
            m_log_file = Utf8ToWide(logfile);

            m_log_input =
                (*input)["log_input"].value_or(false);
        }

        if (auto window = tbl["window"].as_table())
        {
            m_alwaysOnTop =
                (*window)["always_on_top"].value_or(false);

            m_startMinimized =
                (*window)["start_minimized"].value_or(false);
        }

        if (auto appearance = tbl["appearance"].as_table())
        {
            const std::string theme =
                (*appearance)["theme"].value_or<std::string>("light");
            m_theme = StringToTheme(theme);
        }

        return true;
    }
    catch (const toml::parse_error&)
    {
        SetDefault();
        return false;
    }
    catch (...)
    {
        SetDefault();
        return false;
    }
}

bool CFlicksyConfig::Save() const
{
    return Save(m_FlicksyConfig);
}

bool CFlicksyConfig::Save(const std::filesystem::path& file) const
{
    try
    {
        toml::table tbl;

        tbl.insert("server", toml::table{
            { "port", m_port },
            { "web_root", WideToUtf8(m_webRoot) },
            { "auto_start", m_autoStartServer }
        });

        toml::array actions;
        for (const auto& action : m_actions)
        {
            toml::table item;
            item.insert("type", ActionTypeToString(action.type));
            item.insert("enabled", action.enabled);
            item.insert("name", WideToUtf8(action.name));
            item.insert("value", WideToUtf8(action.value));
            actions.push_back(std::move(item));
        }
        tbl.insert("action", std::move(actions));

        tbl.insert("data", toml::table{
            { "data_folder", WideToUtf8(m_dataFolder) }
        });

        tbl.insert("input", toml::table{
            { "mode", InputModeToString(m_inputMode) },
            { "interval", m_inputinterval }
        });

        tbl.insert("log", toml::table{
            { "log_enable", m_log_enable },
            { "log_file", WideToUtf8(m_log_file) },
            { "log_input", m_log_input }
        });

        tbl.insert("window", toml::table{
            { "always_on_top", m_alwaysOnTop },
            { "start_minimized", m_startMinimized }
        });

        tbl.insert("appearance", toml::table{
            { "theme", ThemeToString(m_theme) }
        });

        const std::filesystem::path parent = file.parent_path();
        if (!parent.empty())
            std::filesystem::create_directories(parent);

        std::ofstream ofs(file, std::ios::binary | std::ios::trunc);
        if (!ofs)
            return false;

        ofs << tbl;
        return ofs.good();
    }
    catch (...)
    {
        return false;
    }
}

std::string CFlicksyConfig::WideToUtf8(const std::wstring& text)
{
    if (text.empty())
        return {};

    const int size = WideCharToMultiByte(
        CP_UTF8, 0,
        text.data(), (int)text.size(),
        nullptr, 0,
        nullptr, nullptr);

    if (size <= 0)
        return {};

    std::string result(size, '\0');

    WideCharToMultiByte(
        CP_UTF8, 0,
        text.data(), (int)text.size(),
        result.data(), size,
        nullptr, nullptr);

    return result;
}

std::wstring CFlicksyConfig::Utf8ToWide(const std::string& text)
{
    if (text.empty())
        return {};

    const int size = MultiByteToWideChar(
        CP_UTF8, 0,
        text.data(), (int)text.size(),
        nullptr, 0);

    if (size <= 0)
        return {};

    std::wstring result(size, L'\0');

    MultiByteToWideChar(
        CP_UTF8, 0,
        text.data(), (int)text.size(),
        result.data(), size);

    return result;
}

const char* CFlicksyConfig::InputModeToString(InputMode mode)
{
    switch (mode)
    {
    case InputMode::Clipboard:
        return "clipboard";

    case InputMode::SendInput:
    default:
        return "sendinput";
    }
}

CFlicksyConfig::InputMode CFlicksyConfig::StringToInputMode(const std::string& value)
{
    std::string s = value;
    std::transform(s.begin(), s.end(), s.begin(),
        [](unsigned char c) { return (char)std::tolower(c); });

    if (s == "clipboard")
        return InputMode::Clipboard;

    return InputMode::SendInput;
}

const char* CFlicksyConfig::ThemeToString(Theme theme)
{
    switch (theme)
    {
    case Theme::Dark:
        return "dark";

    case Theme::Light:
    default:
        return "light";
    }
}

CFlicksyConfig::Theme CFlicksyConfig::StringToTheme(const std::string& value)
{
    std::string s = value;
    std::transform(s.begin(), s.end(), s.begin(),
        [](unsigned char c) { return (char)std::tolower(c); });

    if (s == "dark")
        return Theme::Dark;

    return Theme::Light;
}

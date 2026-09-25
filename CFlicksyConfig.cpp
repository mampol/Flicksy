#include "CFlicksyConfig.h"

#include <Windows.h>
#include <algorithm>
#include <cctype>
#include <fstream>
#include <string>
#include <toml.hpp>

CFlicksyConfig::CFlicksyConfig()
{
    SetDefault();
}

void CFlicksyConfig::SetDefault()
{
    m_port = 10000;
    m_webRoot = L"./webroot";
    m_autoStartServer = false;

    m_inputMode = InputMode::SendInput;

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

bool CFlicksyConfig::Load(const std::filesystem::path& file)
{
    SetDefault();

    try
    {
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

        if (auto input = tbl["input"].as_table())
        {
            const std::string mode =
                (*input)["mode"].value_or<std::string>("sendinput");
            m_inputMode = StringToInputMode(mode);

            SetInputInterval((int)(*input)["interval"].value_or<int64_t>(5));
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

        tbl.insert("input", toml::table{
            { "mode", InputModeToString(m_inputMode) },
            { "interval", m_inputinterval }
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

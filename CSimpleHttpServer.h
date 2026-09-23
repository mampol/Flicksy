#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <winsock2.h>
#include <ws2tcpip.h>
#include <Windows.h>

#include <thread>
#include <mutex>
#include <queue>
#include <string>
#include <atomic>
#include <fstream>
#include <sstream>
#include <filesystem>

#include "httplib.h"

#define UM_HTTPPOPMSG   (WM_APP+1)
#define UM_HTTPPOPKEY   (WM_APP+2)
#define UM_HTTPSTATE    (WM_APP+3)
#define UM_HTTPLOG      (WM_APP+4)

enum class ServerState
{
    Stopped,
    Starting,
    Running,
    Stopping,
    Error
};

class CSimpleHttpServer
{
public:
    CSimpleHttpServer();
    ~CSimpleHttpServer();

    bool Start(HWND hMainWnd, int port, const std::filesystem::path& webRoot);
    void Stop();

    int GetPort() { return m_port; };
    const std::filesystem::path& GetWebRoot() const
    {
        return m_webRoot;
    }

    ServerState GetState() const
    {
        return m_state.load();
    }

    bool IsRunning() const
    {
        return GetState() == ServerState::Running;
    }

    const std::string& GetToken() const { return m_token; }

    bool PopMessage(std::string& message);
    bool PopKey(WORD& vk);
    bool PopLog(std::string& log);

    const std::string& GetLastError() const { return m_lastError; }

private:
//    std::string GetExeDirectory();
    std::filesystem::path GetExeDir()
    {
        wchar_t path[MAX_PATH] = {};
        GetModuleFileName(nullptr,
            path,
            _countof(path));
        return std::filesystem::path(path).parent_path();
    }
    std::string LoadTextFile(const std::string& path);
    std::string GenerateToken();
    bool CheckToken(const httplib::Request& req, httplib::Response& res);
    void ServerThread(int port);
    void PostMsg(const UINT msg)
    {
        if (m_hMainWnd) PostMessage(m_hMainWnd, msg, 0, 0);
    }
    std::string GetKeyName(WORD vk)
    {
        switch (vk)
        {
        case VK_RETURN: return "(ENTER)";
        case VK_ESCAPE: return "(ESC)";
        case VK_TAB:    return "(TAB)";
        case VK_BACK:   return "(BS)";
        case VK_LEFT:   return "(LEFT)";
        case VK_RIGHT:  return "(RIGHT)";
        case VK_UP:     return "(UP)";
        case VK_DOWN:   return "(DOWN)";
        }
        return "";
    }
    void AddServerLog(const std::string& log)
    {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_logs.push(log);
        }
        PostMsg(UM_HTTPLOG);
    }
    std::string GetHttpStatusText(int status)
    {
        switch (status)
        {
        case 200: return "OK";
        case 400: return "Bad Request";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 500: return "Internal Server Error";
        default:  return "";
        }
    }
    void AddHttpLog(const httplib::Request& req, int status)
    {
        std::string log = "http " + std::to_string(status) + " " + GetHttpStatusText(status);
        AddServerLog(log);
    }
    void FormatError()
    {
        int err = WSAGetLastError();
        switch (err)
        {
        case WSAEADDRINUSE:
            m_lastError = "The port is already in use.";
            break;

        case WSAEACCES:
            m_lastError = "Access to the port was denied.";
            break;

        case WSAEADDRNOTAVAIL:
            m_lastError = "The specified address is not available.";
            break;

        default:
            m_lastError =
                "Failed to start the HTTP server. Error code: " + std::to_string(err);
            break;
        }
    }
private:
    std::string m_token;

    std::atomic<ServerState> m_state = ServerState::Stopped;

    HWND m_hMainWnd = NULL;
    int m_port = 0;
    std::filesystem::path m_webRoot;

    httplib::Server m_server;
    std::thread m_thread;

    std::queue<std::string> m_messages;
    std::queue<WORD> m_keys;
    std::queue<std::string> m_logs;
    std::mutex m_mutex;

    std::string m_lastError;
};

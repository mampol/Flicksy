#pragma once

#include <thread>
#include <mutex>
#include <queue>
#include <string>
#include <atomic>
#include <fstream>
#include <sstream>

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

    bool Start(HWND hMainWnd, int port = 10000);
    void Stop();

    int GetPort() { return m_port; };

    ServerState GetState() const
    {
        return m_state.load();
    }

    bool IsRunning() const
    {
        return GetState() == ServerState::Running;
    }

    bool PopMessage(std::string& message);
    bool PopKey(WORD& vk);
    bool PopLog(std::string& log);

private:
    int m_port = 0;
    std::string GetExeDirectory();
    std::string LoadTextFile(const std::string& path);
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
    void AddServerLog(std::string& log)
    {
        m_logs.push(log);
        PostMsg(UM_HTTPLOG);
    }

private:
    std::atomic<ServerState> m_state = ServerState::Stopped;

    HWND m_hMainWnd = NULL;

    httplib::Server m_server;
    std::thread m_thread;

    std::queue<std::string> m_messages;
    std::queue<WORD> m_keys;
    std::queue<std::string> m_logs;
    std::mutex m_mutex;
};

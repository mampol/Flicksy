#include "CSimpleHttpServer.h"

CSimpleHttpServer::CSimpleHttpServer()
{
    /*
    m_server.Get("/", [this](const httplib::Request& req, httplib::Response& res)
        {
            std::string log =
                "[" + req.remote_addr + "]  " + req.method + " " + req.path + " (flicksy.html)";
            AddServerLog(log);

            std::string htmlpath = GetExeDirectory() + "\\flicksy.html";
            std::string html = LoadTextFile(htmlpath);

            if (html.empty())
            {
                res.status = 404;
                res.set_content(
                    htmlpath + " not found",
                    "text/plain; charset=UTF-8");
                return;
            }

            res.set_content(
                html,
                "text/html; charset=UTF-8");

            res.status = 200;
        });
        */
    m_server.Get(R"(/|/.*\.(html|png|jpg|webp|css|js))",
        [this](const httplib::Request& req,
            httplib::Response& res)
        {
            if (!CheckToken(req, res)) return;

            std::string log =
                "[" + req.remote_addr + "] " + req.method + " " + req.path;
            AddServerLog(log);

            std::string path = req.path;
            if (path == "/") path = "/flicksy.html";
            if (path.find("..") != std::string::npos)
            {
                res.set_content("Flicksy http 403 error", "text/html");
                res.status = 403;
                AddHttpLog(req, res.status);
                return;
            }

            std::filesystem::path filePath = m_webRoot / path.substr(1);
            std::ifstream ifs(filePath, std::ios::binary);
            if (!ifs)
            {
                res.status = 404;
                res.set_content("Flicksy http 404 error", "text/html");
                AddHttpLog(req, res.status);
                return;
            }

            std::string data((std::istreambuf_iterator<char>(ifs)),
                std::istreambuf_iterator<char>());
            std::string ext = filePath.extension().string();
            std::string contentType = "application/octet-stream";
            if (ext == ".html")
                contentType = "text/html; charset=UTF-8";
            else if (ext == ".png")
                contentType = "image/png";
            else if (ext == ".jpg")
                contentType = "image/jpg";
            else if (ext == ".webp")
                contentType = "image/webp";
            else if (ext == ".css")
                contentType = "text/css; charset=UTF-8";
            else if (ext == ".js")
                contentType = "application/javascript; charset=UTF-8";
            res.status = 200;
            res.set_content(std::move(data), contentType);

            AddHttpLog(req, res.status);
        });

    m_server.Post(
        "/input",
        [this](const httplib::Request& req,
            httplib::Response& res)
        {
            if (!CheckToken(req, res)) return;

            if (!req.has_param("text"))
            {
                res.status = 400;
                res.set_content("Flicksy http 400 error", "text/html");
                AddHttpLog(req, res.status);
                return;
            }

            std::string text = req.get_param_value("text");

            std::string log =
                "[" + req.remote_addr + "] "
                "POST /input text=" + text;
            AddServerLog(log);

            {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_messages.push(text);
            }

            PostMsg(UM_HTTPPOPMSG);

            res.status = 200;
            //AddHttpLog(req, res.status);
        });

    m_server.Post("/key", [this](const httplib::Request& req, httplib::Response& res)
        {
            if (!CheckToken(req, res)) return;

            if (!req.has_param("vk"))
            {
                res.status = 400;
                return;
            }

            int vk = std::stoi(req.get_param_value("vk"));

            std::string log =
                "[" + req.remote_addr + "] " + "POST /key vk=" + std::to_string(vk) + GetKeyName(vk);
            AddServerLog(log);

            if (vk < 0 || vk > 0xFF)
            {
                res.status = 400;
                AddHttpLog(req, res.status);
                return;
            }

            {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_keys.push(static_cast<WORD>(vk));
            }

            PostMsg(UM_HTTPPOPKEY);

            res.status = 200;
            //AddHttpLog(req, res.status);
        });
}

CSimpleHttpServer::~CSimpleHttpServer()
{
    Stop();
}

/*
std::string CSimpleHttpServer::GetExeDirectory()
{
    char path[MAX_PATH] = {};

    GetModuleFileNameA(NULL, path, MAX_PATH);

    std::string fullPath(path);

    size_t pos = fullPath.find_last_of("\\/");

    if (pos == std::string::npos)
        return "";

    return fullPath.substr(0, pos);
}
*/

std::string CSimpleHttpServer::LoadTextFile(const std::string& path)
{
    std::ifstream ifs(path, std::ios::binary);

    if (!ifs)
        return {};

    std::ostringstream oss;
    oss << ifs.rdbuf();

    return oss.str();
}

std::string CSimpleHttpServer::GenerateToken()
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(0, 15);

    const char* hex = "0123456789abcdef";

    std::string token;
    token.reserve(32);

    for (int i = 0; i < 32; ++i)
        token += hex[dist(gen)];

    return token;
}

bool CSimpleHttpServer::CheckToken(const httplib::Request& req, httplib::Response& res)
{
    if (!req.has_param("token"))
    {
        res.status = 403;
        res.set_content("Forbidden", "text/plain");
        return false;
    }

    if (req.get_param_value("token") != m_token)
    {
        res.status = 403;
        res.set_content("Forbidden", "text/plain");
        return false;
    }

    return true;
}

bool CSimpleHttpServer::Start(HWND hMainWnd, int port, const std::filesystem::path& webRoot)
{
    if (m_thread.joinable())
    {
        if (m_state == ServerState::Stopped || m_state == ServerState::Error)
        {
            m_thread.join();
        }
        else
        {
            return false;
        }
    }

    m_hMainWnd = hMainWnd;
    m_port = port;
    if (webRoot.is_relative())
    {
        m_webRoot = GetExeDir() / webRoot;
    }
    else
    {
        m_webRoot = webRoot;
    }

    m_token = GenerateToken();

    m_thread = std::thread(
        &CSimpleHttpServer::ServerThread,
        this,
        port);

    return true;
}

void CSimpleHttpServer::Stop()
{
    if (!m_thread.joinable())
    {
        m_state = ServerState::Stopped;
        PostMsg(UM_HTTPSTATE);
        return;
    }

    m_state = ServerState::Stopping;
//    PostMsg(UM_HTTPSTATE);

    m_server.stop();

    m_thread.join();

//    m_state = ServerState::Stopped;
    PostMsg(UM_HTTPSTATE);
}

void CSimpleHttpServer::ServerThread(int port)
{
    m_state = ServerState::Starting;
    PostMsg(UM_HTTPSTATE);

    int ret = m_server.bind_to_port("0.0.0.0", port);

    if (ret < 0)
    {
        m_state = ServerState::Error;
        PostMsg(UM_HTTPSTATE);
        return;
    }

    m_state = ServerState::Running;
    PostMsg(UM_HTTPSTATE);

    bool result = m_server.listen_after_bind();

    if (!result)
    {
        m_state = ServerState::Error;
    }
    else
    {
        m_state = ServerState::Stopped;
    }
//    PostMsg(UM_HTTPSTATE);
}

bool CSimpleHttpServer::PopMessage(std::string& message)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_messages.empty())
        return false;

    message = std::move(m_messages.front());
    m_messages.pop();

    return true;
}

bool CSimpleHttpServer::PopKey(WORD& vk)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_keys.empty())
        return false;

    vk = m_keys.front();
    m_keys.pop();

    return true;
}

bool CSimpleHttpServer::PopLog(std::string& log)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_logs.empty())
        return false;

    log = m_logs.front();
    m_logs.pop();

    return true;
}

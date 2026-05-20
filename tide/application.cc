#include "application.h"

#include <memory>
#include <functional>
#include <utility>

#include "iomanager.h"
#include "address.h"
#include "env.h"
#include "log.h"
#include "config.h"
#include "daemon.h"
#include "http/http_server.h"

namespace tide
{
    static tide::Logger::ptr g_logger = TIDE_LOG_NAME("system");

    // 定义一个全局配置变量，用于指定服务器的工作目录，默认值为/home/tide/work，并提供描述信息
    static tide::ConfigVar<std::string>::ptr g_server_work_path =
        tide::Config::Lookup<std::string>("server.work_path", "/home/dingjr/sourceCode/devSource/tide/conf", "server work path");

    // 定义一个全局配置变量，用于指定服务器的PID文件路径，默认值为tide.pid，并提供描述信息
    static tide::ConfigVar<std::string>::ptr g_server_pid_file =
        tide::Config::Lookup<std::string>("server.pid_file", "tide.pid", "server pid file");

    // 定义一个结构体，用于表示HTTP服务器的配置项，包括服务器监听的地址列表、连接保持活跃的标志、连接超时时间和服务器名称等字段
    struct HttpServerConfig
    {
        std::vector<std::string> address; // 服务器监听的地址列表，例如["0.0.0.0:8099"]
        int keepalive = 0;
        int timeout = 1000 * 2 * 60; // 连接超时时间，单位为毫秒，默认值为2分钟
        std::string name;

        bool isValid() const { return !address.empty(); }

        bool operator==(const HttpServerConfig &other) const
        {
            return address == other.address && keepalive == other.keepalive && timeout == other.timeout && name == other.name;
        }
    };

    // 模板的偏特化：将字符串转换为HttpServerConfig对象
    template <>
    class LexicalCast<std::string, HttpServerConfig>
    {
    public:
        HttpServerConfig operator()(const std::string &v)
        {
            YAML::Node node = YAML::Load(v);
            HttpServerConfig conf;
            conf.keepalive = node["keepalive"].as<int>(conf.keepalive);
            conf.timeout = node["timeout"].as<int>(conf.timeout);
            conf.name = node["name"].as<std::string>(conf.name);
            if (node["address"].IsDefined())
            {
                for (size_t i = 0; i < node["address"].size(); ++i)
                {
                    conf.address.push_back(node["address"][i].as<std::string>());
                }
            }
            return conf;
        }
    };

    // 模板的偏特化：将HttpServerConfig对象转换为字符串的函数对象。
    template <>
    class LexicalCast<HttpServerConfig, std::string>
    {
    public:
        std::string operator()(const HttpServerConfig &conf)
        {
            YAML::Node node;
            node["keepalive"] = conf.keepalive;
            node["timeout"] = conf.timeout;
            node["name"] = conf.name;
            for (size_t i = 0; i < conf.address.size(); ++i)
            {
                node["address"].push_back(conf.address[i]);
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    // 定义一个全局配置变量，用于指定HTTP服务器的配置列表，默认值为空列表，并提供描述信息
    static tide::ConfigVar<std::vector<HttpServerConfig>>::ptr g_http_server_conf =
        tide::Config::Lookup("http_servers", std::vector<HttpServerConfig>(), "http server config");

    Application *Application::s_Instance = nullptr;

    Application::Application()
    {
        s_Instance = this;
    }

    bool Application::init(int argc, char **argv)
    {
        m_argc = argc;
        m_argv = argv;

        // 加载帮助信息
        tide::EnvMgr::GetInstance()->addHelp("s", "start with the terminal");
        tide::EnvMgr::GetInstance()->addHelp("d", "run as a daemon");
        tide::EnvMgr::GetInstance()->addHelp("c", "config path, default: ./conf");
        tide::EnvMgr::GetInstance()->addHelp("p", "print help");

        // 解析命令行参数并加载到环境变量中，如果解析错误打印帮助手册
        if (!tide::EnvMgr::GetInstance()->init(argc, argv))
        {
            tide::EnvMgr::GetInstance()->printHelp();
            return false;
        }

        // 如果用户请求帮助信息，则打印帮助信息并退出
        if (tide::EnvMgr::GetInstance()->has("p"))
        {
            tide::EnvMgr::GetInstance()->printHelp();
            return false;
        }

        // 根据用户请求的运行方式进行相应的处理
        int run_type = 0;
        if (tide::EnvMgr::GetInstance()->has("s"))
        {
            run_type = 1;
        }

        if (tide::EnvMgr::GetInstance()->has("d"))
        {
            run_type = 2;
        }

        if (run_type == 0)
        {
            tide::EnvMgr::GetInstance()->printHelp();
            return false;
        }

        // 构造PID文件路径，格式为工作目录加上PID文件名，例如/home/tide/work/tide.pid
        std::string pidfile = g_server_work_path->getValue() + "/" + g_server_pid_file->getValue();

        // 检查PID文件是否存在且正在运行，如果是，则说明服务器已经在运行，打印错误日志并返回false
        if (tide::FSUtil::IsRunningPidfile(pidfile))
        {
            TIDE_LOG_ERROR(g_logger) << "server is already running:" << pidfile;
            return false;
        }

        // 获取配置文件路径，默认值为./conf，并将其转换为绝对路径，然后加载配置文件中的配置项到配置管理器中
        std::string conf_path = tide::EnvMgr::GetInstance()->getAbsolutePath(tide::EnvMgr::GetInstance()->get("c", "conf"));
        tide::Config::LoadFromConfDir(conf_path);

        // 创建工作目录，如果创建失败，则打印错误日志并返回false
        if (!tide::FSUtil::Mkdir(g_server_work_path->getValue()))
        {
            TIDE_LOG_ERROR(g_logger) << "create work path failed:" << g_server_work_path->getValue();
            return false;
        }

        return true;
    }

    bool Application::run()
    {
        int is_daemon = tide::EnvMgr::GetInstance()->has("d");

        return tide::start_daemon(m_argc, m_argv, std::bind(&Application::main, this, std::placeholders::_1, std::placeholders::_2), is_daemon);
    }

    int Application::main(int argc, char **argv)
    {
        std::string pidfile = g_server_work_path->getValue() + "/" + g_server_pid_file->getValue();

        std::ofstream ofs(pidfile);
        if (!ofs)
        {
            TIDE_LOG_ERROR(g_logger) << "open pidfile " << pidfile << " failed";
            return -1;
        }

        ofs << getpid();

        auto http_confs = g_http_server_conf->getValue();
        for (auto i : http_confs)
        {
            TIDE_LOG_INFO(g_logger) << "\n" << LexicalCast<HttpServerConfig, std::string>()(i);
        }

        tide::IOManager iom(1);
        iom.schedule(std::bind(&Application::run_fiber, this));
        iom.stop();

        return 0;
    }

    int Application::run_fiber()
    {
        // 从全局配置变量 g_http_server_conf 中获取 HTTP 服务器的配置列表，并为每个配置项创建一个 HTTP 服务器实例，绑定监听地址并启动服务器。
        auto http_confs = g_http_server_conf->getValue();
        for (auto &i : http_confs)
        {
            std::vector<tide::Address::ptr> addrs;
            for (auto &a : i.address)
            {
                size_t pos = a.find(':');
                if (pos == std::string::npos)
                {
                    TIDE_LOG_ERROR(g_logger) << "invalid address: " << a;
                    continue;
                }

                tide::Address::ptr addr = tide::Address::LookupAny(a);
                if (addr)
                {
                    addrs.push_back(addr);
                    continue;
                }

                std::vector<std::pair<tide::Address::ptr, uint32_t>> result;
                if (!tide::Address::GetInterfaceAddresses(result, a.substr(0, pos)))
                {
                    TIDE_LOG_ERROR(g_logger) << "invalid interface: " << a.substr(0, pos);
                }

                for (auto &x : result)
                {
                    auto ipaddr = std::dynamic_pointer_cast<tide::IPAddress>(x.first);
                    if (ipaddr)
                    {
                        ipaddr->setPort(atoi(a.substr(pos + 1).c_str()));
                    }
                    addrs.push_back(ipaddr);
                }
            }

            tide::http::HttpServer::ptr server(new tide::http::HttpServer(i.keepalive));
            std::vector<tide::Address::ptr> fails;
            if (!server->bind(addrs, fails))
            {
                for (auto &x : fails)
                {
                    TIDE_LOG_ERROR(g_logger) << "bind address fail: " << *x;
                }
                exit(0);
            }

            server->start();
            m_httpservers.push_back(server);
        }

        return 0;
    }
}
#include "application.h"

#include <memory>
#include <functional>
#include <utility>

#include "worker.h"
#include "iomanager.h"
#include "address.h"
#include "env.h"
#include "log.h"
#include "config.h"
#include "daemon.h"
#include "module.h"
#include "http/http_server.h"
#include "tcp_server.h"
#include "http/ws_server.h"

namespace tide
{
    static tide::Logger::ptr g_logger = TIDE_LOG_NAME("system");

    // 定义一个全局配置变量，用于指定服务器的工作目录，默认值为相对目录，便于不同环境部署
    static tide::ConfigVar<std::string>::ptr g_server_work_path =
        tide::Config::Lookup<std::string>("server.work_path", std::string("./work"), "server work path");

    // 定义一个全局配置变量，用于指定服务器的PID文件路径，默认值为tide.pid，并提供描述信息
    static tide::ConfigVar<std::string>::ptr g_server_pid_file =
        tide::Config::Lookup<std::string>("server.pid_file", std::string("tide.pid"), "server pid file");

    // 定义一个全局配置变量，用于指定HTTP服务器的配置列表，默认值为空列表，并提供描述信息
    static tide::ConfigVar<std::vector<TcpServerConf>>::ptr g_servers_conf =
        tide::Config::Lookup("servers", std::vector<TcpServerConf>(), "http server config");

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
        bool is_printHelp = false;
        if (!tide::EnvMgr::GetInstance()->init(argc, argv))
        {
            is_printHelp = true;
        }

        // 如果用户请求帮助信息，则打印帮助信息并退出
        if (tide::EnvMgr::GetInstance()->has("p"))
        {
            is_printHelp = true;
        }

        std::string conf_path = tide::EnvMgr::GetInstance()->getConfigPath();
        TIDE_LOG_INFO(g_logger) << "load conf path:" << conf_path;
        tide::Config::LoadFromConfDir(conf_path);

        ModuleMgr::GetInstance()->init();
        std::vector<Module::ptr> modules;
        ModuleMgr::GetInstance()->listAll(modules);

        for (auto i : modules)
        {
            i->onBeforeArgsParse(argc, argv);
        }

        if (is_printHelp)
        {
            tide::EnvMgr::GetInstance()->printHelp();
            return false;
        }

        for (auto i : modules)
        {
            i->onAfterArgsParse(argc, argv);
        }
        modules.clear();

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
        TIDE_LOG_INFO(g_logger) << "main";

        // 加载配置目录，扫描并加载配置目录中的所有配置文件，默认目录为./conf
        std::string conf_path = tide::EnvMgr::GetInstance()->getConfigPath();
        tide::Config::LoadFromConfDir(conf_path, true);

        std::string pidfile = g_server_work_path->getValue() + "/" + g_server_pid_file->getValue();

        std::ofstream ofs(pidfile);
        if (!ofs)
        {
            TIDE_LOG_ERROR(g_logger) << "open pidfile " << pidfile << " failed";
            return -1;
        }

        ofs << getpid();

        m_mainIOManager.reset(new tide::IOManager(1, true, "main"));
        m_mainIOManager->schedule(std::bind(&Application::run_fiber, this));
        m_mainIOManager->addTimer(2000, []() {}, true);
        m_mainIOManager->stop();

        return 0;
    }

    int Application::run_fiber()
    {
        std::vector<Module::ptr> modules;
        ModuleMgr::GetInstance()->listAll(modules);
        bool has_error = false;
        for (auto &i : modules)
        {
            if (!i->onLoad())
            {
                TIDE_LOG_ERROR(g_logger) << "module name="
                                          << i->getName() << " version=" << i->getVersion()
                                          << " filename=" << i->getFilename();
                has_error = true;
            }
        }
        if (has_error)
        {
            _exit(0);
        }
        tide::WorkerMgr::GetInstance()->init();
        auto http_confs = g_servers_conf->getValue();
        for (auto &i : http_confs)
        {
            TIDE_LOG_DEBUG(g_logger) << std::endl
                                      << LexicalCast<TcpServerConf, std::string>()(i);

            std::vector<Address::ptr> address;
            for (auto &a : i.address)
            {
                size_t pos = a.find(":");
                if (pos == std::string::npos)
                {
                    address.push_back(UnixAddress::ptr(new UnixAddress(a)));
                    continue;
                }
                int32_t port = atoi(a.substr(pos + 1).c_str());
                // 127.0.0.1
                auto addr = tide::IPAddress::Create(a.substr(0, pos).c_str(), port);
                if (addr)
                {
                    address.push_back(addr);
                    continue;
                }
                std::vector<std::pair<Address::ptr, uint32_t>> result;
                if (tide::Address::GetInterfaceAddresses(result,
                                                          a.substr(0, pos)))
                {
                    for (auto &x : result)
                    {
                        auto ipaddr = std::dynamic_pointer_cast<IPAddress>(x.first);
                        if (ipaddr)
                        {
                            ipaddr->setPort(atoi(a.substr(pos + 1).c_str()));
                        }
                        address.push_back(ipaddr);
                    }
                    continue;
                }

                auto aaddr = tide::Address::LookupAny(a);
                if (aaddr)
                {
                    address.push_back(aaddr);
                    continue;
                }
                TIDE_LOG_ERROR(g_logger) << "invalid address: " << a;
                _exit(0);
            }
            IOManager *accept_worker = tide::IOManager::GetThis();
            IOManager *process_worker = tide::IOManager::GetThis();
            if (!i.accept_worker.empty())
            {
                accept_worker = tide::WorkerMgr::GetInstance()->getAsIOManager(i.accept_worker).get();
                if (!accept_worker)
                {
                    TIDE_LOG_ERROR(g_logger) << "accept_worker: " << i.accept_worker  << " not exists";
                    _exit(0);
                }
            }
            if (!i.process_worker.empty())
            {
                process_worker = tide::WorkerMgr::GetInstance()->getAsIOManager(i.process_worker).get();
                if (!process_worker)
                {
                    TIDE_LOG_ERROR(g_logger) << "process_worker: " << i.process_worker
                                              << " not exists";
                    _exit(0);
                }
            }

            TcpServer::ptr server;
            if (i.type == "http")
            {
                server.reset(new tide::http::HttpServer(i.keepalive,
                                                         process_worker, accept_worker));
            }
            else if (i.type == "ws")
            {
                server.reset(new tide::http::WSServer(process_worker, accept_worker));
            }
            else
            {
                TIDE_LOG_ERROR(g_logger) << "invalid server type=" << i.type
                                          << LexicalCast<TcpServerConf, std::string>()(i);
                _exit(0);
            }
            std::vector<Address::ptr> fails;
            if (!server->bind(address, fails, i.ssl))
            {
                for (auto &x : fails)
                {
                    TIDE_LOG_ERROR(g_logger) << "bind address fail:"
                                              << *x;
                }
                _exit(0);
            }
            if (i.ssl)
            {
                if (!server->loadCertificates(i.cert_file, i.key_file))
                {
                    TIDE_LOG_ERROR(g_logger) << "loadCertificates fail, cert_file="
                                              << i.cert_file << " key_file=" << i.key_file;
                }
            }
            if (!i.name.empty())
            {
                server->setName(i.name);
            }
            server->setConf(i);
            server->start();
            m_servers[i.type].push_back(server);
        }

        for (auto &i : modules)
        {
            i->onServerReady();
        }
        return 0;
    }

    bool Application::getServer(const std::string &type, std::vector<tide::TcpServer::ptr> &server)
    {
        auto it = m_servers.find(type);
        if (it == m_servers.end())
        {
            return false;
        }
        server = it->second;
        return true;
    }

}
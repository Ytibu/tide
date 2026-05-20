#include "env.h"

#include <string.h>
#include <unistd.h>

#include <iostream>
#include <iomanip>

#include "log.h"

namespace tide
{
    static tide::Logger::ptr g_logger = TIDE_LOG_NAME("system");

    bool Env::init(int argc, char **argv)
    {
        // 通过/proc/self/exe链接获取当前可执行文件的绝对路径，并将其存储在m_exe成员变量中。
        // 同时，提取出可执行文件所在目录的路径并存储在m_cwd成员变量中。最后，将程序的名称（即argv[0]）存储在m_program成员变量中。
        char link[1024] = {0};
        snprintf(link, sizeof(link), "/proc/%d/exe", getpid());
        char path[1024] = {0};
        ssize_t n = readlink(link, path, sizeof(path));
        if (n <= 0)
        {
            TIDE_LOG_ERROR(g_logger) << "Failed to readlink";
            return false;
        }
        m_exe = path;

        auto pos = m_exe.find_last_of('/');
        m_cwd = m_exe.substr(0, pos) + "/";

        m_program = argv[0];

        // -config /path/to/config -file xxx
        const char *now_key = nullptr;
        for (int i = 1; i < argc; ++i)
        {
            if (argv[i][0] == '-')
            {
                if (strlen(argv[i]) > 1)
                {
                    if (now_key)
                    {
                        add(now_key, "");
                    }
                    now_key = argv[i] + 1;
                }
                else
                {
                    TIDE_LOG_ERROR(g_logger) << "Invalid arg idx: " << i << ", value: " << argv[i];
                    return false;
                }
            }
            else
            {
                if (now_key)
                {
                    add(now_key, argv[i]);
                    now_key = nullptr;
                }
                else
                {
                    TIDE_LOG_ERROR(g_logger) << "Invalid arg idx: " << i << ", value: " << argv[i];
                    return false;
                }
            }
        }

        if (now_key)
        {
            add(now_key, "");
        }

        return true;
    }

    void Env::add(const std::string &key, const std::string &value)
    {
        RWMutexType::WriteLock lock(m_mutex);
        m_args[key] = value;
    }

    void Env::del(const std::string &key)
    {
        RWMutexType::WriteLock lock(m_mutex);
        m_args.erase(key);
    }

    bool Env::has(const std::string &key)
    {
        RWMutexType::ReadLock lock(m_mutex);
        auto it = m_args.find(key);
        return it != m_args.end();
    }

    std::string Env::get(const std::string &key, const std::string &default_value)
    {
        RWMutexType::ReadLock lock(m_mutex);
        auto it = m_args.find(key);
        return it != m_args.end() ? it->second : default_value;
    }

    void Env::addHelp(const std::string &key, const std::string &desc)
    {
        removeHelp(key);

        RWMutexType::WriteLock lock(m_mutex);
        m_helps.push_back(std::make_pair(key, desc));
    }

    void Env::removeHelp(const std::string &key)
    {
        RWMutexType::WriteLock lock(m_mutex);
        for (auto it = m_helps.begin(); it != m_helps.end();)
        {
            if (it->first == key)
            {
                it = m_helps.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    void Env::printHelp()
    {
        RWMutexType::ReadLock lock(m_mutex);

        std::cout << "Usage: " << m_program << " [options] " << std::endl;
        for (auto &i : m_helps)
        {
            std::cout << std::setw(5) << "-" << i.first << " : " << i.second << std::endl;
        }
    }

    // 设置环境变量的值，如果环境变量不存在则创建新的环境变量
    bool Env::setEnv(const std::string &key, const std::string &value)
    {
        return !setenv(key.c_str(), value.c_str(), 1);
    }

    // 获取环境变量的值，如果不存在则返回默认值
    std::string Env::getEnv(const std::string &key, const std::string &default_value)
    {
        const char *v = getenv(key.c_str());
        if (v == nullptr)
        {
            return default_value;
        }
        return v;
    }

    std::string Env::getAbsolutePath(const std::string &path) const
    {
        if(path.empty())
        {
            return "/";
        }
        
        if (path[0] == '/')
        {
            return path;
        }
        return m_cwd + path;
    }

}
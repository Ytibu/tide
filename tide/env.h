#ifndef TIDE_ENV_H__
#define TIDE_ENV_H__

#include <memory>
#include <utility>
#include <map>
#include <vector>
#include <string>

#include "singleton.h"
#include "thread.h"

namespace tide
{
    class Env
    {
    public:
        using RWMutexType = RWMutex;

        /**
         * @brief 初始化环境变量，解析命令行参数并加载到环境变量中
         * 
         * @param argc 
         * @param argv 
         * @return true 
         * @return false 
         */
        bool init(int argc, char **argv);
        
        /**
         * @brief 添加环境变量
         * 
         * @param key 
         * @param value 
         */
        void add(const std::string &key, const std::string &value);

        /**
         * @brief 删除环境变量
         * 
         * @param key 
         * @return true 
         * @return false 
         */
        void del(const std::string &key);
        
        /**
         * @brief 检查环境变量是否存在
         * 
         * @param key 
         * @return true 
         * @return false 
         */
        bool has(const std::string &key);
        
        /**
         * @brief 获取环境变量的值
         * 
         * @param key 
         * @param default_value 
         * @return std::string& 
         */
        std::string get(const std::string &key, const std::string &default_value = "");

        /**
         * @brief 添加帮助信息
         * 
         * @param key 
         * @param desc 
         */
        void addHelp(const std::string &key, const std::string &desc);
        
        /**
         * @brief 删除帮助信息
         * 
         * @param key 
         */
        void removeHelp(const std::string &key);
        
        /**
         * @brief 打印帮助信息，列出所有的环境变量和对应的描述信息
         * 
         */
        void printHelp();

        /**
         * @brief 获取当前程序的绝对路径
         * 
         * @return std::string 
         */
        std::string getExe() const { return m_exe; }

        /**
         * @brief 获取当前程序的工作目录
         * 
         * @return std::string 
         */
        std::string getCwd() const { return m_cwd; }

        /**
         * @brief 设置环境变量的值，如果环境变量不存在则创建新的环境变量
         * 
         * @param key 
         * @param value 
         * @return true 
         * @return false 
         */
        bool setEnv(const std::string &key, const std::string &value);

        /**
         * @brief 获取环境变量的值，如果环境变量不存在则返回默认值
         * 
         * @param key 
         * @param default_value 
         * @return std::string 
         */
        std::string getEnv(const std::string &key, const std::string &default_value = "");

        /**
         * @brief 解析环境变量中的路径并返回绝对路径
         * 
         * @param path 
         * @return std::string 
         */
        std::string getAbsolutePath(const std::string &path) const;

    private:
        RWMutexType m_mutex;
        std::map<std::string, std::string> m_args;
        std::vector<std::pair<std::string, std::string>> m_helps;

        std::string m_program;
        std::string m_exe;
        std::string m_cwd;
    };

    using EnvMgr = tide::Singleton<Env>;
}

#endif // TIDE_ENV_H__
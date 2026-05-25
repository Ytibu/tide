#ifndef TIDE_TCP_SERVER_H__
#define TIDE_TCP_SERVER_H__

#include <memory>
#include <boost/lexical_cast.hpp>
#include <yaml-cpp/yaml.h>
#include <string>
#include <vector>
#include <map>
#include <sstream>

#include "iomanager.h"
#include "noncopyable.h"
#include "config.h"
#include "socket.h"

namespace tide
{
    struct TcpServerConf
    {
        typedef std::shared_ptr<TcpServerConf> ptr;

        std::vector<std::string> address;
        int keepalive = 0;
        int timeout = 1000 * 2 * 60;
        int ssl = 0;
        std::string id;
        /// 服务器类型，http, ws
        std::string type = "http";
        std::string name;
        std::string cert_file;
        std::string key_file;
        std::string accept_worker;
        std::string process_worker;
        std::map<std::string, std::string> args;

        bool isValid() const
        {
            return !address.empty();
        }

        bool operator==(const TcpServerConf &oth) const
        {
            return address == oth.address && keepalive == oth.keepalive && timeout == oth.timeout && name == oth.name && ssl == oth.ssl && cert_file == oth.cert_file && key_file == oth.key_file && accept_worker == oth.accept_worker && process_worker == oth.process_worker && args == oth.args && id == oth.id && type == oth.type;
        }
    };

    template <>
    class LexicalCast<std::string, TcpServerConf>
    {
    public:
        TcpServerConf operator()(const std::string &v)
        {
            YAML::Node node = YAML::Load(v);
            TcpServerConf conf;
            conf.id = node["id"].as<std::string>(conf.id);
            conf.type = node["type"].as<std::string>(conf.type);
            conf.keepalive = node["keepalive"].as<int>(conf.keepalive);
            conf.timeout = node["timeout"].as<int>(conf.timeout);
            conf.name = node["name"].as<std::string>(conf.name);
            conf.ssl = node["ssl"].as<int>(conf.ssl);
            conf.cert_file = node["cert_file"].as<std::string>(conf.cert_file);
            conf.key_file = node["key_file"].as<std::string>(conf.key_file);
            conf.accept_worker = node["accept_worker"].as<std::string>();
            conf.process_worker = node["process_worker"].as<std::string>();
            conf.args = LexicalCast<std::string, std::map<std::string, std::string>>()(node["args"].as<std::string>(""));
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

    template <>
    class LexicalCast<TcpServerConf, std::string>
    {
    public:
        std::string operator()(const TcpServerConf &conf)
        {
            YAML::Node node;
            node["id"] = conf.id;
            node["type"] = conf.type;
            node["name"] = conf.name;
            node["keepalive"] = conf.keepalive;
            node["timeout"] = conf.timeout;
            node["ssl"] = conf.ssl;
            node["cert_file"] = conf.cert_file;
            node["key_file"] = conf.key_file;
            node["accept_worker"] = conf.accept_worker;
            node["process_worker"] = conf.process_worker;
            node["args"] = YAML::Load(LexicalCast<std::map<std::string, std::string>, std::string>()(conf.args));
            for (auto &i : conf.address)
            {
                node["address"].push_back(i);
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    class TcpServer : public std::enable_shared_from_this<TcpServer>, noncopyable
    {
    public:
        using ptr = std::shared_ptr<TcpServer>;

        /**
         * @brief 构造函数，
         * 接受两个IOManager指针参数，分别用于处理客户端连接和接受新连接的IO事件。
         *
         * @param worker socket客户端工作的协程调度器
         * @param accept_worker 服务器socket接受socket连接的协程调度器
         */
        TcpServer(tide::IOManager *worker = tide::IOManager::GetThis(), tide::IOManager *accept_worker = tide::IOManager::GetThis());

        /**
         * @brief 虚析构函数，确保在派生类中正确释放资源。
         *
         */
        virtual ~TcpServer();

        /**
         * @brief 绑定地址
         *
         * @param addr
         * @return true
         * @return false
         */
        virtual bool bind(tide::Address::ptr addr, bool ssl = false);

        /**
         * @brief 绑定多个地址
         *
         * @param addrs vector类型的地址列表
         * @param fails 用于存储绑定失败的地址列表
         * @return true
         * @return false
         */
        virtual bool bind(const std::vector<tide::Address::ptr> &addrs, std::vector<tide::Address::ptr> &fails, bool ssl = false);

        /**
         * @brief 加载SSL证书和私钥文件，启用SSL/TLS支持。
         *
         * @param cert_file
         * @param key_file
         * @return true
         * @return false
         */
        bool loadCertificates(const std::string &cert_file, const std::string &key_file);

        /**
         * @brief 启动服务器，开始接受客户端连接并处理请求。
         *
         * @return true
         * @return false
         */
        virtual bool start();

        /**
         * @brief 停止服务器，关闭所有连接并释放资源。
         *
         * @return true
         * @return false
         */
        virtual bool stop();

        /**
         * @brief 获取服务器的读超时时间，单位为毫秒。
         *
         * @return uint64_t
         */
        uint64_t getReadTimeout() const { return m_readTimeout; }

        /**
         * @brief 设置服务器的读超时时间，单位为毫秒。
         *
         * @param v
         */
        void setReadTimeout(uint64_t v) { m_readTimeout = v; }

        /**
         * @brief 获取服务器名称
         *
         * @return std::string
         */
        std::string getName() const { return m_name; }

        /**
         * @brief 设置服务器名称
         *
         * @param v
         */
        virtual void setName(const std::string &v) { m_name = v; }

        TcpServerConf::ptr getConf() const { return m_conf; }
        void setConf(TcpServerConf::ptr v) { m_conf = v; }
        void setConf(const TcpServerConf &v);

    protected:
        /**
         * @brief 处理客户端连接的函数，
         * 接受一个Socket指针参数，表示与客户端的连接套接字。
         *
         * @param client Socket指针，表示与客户端的连接套接字
         */
        virtual void handleClient(Socket::ptr client);

        /**
         * @brief 启动接受连接的协程，
         * 负责监听服务器socket上的连接请求，并在接受到新连接时调用handleClient函数处理客户端连接。
         *
         * @param sock Socket指针，表示服务器的监听套接字
         */
        virtual void startAccept(Socket::ptr sock);

    protected:
        std::vector<Socket::ptr> m_socks;
        IOManager *m_worker;
        IOManager *m_acceptWorker;
        uint64_t m_readTimeout;
        std::string m_name;
        std::string m_type = "tcp";
        bool m_isStop;
        bool m_ssl = false;

        TcpServerConf::ptr m_conf;
    };

} // namespace tide

#endif // TIDE_TCP_SERVER_H__
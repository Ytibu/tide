#ifndef TIDE_TCP_SERVER_H__
#define TIDE_TCP_SERVER_H__

#include <memory>

#include "iomanager.h"
#include "noncopyable.h"

namespace tide
{
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
        TcpServer(tide::IOManager* worker = tide::IOManager::GetThis(), tide::IOManager* accept_worker = tide::IOManager::GetThis());
        
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
        virtual bool bind(const std::vector<tide::Address::ptr>& addrs, std::vector<tide::Address::ptr>& fails, bool ssl = false);
        
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
        void setName(const std::string& v) { m_name = v; }
    
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

    private:
        std::vector<Socket::ptr> m_socks;
        IOManager* m_worker;
        IOManager* m_acceptWorker;
        uint64_t m_readTimeout;
        std::string m_name;
        bool m_isStop;
    };
    
    
} // namespace tide


#endif // TIDE_TCP_SERVER_H__
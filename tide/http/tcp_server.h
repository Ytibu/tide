#ifndef TIDE_TCP_SERVER_H__
#define TIDE_TCP_SERVER_H__

#include <memory>

#include "../iomanager.h"
#include "../noncopyable.h"

namespace tide
{
    class TcpServer : public std::enable_shared_from_this<TcpServer>, noncopyable
    {
    public:
        using ptr = std::shared_ptr<TcpServer>;

        TcpServer(tide::IOManager* worker = tide::IOManager::GetThis(), tide::IOManager* accept_worker = tide::IOManager::GetThis());
        virtual ~TcpServer();

        virtual bool bind(tide::Address::ptr addr);
        virtual bool bind(const std::vector<tide::Address::ptr>& addrs, std::vector<tide::Address::ptr>& fails);
        virtual bool start();
        virtual bool stop();

        uint64_t getReadTimeout() const { return m_readTimeout; }
        void setReadTimeout(uint64_t v) { m_readTimeout = v; }
        std::string getName() const { return m_name; }
        void setName(const std::string& v) { m_name = v; }
    
    protected:
        virtual void handleClient(Socket::ptr client);
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
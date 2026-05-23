#include "tcp_server.h"

#include "iomanager.h"

namespace tide
{

    static tide::Logger::ptr g_logger = TIDE_LOG_NAME("system");
    static tide::ConfigVar<uint64_t>::ptr g_tcp_server_read_timeout = tide::Config::Lookup("tcp_server.read_timeout", (uint64_t)60 * 1000 * 2, "tcp server read timeout");

    TcpServer::TcpServer(tide::IOManager *worker, tide::IOManager *accept_worker)
        : m_worker(worker), m_acceptWorker(accept_worker), m_readTimeout(g_tcp_server_read_timeout->getValue()), m_name("tide/1.0.0"), m_isStop(true)
    {
    }

    TcpServer::~TcpServer()
    {
    }

    void TcpServer::setConf(const TcpServerConf &v)
    {
        m_conf.reset(new TcpServerConf(v));
    }

    bool TcpServer::bind(tide::Address::ptr addr, bool ssl)
    {
        std::vector<tide::Address::ptr> addrs;
        addrs.push_back(addr);
        std::vector<tide::Address::ptr> fails;
        return bind(addrs, fails, ssl);
    }

    bool TcpServer::bind(const std::vector<tide::Address::ptr> &addrs, std::vector<tide::Address::ptr> &fails, bool ssl)
    {
        m_ssl = ssl;
        for (auto &addr : addrs)
        {
            Socket::ptr sock = ssl ? SSLSocket::CreateTCP(addr) : Socket::CreateTCP(addr);
            if (!sock->bind(addr))
            {
                TIDE_LOG_ERROR(g_logger) << "bind tcp server socket failed, addr=" << addr->toString();
                fails.push_back(addr);
                continue;
            }
            if (!sock->listen())
            {
                TIDE_LOG_ERROR(g_logger) << "listen tcp server socket failed, addr=" << addr->toString();
                fails.push_back(addr);
                continue;
            }
            m_socks.push_back(sock);
        }

        if (!fails.empty())
        {
            m_socks.clear();
            return false;
        }

        for (auto &sock : m_socks)
        {
            TIDE_LOG_INFO(g_logger) << "type= " << m_type << " name= " << m_name << " ssl=" << m_ssl << " bind success: " << *sock;
        }

        return true;
    }

    bool TcpServer::loadCertificates(const std::string &cert_file, const std::string &key_file)
    {
        for (auto &i : m_socks)
        {
            auto ssl_socket = std::dynamic_pointer_cast<SSLSocket>(i);
            if (ssl_socket)
            {
                if (!ssl_socket->loadCertificates(cert_file, key_file))
                {
                    return false;
                }
            }
        }
        return true;
    }

    bool TcpServer::start()
    {
        if (!m_isStop)
        {
            return true;
        }
        m_isStop = false;
        for (auto &sock : m_socks)
        {
            m_acceptWorker->schedule(std::bind(&TcpServer::startAccept, shared_from_this(), sock));
        }
        return true;
    }

    bool TcpServer::stop()
    {
        m_isStop = true;
        auto self = shared_from_this();
        m_acceptWorker->schedule([this, self]()
                                 {
            for(auto &sock : m_socks){
                sock->cancelAll();
                sock->close();
            }
            m_socks.clear(); });

        return true;
    }

    void TcpServer::handleClient(Socket::ptr client)
    {
        TIDE_LOG_INFO(g_logger) << "handleClient: " << *client;
    }

    void TcpServer::startAccept(Socket::ptr sock)
    {
        while (!m_isStop)
        {
            Socket::ptr client = sock->accept();
            if (client)
            {
                client->setRecvTimeout(m_readTimeout);
                m_worker->schedule(std::bind(&TcpServer::handleClient, shared_from_this(), client));
            }
            else
            {
                TIDE_LOG_ERROR(g_logger) << "accept failed, errno=" << errno << " errstr=" << strerror(errno);
            }
        }
    }
}
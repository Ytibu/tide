#include "../tide/http/tcp_server.h"
#include "../tide/iomanager.h"
#include "../tide/log.h"
#include "../tide/bytearray.h"

static tide::Logger::ptr g_logger = TIDE_LOG_ROOT();

class EchoServer : public tide::TcpServer
{
public:
    EchoServer(int type);
    void handleClient(tide::Socket::ptr client);

private:
    int m_type = 0;
};

EchoServer::EchoServer(int type)
    : m_type(type)
{
}

void EchoServer::handleClient(tide::Socket::ptr client)
{
    TIDE_LOG_INFO(g_logger) << "handleClient: " << *client;
    tide::ByteArray::ptr ba(new tide::ByteArray);
    while(true)
    {
        ba->clear();
        std::vector<iovec> buf;
        ba->getWriteBuffers(buf, 1024);
        int len = client->recv(&buf[0], buf.size());
        if(len <= 0)
        {
            TIDE_LOG_INFO(g_logger) << "client close: " << *client;
            break;
        }
        ba->setPosition(ba->getPosition() + len);
        ba->setPosition(0);
        if(m_type == 1)
        {
            TIDE_LOG_INFO(g_logger) << "recv: " << ba->toString();
        }
        else
        {
            TIDE_LOG_INFO(g_logger) << "recv: " << ba->toHexString();
        }
    }
}

int type = 1;

int main()
{
    tide::IOManager iom(2);
    iom.schedule([&](){
        EchoServer::ptr es(new EchoServer(type));
        auto addr =  tide::Address::LookupAny("0.0.0.0:8115");
        if(!addr)
        {
            TIDE_LOG_ERROR(g_logger) << "invalid bind address";
            return;
        }

        if(!es->bind(addr))
        {
            TIDE_LOG_ERROR(g_logger) << "bind echo server failed, addr=" << addr->toString();
            return;
        }

        if(!es->start())
        {
            TIDE_LOG_ERROR(g_logger) << "start echo server failed, addr=" << addr->toString();
        }
    });

    return 0;
}
#include "../tide/address.h"
#include "../tide/socket.h"
#include "../tide/log.h"
#include "../tide/iomanager.h"

#include <memory>

static tide::Logger::ptr g_logger = TIDE_LOG_ROOT();

void test_socket()
{
    tide::IPAddress::ptr addr = tide::Address::LookupAnyIPAddress("www.baidu.com");
    if (!addr)
    {
        TIDE_LOG_ERROR(g_logger) << "get address failed";
        return;
    }
    else
    {
        TIDE_LOG_INFO(g_logger) << "get address success: " << addr->toString();
    }

    tide::Socket::ptr sock = tide::Socket::CreateTCP(addr);
    addr->setPort(80);
    TIDE_LOG_INFO(g_logger) << "address=" << addr->toString();
    if (sock->connect(addr))
    {
        TIDE_LOG_INFO(g_logger) << "connect " << addr->toString() << " success";
    }
    else
    {
        TIDE_LOG_ERROR(g_logger) << "connect " << addr->toString() << " failed";
        return;
    }

    const char buff[] = "GET / HTTP/1.0\r\n\r\n";
    int rt = sock->send(buff, sizeof(buff));
    if(rt < 0)
    {
        TIDE_LOG_ERROR(g_logger) << "send failed, errno=" << errno << " errstr=" << strerror(errno);
        return;
    }
    else
    {
        TIDE_LOG_INFO(g_logger) << "send success rt=" << rt;
    }

    std::string buffs;
    buffs.resize(4096);
    rt = sock->recv(&buffs[0], buffs.size());
    if(rt <= 0)
    {
        TIDE_LOG_INFO(g_logger) << "recv faile rt = " << rt;
        return;
    }

    buffs.resize(rt);
    TIDE_LOG_INFO(g_logger) << "recv success rt=" << rt << " data=\n" << buffs;

}

int main()
{
    tide::IOManager iom;
    iom.schedule(&test_socket);
    return 0;
}
#include "../tide/http/http_connection.h"
#include "../tide/http/http_parser.h"
#include <iostream>
#include "../tide/log.h"
#include "../tide/iomanager.h"
#include "../tide/address.h"

static tide::Logger::ptr g_logger = TIDE_LOG_ROOT();


void run()
{
    tide::Address::ptr addr = tide::IPv4Address::LookupAny("127.0.0.1:8080");
    if (!addr)
    {
        TIDE_LOG_ERROR(g_logger) << "get address error";
        return;
    }else{
        TIDE_LOG_INFO(g_logger) << "address=" << addr->toString();
    }
    tide::Socket::ptr sock = tide::Socket::CreateTCP(addr);
    if (!sock)    
    {
        TIDE_LOG_ERROR(g_logger) << "create socket error";
        return;
    }
    bool rt = sock->connect(addr);
    if (!rt)
    {
        TIDE_LOG_ERROR(g_logger) << "connect error";
        return;
    }

    tide::http::HttpConnection::ptr conn(new tide::http::HttpConnection(sock));
    tide::http::HttpRequest::ptr req(new tide::http::HttpRequest);
    TIDE_LOG_INFO(g_logger) << "req=\n" << *req;

    conn->sendRequest(req);
    auto rsp = conn->recvResponse();

    if (!rsp) {
        TIDE_LOG_ERROR(g_logger) << "recv response error";
        return;
    }
    TIDE_LOG_INFO(g_logger) << "rsp=\n" << *rsp;

}

int main()
{
    tide::IOManager iom(2);
    iom.schedule(run);
    return 0;
}
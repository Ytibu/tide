#include "../tide/http/http_connection.h"
#include "../tide/http/http_parser.h"
#include <iostream>
#include "../tide/log.h"
#include "../tide/iomanager.h"
#include "../tide/address.h"

static tide::Logger::ptr g_logger = TIDE_LOG_ROOT();

void test_pool()
{
    tide::http::HttpConnectionPool::ptr pool(new tide::http::HttpConnectionPool("8.146.201.152", "", 80, 10, 1000 * 30, 20));
    if(!pool)
    {
        TIDE_LOG_ERROR(g_logger) << "create http connection pool failed";
        return;
    }

    tide::IOManager::GetThis()->addTimer(1000, [pool](){
        auto r = pool->doGET("/", 3000);
        TIDE_LOG_INFO(g_logger) << "result=" << r->result << " error=" << r->error << " rsp=\n" << (r->response ? r->response->toString() : "");
    }, true);



}

void run()
{
    tide::Address::ptr addr = tide::IPv4Address::LookupAny("www.baidu.com:80");
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

    // 写入文件
    std::ofstream ofs("./log/response.txt");
    ofs << *rsp;
    ofs.close();


    std::cout << "-----------------------------------------" << std::endl;

    // auto r = tide::http::HttpConnection::DoGET("http://chat.baidu.com/search/9006854824565958738?enter_type=pic_picfunc_3", 5000);
    auto r = tide::http::HttpConnection::DoGET("http://8.146.201.152/assistant.html", 5000);
    TIDE_LOG_INFO(g_logger) << "DoGET result=" << r->result << " error=" << r->error << " rsp=\n" << (r->response ? r->response->toString() : "");

    test_pool();
}

int main()
{
    tide::IOManager iom(2);
    iom.schedule(run);
    return 0;
}
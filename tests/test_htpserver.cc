#include "../tide/http/http_server.h"
#include "../tide/log.h"

static tide::Logger::ptr g_logger = TIDE_LOG_ROOT();


void run()
{
    tide::http::HttpServer::ptr server(new tide::http::HttpServer);
    auto addr = tide::Address::LookupAny("127.0.0.1:8080");
    while(!server->bind(addr))
    {
        TIDE_LOG_ERROR(g_logger) << "bind failed";
        sleep(2);
    }
    server->start();
}

int main()
{
    tide::IOManager iom(2);
    iom.schedule(run);
    return 0;
}
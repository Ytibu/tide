#include "../tide/http/tcp_server.h"
#include "../tide/iomanager.h"
#include "../tide/log.h"

static tide::Logger::ptr g_logger = TIDE_LOG_ROOT();
void run()
{
    auto addr =  tide::Address::LookupAny("0.0.0.0:8115");
    
    std::vector<tide::Address::ptr> addrs;
    addrs.push_back(addr);

    tide::TcpServer::ptr tcp_server(new tide::TcpServer);
    std::vector<tide::Address::ptr> fails;
    while(!tcp_server->bind(addrs, fails)){
        sleep(2);
    }
    tcp_server->start();
}

int main()
{
    tide::IOManager iom(2);
    iom.schedule(run);

    return 0;
}
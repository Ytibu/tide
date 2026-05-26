#include "../tide/http/ws_server.h"
#include "../tide/log.h"

static tide::Logger::ptr g_logger = TIDE_LOG_ROOT();

void run() {
    tide::http::WSServer::ptr server(new tide::http::WSServer);
    tide::Address::ptr addr = tide::Address::LookupAnyIPAddress("0.0.0.0:8020");
    if(!addr) {
        TIDE_LOG_ERROR(g_logger) << "get address error";
        return;
    }
    auto fun = [](tide::http::HttpRequest::ptr header
                  ,tide::http::WSFrameMessage::ptr msg
                  ,tide::http::WSSession::ptr session) {
        session->sendMessage(msg);
        return 0;
    };


    server->getWSServletDispatch()->addServlet("/tide", fun);
    while(!server->bind(addr)) {
        TIDE_LOG_ERROR(g_logger) << "bind " << *addr << " fail";
        sleep(1);
    }
    server->start();
}

int main(int argc, char** argv) {
    tide::IOManager iom(2);
    iom.schedule(run);
    return 0;
}

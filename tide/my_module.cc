#include "my_module.h"
#include "config.h"
#include "log.h"
#include "application.h"
// #include "tide/http/ws_server.h"

// #include "resource_servlet.h"
// #include "chat_servlet.h"
#include "env.h"

namespace chat {

static tide::Logger::ptr g_logger = TIDE_LOG_ROOT();

MyModule::MyModule()
    :tide::Module("chat_room", "1.0", "") {
}

bool MyModule::onLoad() {
    TIDE_LOG_INFO(g_logger) << "onLoad";
    return true;
}

bool MyModule::onUnload() {
    TIDE_LOG_INFO(g_logger) << "onUnload";
    return true;
}

bool MyModule::onServerReady() {
    TIDE_LOG_INFO(g_logger) << "onServerReady";
    
    return true;
}

static int32_t handle(tide::http::HttpRequest::ptr req, tide::http::HttpResponse::ptr rsp, tide::http::HttpSession::ptr session) {
    TIDE_LOG_INFO(g_logger) << "handle";
    rsp->setBody("hello status");
    return 0;
}


bool MyModule::onServerUp() {
    TIDE_LOG_INFO(g_logger) << "onServerUp";

    std::vector<tide::TcpServer::ptr> servers;
    if(!tide::Application::GetInstance()->getServer("http", servers)) {
        TIDE_LOG_INFO(g_logger) << "not http server alive";
        return false;
    }

    for(auto& i : servers) {
        tide::http::HttpServer::ptr http_server = std::dynamic_pointer_cast<tide::http::HttpServer>(i);
        if(!http_server) {
            continue;
        }

        auto slt_dispatch = http_server->getServletDispatch();
        slt_dispatch->addServlet("/html/*", handle);

        TIDE_LOG_INFO(g_logger) << "http server name=" << i->getName() << " is up";
    }
    return true;
}

}

extern "C" {

tide::Module* CreateModule() {
    tide::Module* module = new chat::MyModule;
    TIDE_LOG_INFO(chat::g_logger) << "CreateModule " << module;
    return module;
}

void DestoryModule(tide::Module* module) {
    TIDE_LOG_INFO(chat::g_logger) << "DestoryModule " << module;
    delete module;
}

}
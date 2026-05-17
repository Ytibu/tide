#include "../tide/http/http_server.h"
#include "../tide/log.h"
#include "../tide/address.h"

static tide::Logger::ptr g_logger = TIDE_LOG_ROOT();


void run()
{
    // 创建 HttpServer 对象，绑定地址，添加 Servlet，启动服务器
    tide::http::HttpServer::ptr server(new tide::http::HttpServer);
    auto addr = tide::Address::LookupAny("0.0.0.0:8080");
    while(!server->bind(addr))
    {
        TIDE_LOG_ERROR(g_logger) << "bind failed";
        sleep(2);
    }

    // 获取 ServletDispatch 对象，添加 Servlet，处理 HTTP 请求
    auto sd = server->getServletDispatch();
    sd->addServlet("/tide/xx", [](tide::http::HttpRequest::ptr req, tide::http::HttpResponse::ptr rsp, tide::http::HttpSession::ptr session){
        rsp->setBody(req->toString());
        return 0;
    });
    sd->addGlobServlet("/tide/*", [](tide::http::HttpRequest::ptr req, tide::http::HttpResponse::ptr rsp, tide::http::HttpSession::ptr session){
        rsp->setBody("GLOB\r\n" + req->toString());
        return 0;
    });

    server->start();
}

int main()
{
    tide::IOManager iom(2);
    iom.schedule(run);
    return 0;
}
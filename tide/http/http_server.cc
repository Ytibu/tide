#include "http_server.h"

#include "http_session.h"
#include "http_parser.h"
#include "../log.h"
#include "servlet.h"

static tide::Logger::ptr g_logger = TIDE_LOG_NAME("system");

namespace tide
{
    namespace http
    {
        // 构造函数，初始化 HttpServer 对象，默认不使用 keepalive，使用当前线程的 IOManager 作为工作线程和接收线程
        HttpServer::HttpServer(bool keepalive, tide::IOManager* worker, tide::IOManager* accept_worker)
            : TcpServer(worker, accept_worker)
            , m_isKeepalive(keepalive)
        {
            m_dispatch.reset(new ServletDispatch);
        }

        // 处理客户端请求，步骤：创建 HttpSession 对象，接收 HTTP 请求，处理 HTTP 请求，发送 HTTP 响应，关闭 HttpSession 对象
        void HttpServer::handleClient(Socket::ptr client)
        {
            HttpSession::ptr session(new HttpSession(client));
            do
            {
                // 接收HTTP请求
                auto req = session->recvRequest();
                if (!req)
                {
                    TIDE_LOG_WARN(g_logger) << "recv http request failed, errno=" << errno 
                            << " errstr=" << strerror(errno) << " client:" << *client;
                    break;
                }

                // 处理HTTP请求，创建 HttpResponse 对象，调用 ServletDispatch 来处理 HTTP 请求，得到 HTTP 响应
                http::HttpResponse::ptr rsp(new http::HttpResponse(req->getVersion(), req->isClose() || !m_isKeepalive));

                rsp->setHeader("Server", getName());
                m_dispatch->handle(req, rsp, session);

                // rsp->setBody("hello world");

                session->sendResponse(rsp);
            } while (m_isKeepalive);

            session->close();
        }
    }
}
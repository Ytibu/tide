#include "http_server.h"

#include "http_session.h"
#include "http_parser.h"
#include "../log.h"

static tide::Logger::ptr g_logger = TIDE_LOG_NAME("system");

namespace tide
{
    namespace http
    {
        HttpServer::HttpServer(bool keepalive, tide::IOManager* worker, tide::IOManager* accept_worker)
            : TcpServer(worker, accept_worker)
            , m_isKeepalive(keepalive)
        {
        }

        void HttpServer::handleClient(Socket::ptr client)
        {
            TIDE_LOG_DEBUG(g_logger) << "handleClient " << *client;
            HttpSession::ptr session(new HttpSession(client));
            do
            {
                auto req = session->recvRequest();
                if (!req)
                {
                    TIDE_LOG_WARN(g_logger) << "recv http request failed, errno=" << errno 
                            << " errstr=" << strerror(errno) << " client:" << *client;
                    break;
                }
                http::HttpResponse::ptr rsp(new http::HttpResponse(req->getVersion(), req->isClose() || !m_isKeepalive));

                rsp->setBody("fuck you nvidia");
                TIDE_LOG_INFO(g_logger) << "request: " << *req;
                TIDE_LOG_INFO(g_logger) << "Response: " << *rsp;

                session->sendResponse(rsp);
            } while (m_isKeepalive);

            session->close();
        }
    }
}
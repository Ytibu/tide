#include "ws_server.h"

#include "ws_session.h"
#include "ws_servlet.h"

namespace tide
{
    namespace http
    {
        static tide::Logger::ptr g_logger = TIDE_LOG_NAME("system");
        WSServer::WSServer(tide::IOManager *worker, tide::IOManager *accept_worker)
            : TcpServer(worker, accept_worker)
        {
            m_dispatch.reset(new WSServletDispatch);
            m_type = "websocket_server";
        }

        void WSServer::handleClient(Socket::ptr client)
        {
            TIDE_LOG_DEBUG(g_logger) << "handleClient " << *client;
            WSSession::ptr session(new WSSession(client));

            do{
                HttpRequest::ptr header = session->handleShake();
                if(!header)
                {
                    TIDE_LOG_ERROR(g_logger) << "hand shake failed";
                    break;
                }

                WSServlet::ptr servlet = m_dispatch->getWSServlet(header->getPath());
                if(!servlet)
                {
                    TIDE_LOG_ERROR(g_logger) << "no servlet for path: " << header->getPath();
                    break;
                }

                int rt = servlet->onConnect(header, session);
                if(rt)
                {
                    TIDE_LOG_ERROR(g_logger) << "onConnect failed, rt=" << rt;
                    break;
                }

                while(true)
                {
                    auto msg = session->recvMessage();
                    if(!msg)
                    {
                        TIDE_LOG_ERROR(g_logger) << "recvMessage failed";
                        break;
                    }

                    rt = servlet->handle(header, msg, session);
                    if(rt)
                    {
                        TIDE_LOG_ERROR(g_logger) << "onMessage failed, rt=" << rt;
                        break;
                    }
                }

                servlet->onClose(header, session);
            }while(0);
            session->close();
        }

    }
}
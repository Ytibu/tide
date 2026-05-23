#ifndef TIDE_HTTP_WS_SERVER_H__
#define TIDE_HTTP_WS_SERVER_H__

#include "../tcp_server.h"
#include "ws_servlet.h"
#include "../iomanager.h"

namespace tide
{
    namespace http
    {
        class WSServer : public TcpServer
        {
        public:
            using ptr = std::shared_ptr<WSServer>;

            WSServer(tide::IOManager* worker = tide::IOManager::GetThis(), tide::IOManager* accept_worker = tide::IOManager::GetThis());

            WSServletDispatch::ptr getWSServletDispatch() const { return m_dispatch; }
            void setWSServletDispatcher(WSServletDispatch::ptr dispatch) { m_dispatch = dispatch; }

        protected:
            virtual void handleClient(Socket::ptr client) override;
        protected:
            WSServletDispatch::ptr m_dispatch;
        };
    }
}

#endif // TIDE_HTTP_WS_SERVER_H__
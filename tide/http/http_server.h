#ifndef TIDE_HTTP_SERVER_H__
#define TIDE_HTTP_SERVER_H__

#include <memory>

#include "../iomanager.h"
#include "tcp_server.h"

namespace tide
{
    namespace http
    {
        class HttpServer : public TcpServer
        {
        public:
            using ptr = std::shared_ptr<HttpServer>;
            HttpServer(bool keepalive = false, 
                tide::IOManager* worker = tide::IOManager::GetThis(),
                tide::IOManager* accept_worker = tide::IOManager::GetThis());
        protected:
            virtual void handleClient(Socket::ptr client) override;
        private:
            bool m_isKeepalive;
        };
    }
}

#endif // TIDE_HTTP_SERVER_H__
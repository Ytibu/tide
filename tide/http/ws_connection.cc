#include "ws_connection.h"

namespace tide
{
    namespace http
    {
        WSConnection::WSConnection(Socket::ptr sock, bool owner)
        :HttpConnection(sock, owner)
        {

        }
        std::pair<HttpResult::ptr, WSConnection::ptr> WSConnection::Create(const std::string &url, uint64_t timeout_ms, const std::map<std::string, std::string> &headers)
        {
            Uri::ptr uri = Uri::Create(url);
            if (!uri)
            {
                return {std::make_shared<HttpResult>(HttpResult::Error::INVALID_URL, nullptr, "invalid url: " + url), nullptr};
            }
            return Create(uri, timeout_ms, headers);
        }

        std::pair<HttpResult::ptr, WSConnection::ptr> WSConnection::Create(Uri::ptr uri, uint64_t timeout_ms, const std::map<std::string, std::string> &headers)
        {
            Address::ptr addr = uri->createAddress();
            if (!addr){
                return {std::make_shared<HttpResult>(HttpResult::Error::INVALID_HOST, nullptr, "invalid host: " + uri->getHost()), nullptr};
            }

            Socket::ptr sock = Socket::CreateTCP(addr);
            if (!sock){
                return {std::make_shared<HttpResult>(HttpResult::Error::CONNECT_FAIL, nullptr, "create socket fail: " + uri->getHost()), nullptr};
            }

            if(!sock->connect(addr)){
                return {std::make_shared<HttpResult>(HttpResult::Error::CONNECT_FAIL, nullptr, "connect fail: " + uri->getHost()), nullptr};
            }

            sock->setRecvTimeout(timeout_ms);
            WSConnection::ptr conn = std::make_shared<WSConnection>(sock);
            HttpRequest::ptr req = std::make_shared<HttpRequest>();
            req->setPath(uri->getPath());
            req->setQuery(uri->getQuery());
            req->setFragment(uri->getFragment());
            req->setMethod(HttpMethod::HTTP_GET);

            bool has_host = false;
            bool has_conn = false;
            for(auto &i: headers)
            {
                if(strcasecmp(i.first.c_str(), "connection") == 0){
                    has_conn = true;
                }
                else if(strcasecmp(i.first.c_str(), "host") == 0  ){
                    has_host = !i.second.empty();
                }
                req->setHeader(i.first, i.second);
            }
            req->setWebsocket(true);

            if(!has_conn){
                req->setHeader("Connection", "Upgrade");
            }

            req->setHeader("Upgrade", "websocket");
            req->setHeader("Sec-WebSocket-Version", "13");
            req->setHeader("Sec-WebSocket-Key", tide::base64encode(random_string(16)));
            if(!has_host){
                req->setHeader("Host", uri->getHost());
            }

            int rt = conn->sendRequest(req);
            if(rt == 0){
                return std::make_pair(std::make_shared<HttpResult>(HttpResult::Error::OK, nullptr, "ok"), conn);
            }

        }

        WSFrameMessage::ptr WSConnection::recvMessage()
        {

        }
        int32_t WSConnection::sendMessage(WSFrameMessage::ptr msg, bool fin)
        {

        }
        int32_t WSConnection::sendMessage(const std::string &msg, int opcode = WSFrameHeader::TEXT_FRAME, bool fin = true)
        {
            return 0;
        }
        int32_t WSConnection::ping()
        {
            return 0;
        }
        int32_t WSConnection::pong()
        {
            return 0;
        }
    }
}
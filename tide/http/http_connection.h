#ifndef TIDE_HTTP_CONNECTION_H
#define TIDE_HTTP_CONNECTION_H

#include "../socket_stream.h"
#include "http.h"

namespace tide
{
    namespace http
    {
        // 表示一个 HTTP 连接，封装了发送 HTTP 请求和接收 HTTP 响应的功能
        class HttpConnection : public SocketStream
        {
        public:
            using ptr = std::shared_ptr<HttpConnection>;

            // 构造函数，owner 表示是否由 HttpConnection 负责管理 Socket 的生命周期
            HttpConnection(Socket::ptr sock, bool owner = true);
            
            // 接收 HTTP 响应
            HttpResponse::ptr recvResponse();
            // 发送 HTTP 请求
            int sendRequest(HttpRequest::ptr req);

        private:
            
        };

    } // namespace http

} // namespace tide

#endif // TIDE_HTTP_CONNECTION_H
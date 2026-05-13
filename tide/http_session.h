#ifndef TIDE_HTTP_SESSION_H
#define TIDE_HTTP_SESSION_H

#include "socket_stream.h"
#include "http.h"

namespace tide
{
    namespace http
    {
        class HttpSession : public SocketStream
        {
        public:
            using ptr = std::shared_ptr<HttpSession>;

            // 构造函数，owner 表示是否由 HttpSession 负责管理 Socket 的生命周期
            HttpSession(Socket::ptr sock, bool owner = true);
            
            // 接收 HTTP 请求
            HttpRequest::ptr recvRequest();

            // 发送 HTTP 响应
            int sendResponse(HttpResponse::ptr rsp);

        private:
            
        };

    } // namespace http

} // namespace tide

#endif // TIDE_HTTP_SESSION_H
#ifndef TIDE_HTTP_SESSION_H
#define TIDE_HTTP_SESSION_H

#include "../socket_stream.h"
#include "http.h"

namespace tide
{
    namespace http
    {
        // 表示一个 HTTP 会话，负责处理一个 HTTP 连接，提供接收 HTTP 请求和发送 HTTP 响应的功能
        class HttpSession : public SocketStream
        {
        public:
            using ptr = std::shared_ptr<HttpSession>;

            /**
             * @brief 利用socket指针构造 HttpSession 对象，owner 参数表示是否拥有这个 socket
             *  如果拥有在 HttpSession 对象销毁时会自动关闭这个 socket，否则 HttpSession 对象销毁时不会关闭这个 socket
             * 
             * @param sock 
             * @param owner 
             */
            HttpSession(Socket::ptr sock, bool owner = true);
            
            /**
             * @brief 接收 HTTP 请求并返回，以便后续操作
             * 
             * @return HttpRequest::ptr 
             */
            HttpRequest::ptr recvRequest();

            /**
             * @brief 发送指定的Respoense作为 HTTP 响应
             * 
             * @param rsp 
             * @return int 
             */
            int sendResponse(HttpResponse::ptr rsp);  
        };

    } // namespace http

} // namespace tide

#endif // TIDE_HTTP_SESSION_H
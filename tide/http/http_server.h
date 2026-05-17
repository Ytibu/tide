#ifndef TIDE_HTTP_SERVER_H__
#define TIDE_HTTP_SERVER_H__

#include <memory>

#include "../iomanager.h"
#include "../tcp_server.h"
#include "servlet.h"

namespace tide
{
    namespace http
    {
        /**
         * @brief HttpServer层，提供可以继承的handleClient方法，
         * 默认的handleClient方法会创建一个HttpSession对象来处理HTTP请求，用户可以继承HttpServer类并重写handleClient方法来实现自定义的HTTP请求处理逻辑
         * 
         */

         
        // HttpServer 是一个 HTTP 服务器类，继承自 TcpServer，重写了 handleClient 方法来处理客户端请求
        class HttpServer : public TcpServer
        {
        public:
            using ptr = std::shared_ptr<HttpServer>;

            /**
             * @brief 利用io工作线程和io接收线程来构造一个HttpServer
             * 
             * @param keepalive 
             * @param worker 
             * @param accept_worker 
             */
            HttpServer(bool keepalive = false, 
                tide::IOManager* worker = tide::IOManager::GetThis(),
                tide::IOManager* accept_worker = tide::IOManager::GetThis()
            );

            /**
             * @brief 获取通配符Servlet
             * 
             * @return ServletDispatch::ptr 
             */
            ServletDispatch::ptr getServletDispatch() const { return m_dispatch; }

            /**
             * @brief 设置通配符Servlet
             * 
             * @param v 
             */
            void setServletDispatch(ServletDispatch::ptr v) { m_dispatch = v; }

        protected:
            /**
             * @brief 处理客户端请求
             * 
             * @param client 
             */
            virtual void handleClient(Socket::ptr client) override;
        private:
            bool m_isKeepalive;
            ServletDispatch::ptr m_dispatch;
        };
    }
}

#endif // TIDE_HTTP_SERVER_H__
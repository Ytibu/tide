#ifndef TIDE_HTTP_CONNECTION_H
#define TIDE_HTTP_CONNECTION_H

#include <memory>

#include <stdint.h>

#include "http.h"
#include "../socket_stream.h"
#include "../uri.h"
#include "../thread.h"

namespace tide
{
    namespace http
    {
        struct HttpResult
        {
            using ptr = std::shared_ptr<HttpResult>;

            enum class Error
            {
                OK = 0,
                INVALID_URL = -1,
                INVALID_HOST = -2,
                CONNECT_FAIL = -3,
                SEND_FAIL = -4,
                RECV_FAIL = -5,
                PARSE_RESPONSE_FAIL = -6,
                UNKNOWN = -100,
            };

            HttpResult(int d_result, HttpResponse::ptr d_response, const std::string &d_error)
                : result(d_result), response(d_response), error(d_error)
            {
            }

            int result;
            HttpResponse::ptr response;
            std::string error;
        };

        // 表示一个 HTTP 连接，封装了发送 HTTP 请求和接收 HTTP 响应的功能
        class HttpConnection : public SocketStream
        {
        public:
            using ptr = std::shared_ptr<HttpConnection>;

            // 提供静态方法，方便用户直接通过 URL 发起 GET/POST 请求，内部会创建 HttpRequest 对象并调用 DoRequest。
            static HttpResult::ptr DoGET(const std::string url, uint64_t timout_ms,
                                         const std::map<std::string, std::string> &headrs = {},
                                         const std::string &body = "");
            static HttpResult::ptr DoGET(Uri::ptr uri, uint64_t timout_ms,
                                         const std::map<std::string, std::string> &headrs = {},
                                         const std::string &body = "");
            static HttpResult::ptr DoPOST(const std::string url, uint64_t timout_ms,
                                          const std::map<std::string, std::string> &headrs = {},
                                          const std::string &body = "");
            static HttpResult::ptr DoPOST(Uri::ptr uri, uint64_t timout_ms,
                                          const std::map<std::string, std::string> &headrs = {},
                                          const std::string &body = "");

            // 发送 HTTP 请求的核心方法，接收一个 HttpRequest 对象和一个 Uri 对象，负责将请求发送到服务器并等待响应。
            static HttpResult::ptr DoRequest(HttpMethod method, const std::string url, uint64_t timout_ms,
                                             const std::map<std::string, std::string> &headrs = {},
                                             const std::string &body = "");
            static HttpResult::ptr DoRequest(HttpMethod method, Uri::ptr uri, uint64_t timout_ms,
                                             const std::map<std::string, std::string> &headrs = {},
                                             const std::string &body = "");
            static HttpResult::ptr DoRequest(HttpRequest::ptr req, Uri::ptr uri, uint64_t timout_ms);

            // 构造函数，owner 表示是否由 HttpConnection 负责管理 Socket 的生命周期
            HttpConnection(Socket::ptr sock, bool owner = true);

            // 接收 HTTP 响应
            HttpResponse::ptr recvResponse();
            // 发送 HTTP 请求
            int sendRequest(HttpRequest::ptr req);
        };

        class HttpConnectionPool
        {
        public:
            using ptr = std::shared_ptr<HttpConnectionPool>;
            using MutexType = Mutex;

            HttpConnectionPool(const std::string &host,
                               const std::string &vhost,
                               uint32_t port,
                               uint32_t maxSize,
                               uint32_t maxAliveTime,
                               uint32_t maxRequest);

            HttpConnectionPool::ptr getConnection();

            HttpResult::ptr doGET(const std::string &url,
                                  uint64_t timeout_ms,
                                  const std::map<std::string, std::string> &headers = {},
                                  const std::string &body = "");

            HttpResult::ptr doGETr(Uri::ptr uri,
                                   uint64_t timeout_ms,
                                   const std::map<std::string, std::string> &headers = {},
                                   const std::string &body = "");

            HttpResult::ptr doPOST(const std::string &url,
                                   uint64_t timeout_ms,
                                   const std::map<std::string, std::string> &headers = {},
                                   const std::string &body = "");

            HttpResult::ptr doPOSTr(Uri::ptr uri,
                                    uint64_t timeout_ms,
                                    const std::map<std::string, std::string> &headers = {},
                                    const std::string &body = "");

            HttpResult::ptr DoRequest(HttpMethod method,
                                      const std::string &url,
                                      uint64_t timeout_ms,
                                      const std::map<std::string, std::string> &headers = {},
                                      const std::string &body = "");
            HttpResult::ptr DoRequestR(HttpMethod method,
                                      Uri::ptr uri,
                                      uint64_t timeout_ms,
                                      const std::map<std::string, std::string> &headers = {},
                                      const std::string &body = "");

            HttpRequest::ptr DoRequest(HttpRequest::ptr req,
                                       Uri::ptr uri,
                                       uint64_t timeout_ms,
                                       const std::map<std::string, std::string> &headers = {},
                                       const std::string &body = "");

        private:
            static void ReleasePtr(HttpConnection *ptr, HttpConnectionPool *pool);

        private:
            std::string m_host;
            std::string m_vhost;
            uint32_t m_port;
            uint32_t m_maxSize;
            uint32_t m_maxAliveTime;
            uint32_t m_maxRequest;

            MutexType m_mutex;
            std::list<HttpConnection *> m_conns;
            std::atomic<int32_t> m_total = {0};
        };

    } // namespace http

} // namespace tide

#endif // TIDE_HTTP_CONNECTION_H
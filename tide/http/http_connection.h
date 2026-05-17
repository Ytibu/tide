#ifndef TIDE_HTTP_CONNECTION_H
#define TIDE_HTTP_CONNECTION_H

#include <memory>
#include <list>
#include <map>
#include <string>
#include <atomic>

#include <stdint.h>

#include "http.h"
#include "../socket_stream.h"
#include "../uri.h"
#include "../thread.h"

namespace tide
{
    namespace http
    {

        /**
         * @brief HttpConnection层，提供简单的静态接口，封装了发送 HTTP 请求和接收 HTTP 响应的功能，
         * 用户可以直接调用这些静态方法来完成 HTTP 请求，而不需要手动创建 HttpRequest 对象和 HttpConnection 对象。
         * 
         * HttpResult 结构体用于封装 HTTP 请求的结果，包括请求是否成功、HTTP 响应对象和错误信息等。
         * 通过解析返回的HttpResult对象，用户可以获取 HTTP 响应的状态码、响应头部和响应主体等信息，并根据请求结果进行相应的处理。
         */

         
        struct HttpResult
        {
            using ptr = std::shared_ptr<HttpResult>;

            /**
             * @brief HttpResult的错误码
             * 
             */
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

            /**
             * @brief HttpResult 结构体用于封装 HTTP 请求的结果，包括请求是否成功、HTTP 响应对象和错误信息等。
             * result 字段表示请求结果的状态，response 字段是一个指向 HttpResponse 对象的智能指针，error 字段是一个字符串，用于描述错误信息。
             * 
             * @param d_result 
             * @param d_response 
             * @param d_error 
             */
            HttpResult(int d_result, HttpResponse::ptr d_response, const std::string &d_error)
                : result(d_result), response(d_response), error(d_error)
            {
            }

            /**
             * @brief HttpResult格式化字符串
             * 
             * @return std::string 
             */
            std::string toString()
            {
                std::stringstream ss;
                ss << "result = " << result 
                << "error = " << error;

                return ss.str();
            }

            int result;
            HttpResponse::ptr response;
            std::string error;
        };

        // 表示一个 HTTP 连接，封装了发送 HTTP 请求和接收 HTTP 响应的功能
        class HttpConnection : public SocketStream
        {
            friend class HttpConnectionPool;

        public:
            using ptr = std::shared_ptr<HttpConnection>;

            /**
             * @brief 提供了一系列静态方法，来完成 HTTP 请求，而不需要手动创建 HttpRequest 对象和 HttpConnection 对象。
             * 
             * @param url 字符串形式的 URL，表示请求的目标地址
             * @param timout_ms 请求的超时时间，单位毫秒
             * @param headrs  HTTP 请求头部信息，使用一个字符串到字符串的映射来表示
             * @param body  HTTP 请求的主体内容，通常用于 POST 请求
             * @return HttpResult::ptr HTTP 请求的结果，包含请求是否成功、HTTP 响应对象和错误信息等
             */
            static HttpResult::ptr DoGET(const std::string url, uint64_t timout_ms,
                                         const std::map<std::string, std::string> &headrs = {},
                                         const std::string &body = "");
            static HttpResult::ptr DoGET(Uri::ptr uri, uint64_t timout_ms,
                                         const std::map<std::string, std::string> &headrs = {},
                                         const std::string &body = "");

            /**
             * @brief 静态的 DoPOST 方法用于发送 HTTP POST 请求，用于向服务器发送数据。
             * 
             * @param url 
             * @param timout_ms 
             * @param headrs 
             * @param body 
             * @return HttpResult::ptr 
             */
            static HttpResult::ptr DoPOST(const std::string url, uint64_t timout_ms,
                                          const std::map<std::string, std::string> &headrs = {},
                                          const std::string &body = "");
            static HttpResult::ptr DoPOST(Uri::ptr uri, uint64_t timout_ms,
                                          const std::map<std::string, std::string> &headrs = {},
                                          const std::string &body = "");

            /**
             * @brief 静态的HTTP请求方法，提供了一个通用的接口，
             * 可以发送任意类型的 HTTP 请求（GET、POST、PUT、DELETE 等），通过 method 参数指定请求方法。
             * 
             * @param method 
             * @param url 
             * @param timout_ms 
             * @param headrs 
             * @param body 
             * @return HttpResult::ptr 
             */
            static HttpResult::ptr DoRequest(HttpMethod method, const std::string url, uint64_t timout_ms,
                                             const std::map<std::string, std::string> &headrs = {},
                                             const std::string &body = "");
            static HttpResult::ptr DoRequest(HttpMethod method, Uri::ptr uri, uint64_t timout_ms,
                                             const std::map<std::string, std::string> &headrs = {},
                                             const std::string &body = "");
            
            /**
             * @brief 静态的 DoRequest 方法是 HTTP 请求的核心方法，
             * 接收一个 HttpRequest 对象和一个 Uri 对象，负责发送 HTTP 请求并接收 HTTP 响应。
             * 
             * @param req HttpRequest 对象，封装了 HTTP 请求的相关信息，如请求方法、请求路径、请求头部和请求主体等
             * @param uri Uri 对象，表示请求的目标地址，包含了主机、端口、路径等信息
             * @param timout_ms 
             * @return HttpResult::ptr 
             */
            static HttpResult::ptr DoRequest(HttpRequest::ptr req, Uri::ptr uri, uint64_t timout_ms);

            /**
             * @brief 构造函数，owner 表示是否由 HttpConnection 负责管理 Socket 的生命周期
             * 
             * @param sock 
             * @param owner 
             */
            HttpConnection(Socket::ptr sock, bool owner = true);

            /**
             * @brief 接收 HTTP 响应
             * 
             * @return HttpResponse::ptr 接收的 HTTP 响应对象，包含了响应状态码、响应头部和响应主体等信息
             */
            HttpResponse::ptr recvResponse();
            
            /**
             * @brief 发送 HTTP 请求
             * 
             * @param req HttpRequest 对象，封装了 HTTP 请求的相关信息，如请求方法、请求路径、请求头部和请求主体等
             * @return int 发送的字节数，如果发送失败则返回 -1
             */
            int sendRequest(HttpRequest::ptr req);

        private:
            uint64_t m_createTime = 0;   // 连接创建时间，单位毫秒
            uint64_t m_requestCount = 0; // 已经发送的请求数量
        };

        // 表示一个 HTTP 连接池，负责管理多个 HttpConnection 对象，提供获取连接和发送请求的功能
        class HttpConnectionPool
        {
        public:
            using ptr = std::shared_ptr<HttpConnectionPool>;
            using MutexType = Mutex;

            /**
             * @brief 构造函数， 构造一个连接池
             * 
             * @param host 
             * @param vhost 
             * @param port 
             * @param maxSize 
             * @param maxAliveTime 
             * @param maxRequest 
             */
            HttpConnectionPool(const std::string &host,
                               const std::string &vhost,
                               uint32_t port,
                               uint32_t maxSize,
                               uint32_t maxAliveTime,
                               uint32_t maxRequest);

            /**
             * @brief 负责从连接池中获取一个连接对象，
             * 如果连接池中有可用的连接对象则直接返回，否则创建一个新的连接对象并返回
             * 
             * @return HttpConnection::ptr 
             */
            HttpConnection::ptr getConnection();

            /**
             * @brief 用GET + url 等参数，调用doRequest方法
             * 
             * @param url 字符串url
             * @param timeout_ms 
             * @param headers 
             * @param body 
             * @return HttpResult::ptr 
             */
            HttpResult::ptr doGET(const std::string &url,
                                  uint64_t timeout_ms,
                                  const std::map<std::string, std::string> &headers = {},
                                  const std::string &body = "");

            /**
             * @brief 解析uri为url并调用doRequest方法
             * 
             * @param uri 
             * @param timeout_ms 
             * @param headers 
             * @param body 
             * @return HttpResult::ptr 
             */
            HttpResult::ptr doGET(Uri::ptr uri,
                                  uint64_t timeout_ms,
                                  const std::map<std::string, std::string> &headers = {},
                                  const std::string &body = "");

            /**
             * @brief 用POST + url 调用 doRequest方法
             * 
             * @param url 
             * @param timeout_ms 
             * @param headers 
             * @param body 
             * @return HttpResult::ptr 
             */
            HttpResult::ptr doPOST(const std::string &url,
                                   uint64_t timeout_ms,
                                   const std::map<std::string, std::string> &headers = {},
                                   const std::string &body = "");

            /**
             * @brief 解析uri为url并调用doRequest
             * 
             * @param uri 
             * @param timeout_ms 
             * @param headers 
             * @param body 
             * @return HttpResult::ptr 
             */
            HttpResult::ptr doPOST(Uri::ptr uri,
                                   uint64_t timeout_ms,
                                   const std::map<std::string, std::string> &headers = {},
                                   const std::string &body = "");
            
            /**
             * @brief 解析生成HttpRequest
             * 
             * @param method 
             * @param url 
             * @param timeout_ms 
             * @param headers 
             * @param body 
             * @return HttpResult::ptr 
             */
            HttpResult::ptr doRequest(HttpMethod method,
                                      const std::string &url,
                                      uint64_t timeout_ms,
                                      const std::map<std::string, std::string> &headers = {},
                                      const std::string &body = "");

            HttpResult::ptr doRequest(HttpMethod method,
                                      Uri::ptr uri,
                                      uint64_t timeout_ms,
                                      const std::map<std::string, std::string> &headers = {},
                                      const std::string &body = "");

            /**
             * @brief 连接池的核心方法，接收一个 HttpRequest 指针对象，
             * 负责从连接池中获取一个连接对象，将请求发送到服务器并等待响应，最后返回 HttpResult 对象封装请求结果。
             *
             * @param req   HttpRequest 对象，封装了 HTTP 请求的相关信息，如请求方法、请求路径、请求头部和请求主体等
             * @param timeout_ms
             * @return HttpResult::ptr 返回的 HttpResult 对象封装了 HTTP 请求的结果，包括请求是否成功、HTTP 响应对象和错误信息等。
             */
            HttpResult::ptr doRequest(HttpRequest::ptr req, uint64_t timeout_ms);

        private:
            /**
             * @brief 超过限制的释放连接对象，否则将连接对象放回连接池中，供其他请求使用
             *
             * @param ptr
             * @param pool
             */
            static void ReleasePtr(HttpConnection *ptr, HttpConnectionPool *pool);

        private:
            // 连接池的基本信息，包括主机、端口、最大连接数、最大存活时间和最大请求数等
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
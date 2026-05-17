#ifndef TIDE_HTTP_PARSER_H__
#define TIDE_HTTP_PARSER_H__

#include <memory>

#include "http.h"
#include "http11/http11_parser.h"
#include "http11/httpclient_parser.h"

namespace  tide
{
    namespace http
    {

        /**
         * @brief HttpParser层，提供 HTTP 请求和响应的解析功能，封装了 http_parser 和 httpclient_parser 库的使用细节
         * 
         */


        // 表示一个 HTTP 请求解析器，负责解析 HTTP 请求数据并构造 HttpRequest 对象
        class HttpRequestParser
        {
        public:
            using ptr = std::shared_ptr<HttpRequestParser>;

            /**
             * @brief 构造函数
             * 
             */
            HttpRequestParser();

            /**
             * @brief 判断 HTTP 请求是否解析完成
             * 
             * @return int 
             */
            int isFinished() { return http_parser_is_finished(&m_parser); }

            /**
             * @brief 判断 HTTP 请求是否解析出错
             * 
             * @return int 
             */
            int hasError() { return m_error || http_parser_has_error(&m_parser); }
            
            /**
             * @brief 解析 HTTP 请求数据
             * 
             * @param data 
             * @param len 
             * @return size_t 
             */
            size_t execute(char *data, size_t len);

            /**
             * @brief 获取解析后的 HTTP 请求对象
             * 
             * @return HttpRequest::ptr 
             */
            HttpRequest::ptr getRequest() const { return m_request; }
            
            /**
             * @brief 设置错误状态
             * 
             * @param v 
             */
            void setError(int v) { m_error = v; }

            /**
             * @brief 获取内容长度
             * 
             * @return uint64_t 
             */
            uint64_t getContentLength();

            /**
             * @brief 获取 HTTP 请求解析器的引用
             * 
             * @return const http_parser& 
             */
            const http_parser &getParser() const { return m_parser; }

        public:
            /**
             * @brief 获取 HTTP 请求缓冲区大小
             * 
             * @return uint64_t 
             */
            static uint64_t GetHttpRequestBufferSize();
            /**
             * @brief 获取 HTTP 请求最大主体大小
             * 
             * @return uint64_t 
             */
            static uint64_t GetHttpRequestMaxBodySize();

        private:
            http_parser m_parser;
            HttpRequest::ptr m_request;
            int m_error;
        };

        // 表示一个 HTTP 响应解析器，负责解析 HTTP 响应数据并构造 HttpResponse 对象
        class HttpResponseParser
        {
        public:
            using ptr = std::shared_ptr<HttpResponseParser>;

            /**
             * @brief 构造函数
             * 
             */
            HttpResponseParser();

            /**
             * @brief 判断 HTTP 响应是否解析完成
             * 
             * @return int 
             */
            int isFinished() { return httpclient_parser_is_finished(&m_parser); }

            /**
             * @brief 判断 HTTP 响应是否解析出错
             * 
             * @return int 
             */
            int hasError() { return m_error || httpclient_parser_has_error(&m_parser); }

            /**
             * @brief 解析 HTTP 响应数据
             * 
             * @param data 
             * @param len 
             * @param chunked 
             * @return size_t 
             */
            size_t execute(char *data, size_t len, bool chunked);

            /**
             * @brief 获取解析后的 HTTP 响应对象
             * 
             * @return HttpResponse::ptr 
             */
            HttpResponse::ptr getResponse() const { return m_response; }

            /**
             * @brief 设置错误状态
             * 
             * @param v 
             */
            void setError(int v) { m_error = v; }

            /**
             * @brief 获取内容长度
             * 
             * @return uint64_t 
             */
            uint64_t getContentLength();

            /**
             * @brief 获取 HTTP 响应解析器的引用
             * 
             * @return const httpclient_parser& 
             */
            const httpclient_parser &getParser() const { return m_parser; }
        public:
            /**
             * @brief 获取 HTTP 响应缓冲区大小
             * 
             * @return uint64_t 
             */
            static uint64_t GetHttpResponseBufferSize();
            
            /**
             * @brief 获取 HTTP 响应最大主体大小
             * 
             * @return uint64_t 
             */
            static uint64_t GetHttpResponseMaxBodySize();

        private:
            httpclient_parser m_parser;
            HttpResponse::ptr m_response;
            int m_error;
        };
    }
}

#endif // TIDE_HTTP_PARSER_H__
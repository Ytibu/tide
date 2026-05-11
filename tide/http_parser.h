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
        class HttpRequestParser
        {
        public:
            using ptr = std::shared_ptr<HttpRequestParser>;

            HttpRequestParser();

            int isFinished() { return http_parser_is_finished(&m_parser); }
            int hasError() { return m_error || http_parser_has_error(&m_parser); }
            size_t execute(char *data, size_t len);

            HttpRequest::ptr getRequest() const { return m_request; }
            void setError(int v) { m_error = v; }

            uint64_t getContentLength();

        private:
            http_parser m_parser;
            HttpRequest::ptr m_request;
            int m_error;
        };

        class HttpResponseParser
        {
        public:
            using ptr = std::shared_ptr<HttpResponseParser>;

            HttpResponseParser();

            int isFinished() { return httpclient_parser_is_finished(&m_parser); }
            int hasError() { return m_error || httpclient_parser_has_error(&m_parser); }
            size_t execute(char *data, size_t len);

            HttpResponse::ptr getResponse() const { return m_response; }
            void setError(int v) { m_error = v; }

            uint64_t getContentLength();

        private:
            httpclient_parser m_parser;
            HttpResponse::ptr m_response;
            int m_error;
        };
    }
}

#endif // TIDE_HTTP_PARSER_H__
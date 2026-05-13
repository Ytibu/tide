#include "http_parser.h"

#include "../log.h"
#include "../config.h"

namespace tide
{

    namespace http
    {

        static tide::Logger::ptr g_logger = TIDE_LOG_NAME("system");

        static tide::ConfigVar<uint64_t>::ptr g_http_request_buffer_size =
            tide::Config::Lookup("http.request.buffer_size", (uint64_t)(4 * 1024ull), "http request buffer size");
        static tide::ConfigVar<uint64_t>::ptr g_http_request_max_body_size =
            tide::Config::Lookup("http.request.max_body_size", (uint64_t)(64 * 1024 * 1024ull), "http request max body size");

        static uint64_t s_http_request_buffer_size = g_http_request_buffer_size->getValue();
        static uint64_t s_http_request_max_body_size = g_http_request_max_body_size->getValue();

        uint64_t HttpRequestParser::GetHttpRequestBufferSize()
        {
            return s_http_request_buffer_size;
        }

        uint64_t HttpRequestParser::GetHttpResponseBufferSize()
        {
            return s_http_request_max_body_size;
        }

        namespace
        {
            struct _RequestSizeIniter
            {
                _RequestSizeIniter()
                {
                    g_http_request_buffer_size->addListener([](const uint64_t &old_value, const uint64_t &new_value)
                                                            {
                    TIDE_LOG_INFO(g_logger) << "http request buffer size changed from " << old_value << " to " << new_value;
                    s_http_request_buffer_size = new_value; });
                    g_http_request_max_body_size->addListener([](const uint64_t &old_value, const uint64_t &new_value)
                                                              {
                    TIDE_LOG_INFO(g_logger) << "http request max body size changed from " << old_value << " to " << new_value;
                    s_http_request_max_body_size = new_value; });
                }
            };
            static _RequestSizeIniter _init;
        }

        void on_request_http_field(void *data, const char *field, size_t flen, const char *value, size_t vlen)
        {
            HttpRequestParser *parser = static_cast<HttpRequestParser *>(data);
            if (flen == 0)
            {
                TIDE_LOG_WARN(g_logger) << "invalid http header field length: " << flen;
                parser->setError(1002);
                return;
            }
            parser->getRequest()->setHeader(std::string(field, flen), std::string(value, vlen));
        }
        void on_request_method(void *data, const char *at, size_t length)
        {
            HttpRequestParser *parser = static_cast<HttpRequestParser *>(data);
            HttpMethod m = CharsToHttpMethod(at);

            if (m == HttpMethod::HTTP_INVALID_METHOD)
            {
                TIDE_LOG_WARN(g_logger) << "invalid http method: " << std::string(at, length);
                parser->setError(1000);
                return;
            }
            parser->getRequest()->setMethod(m);
        }

        void on_request_uri(void *data, const char *at, size_t length)
        {
            // HttpRequestParser *parser = static_cast<HttpRequestParser *>(data);
        }
        void on_request_fragment(void *data, const char *at, size_t length)
        {
            HttpRequestParser *parser = static_cast<HttpRequestParser *>(data);
            parser->getRequest()->setFragment(std::string(at, length));
        }
        void on_request_path(void *data, const char *at, size_t length)
        {
            HttpRequestParser *parser = static_cast<HttpRequestParser *>(data);
            parser->getRequest()->setPath(std::string(at, length));
        }
        void on_request_query_string(void *data, const char *at, size_t length)
        {
            HttpRequestParser *parser = static_cast<HttpRequestParser *>(data);
            parser->getRequest()->setQuery(std::string(at, length));
        }
        void on_request_http_version(void *data, const char *at, size_t length)
        {
            HttpRequestParser *parser = static_cast<HttpRequestParser *>(data);
            uint8_t version = 0;
            if (strncmp(at, "HTTP/1.0", length) == 0)
            {
                version = 0x10;
            }
            else if (strncmp(at, "HTTP/1.1", length) == 0)
            {
                version = 0x11;
            }
            else
            {
                TIDE_LOG_WARN(g_logger) << "invalid http version: " << std::string(at, length);
                parser->setError(1001);
                return;
            }
            parser->getRequest()->setVersion(version);
        }

        void on_request_header_done(void *data, const char *at, size_t length)
        {
            // HttpRequestParser *parser = static_cast<HttpRequestParser *>(data);
        }

        HttpRequestParser::HttpRequestParser()
            : m_error(0)
        {
            m_request.reset(new tide::http::HttpRequest);
            http_parser_init(&m_parser);
            m_parser.http_field = on_request_http_field;
            m_parser.request_method = on_request_method;
            m_parser.request_uri = on_request_uri;
            m_parser.fragment = on_request_fragment;
            m_parser.request_path = on_request_path;
            m_parser.query_string = on_request_query_string;
            m_parser.http_version = on_request_http_version;
            m_parser.header_done = on_request_header_done;
            m_parser.data = this;
        }

        size_t HttpRequestParser::execute(char *data, size_t len)
        {
            size_t off = http_parser_execute(&m_parser, data, len, 0);
            memmove(data, data + off, len - off);
            return off;
        }

        uint64_t HttpRequestParser::getContentLength()
        {
            return m_request->getHeaderAs<uint64_t>("content-length", 0);
        }

        void on_response_http_field(void *data, const char *field, size_t flen, const char *value, size_t vlen)
        {
            HttpResponseParser *parser = static_cast<HttpResponseParser *>(data);
            if (flen == 0)
            {
                TIDE_LOG_WARN(g_logger) << "invalid http header field length: " << flen;
                parser->setError(1002);
                return;
            }
            parser->getResponse()->setHeader(std::string(field, flen), std::string(value, vlen));
        }
        void on_response_reason_phrase(void *data, const char *at, size_t length)
        {
            HttpResponseParser *parser = static_cast<HttpResponseParser *>(data);
            parser->getResponse()->setReason(std::string(at, length));
        }

        void on_response_status_code(void *data, const char *at, size_t length)
        {
            HttpResponseParser *parser = static_cast<HttpResponseParser *>(data);
            int status = atoi(std::string(at, length).c_str());
            parser->getResponse()->setStatus((HttpStatus)status);
        }

        void on_response_chunk_size(void *data, const char *at, size_t length)
        {
            // HttpResponseParser *parser = static_cast<HttpResponseParser *>(data);
        }

        void on_response_http_version(void *data, const char *at, size_t length)
        {
            HttpResponseParser *parser = static_cast<HttpResponseParser *>(data);
            uint8_t version = 0;
            if (strncmp(at, "HTTP/1.0", length) == 0)
            {
                version = 0x10;
            }
            else if (strncmp(at, "HTTP/1.1", length) == 0)
            {
                version = 0x11;
            }
            else
            {
                TIDE_LOG_WARN(g_logger) << "invalid http version: " << std::string(at, length);
                parser->setError(1001);
                return;
            }
            parser->getResponse()->setVersion(version);
        }

        void on_response_header_done(void *data, const char *at, size_t length)
        {
            // HttpResponseParser *parser = static_cast<HttpResponseParser *>(data);
        }

        void on_response_last_chunk(void *data, const char *at, size_t length)
        {
        }

        HttpResponseParser::HttpResponseParser()
            : m_error(0)
        {
            httpclient_parser_init(&m_parser);
            m_response.reset(new tide::http::HttpResponse);
            m_parser.http_field = on_response_http_field;
            m_parser.reason_phrase = on_response_reason_phrase;
            m_parser.status_code = on_response_status_code;
            m_parser.chunk_size = on_response_chunk_size;
            m_parser.http_version = on_response_http_version;
            m_parser.header_done = on_response_header_done;
            m_parser.last_chunk = on_response_last_chunk;
            m_parser.data = this;
        }

        size_t HttpResponseParser::execute(char *data, size_t len)
        {
            size_t off = httpclient_parser_execute(&m_parser, data, len, 0);
            memmove(data, data + off, len - off);
            return off;
        }
        uint64_t HttpResponseParser::getContentLength()
        {
            return m_response->getHeaderAs<uint64_t>("content-length", 0);
        }

    } // namespace http

} // namespace tide
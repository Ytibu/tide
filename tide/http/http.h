#ifndef TIDE_HTTP_H__
#define TIDE_HTTP_H__

#include <map>
#include <string>
#include <memory>
#include <iostream>
#include <sstream>

#include <boost/lexical_cast.hpp>

#include <strings.h>

#include "http11/http11_parser.h"
#include "http11/httpclient_parser.h"

namespace tide
{
    namespace http
    {

        /* Status Codes 例如: HTTP_STATUS_OK */
#define HTTP_STATUS_MAP(XX)                                                   \
    XX(100, CONTINUE, Continue)                                               \
    XX(101, SWITCHING_PROTOCOLS, Switching Protocols)                         \
    XX(102, PROCESSING, Processing)                                           \
    XX(200, OK, OK)                                                           \
    XX(201, CREATED, Created)                                                 \
    XX(202, ACCEPTED, Accepted)                                               \
    XX(203, NON_AUTHORITATIVE_INFORMATION, Non - Authoritative Information)   \
    XX(204, NO_CONTENT, No Content)                                           \
    XX(205, RESET_CONTENT, Reset Content)                                     \
    XX(206, PARTIAL_CONTENT, Partial Content)                                 \
    XX(207, MULTI_STATUS, Multi - Status)                                     \
    XX(208, ALREADY_REPORTED, Already Reported)                               \
    XX(226, IM_USED, IM Used)                                                 \
    XX(300, MULTIPLE_CHOICES, Multiple Choices)                               \
    XX(301, MOVED_PERMANENTLY, Moved Permanently)                             \
    XX(302, FOUND, Found)                                                     \
    XX(303, SEE_OTHER, See Other)                                             \
    XX(304, NOT_MODIFIED, Not Modified)                                       \
    XX(305, USE_PROXY, Use Proxy)                                             \
    XX(307, TEMPORARY_REDIRECT, Temporary Redirect)                           \
    XX(308, PERMANENT_REDIRECT, Permanent Redirect)                           \
    XX(400, BAD_REQUEST, Bad Request)                                         \
    XX(401, UNAUTHORIZED, Unauthorized)                                       \
    XX(402, PAYMENT_REQUIRED, Payment Required)                               \
    XX(403, FORBIDDEN, Forbidden)                                             \
    XX(404, NOT_FOUND, Not Found)                                             \
    XX(405, METHOD_NOT_ALLOWED, Method Not Allowed)                           \
    XX(406, NOT_ACCEPTABLE, Not Acceptable)                                   \
    XX(407, PROXY_AUTHENTICATION_REQUIRED, Proxy Authentication Required)     \
    XX(408, REQUEST_TIMEOUT, Request Timeout)                                 \
    XX(409, CONFLICT, Conflict)                                               \
    XX(410, GONE, Gone)                                                       \
    XX(411, LENGTH_REQUIRED, Length Required)                                 \
    XX(412, PRECONDITION_FAILED, Precondition Failed)                         \
    XX(413, PAYLOAD_TOO_LARGE, Payload Too Large)                             \
    XX(414, URI_TOO_LONG, URI Too Long)                                       \
    XX(415, UNSUPPORTED_MEDIA_TYPE, Unsupported Media Type)                   \
    XX(416, RANGE_NOT_SATISFIABLE, Range Not Satisfiable)                     \
    XX(417, EXPECTATION_FAILED, Expectation Failed)                           \
    XX(421, MISDIRECTED_REQUEST, Misdirected Request)                         \
    XX(422, UNPROCESSABLE_ENTITY, Unprocessable Entity)                       \
    XX(423, LOCKED, Locked)                                                   \
    XX(424, FAILED_DEPENDENCY, Failed Dependency)                             \
    XX(426, UPGRADE_REQUIRED, Upgrade Required)                               \
    XX(428, PRECONDITION_REQUIRED, Precondition Required)                     \
    XX(429, TOO_MANY_REQUESTS, Too Many Requests)                             \
    XX(431, REQUEST_HEADER_FIELDS_TOO_LARGE, Request Header Fields Too Large) \
    XX(451, UNAVAILABLE_FOR_LEGAL_REASONS, Unavailable For Legal Reasons)     \
    XX(500, INTERNAL_SERVER_ERROR, Internal Server Error)                     \
    XX(501, NOT_IMPLEMENTED, Not Implemented)                                 \
    XX(502, BAD_GATEWAY, Bad Gateway)                                         \
    XX(503, SERVICE_UNAVAILABLE, Service Unavailable)                         \
    XX(504, GATEWAY_TIMEOUT, Gateway Timeout)                                 \
    XX(505, HTTP_VERSION_NOT_SUPPORTED, HTTP Version Not Supported)           \
    XX(506, VARIANT_ALSO_NEGOTIATES, Variant Also Negotiates)                 \
    XX(507, INSUFFICIENT_STORAGE, Insufficient Storage)                       \
    XX(508, LOOP_DETECTED, Loop Detected)                                     \
    XX(510, NOT_EXTENDED, Not Extended)                                       \
    XX(511, NETWORK_AUTHENTICATION_REQUIRED, Network Authentication Required)

        enum class HttpStatus
        {
#define XX(num, name, string) HTTP_STATUS_##name = num,
            HTTP_STATUS_MAP(XX)
#undef XX
        };
        /**
         * @brief 展开后：
         * enum class HttpStatus
         * {
         *   HTTP_STATUS_CONTINUE = 100,
         *   HTTP_STATUS_SWITCHING_PROTOCOLS = 101,
         *   HTTP_STATUS_PROCESSING = 102,
         *   HTTP_STATUS_OK = 200,
         *   // ... 其余状态码
         * };
         */

        /* Request Methods 例如: HTTP_GET */
#define HTTP_METHOD_MAP(XX)          \
    XX(0, DELETE, DELETE)            \
    XX(1, GET, GET)                  \
    XX(2, HEAD, HEAD)                \
    XX(3, POST, POST)                \
    XX(4, PUT, PUT)                  \
    /* pathological */               \
    XX(5, CONNECT, CONNECT)          \
    XX(6, OPTIONS, OPTIONS)          \
    XX(7, TRACE, TRACE)              \
    /* WebDAV */                     \
    XX(8, COPY, COPY)                \
    XX(9, LOCK, LOCK)                \
    XX(10, MKCOL, MKCOL)             \
    XX(11, MOVE, MOVE)               \
    XX(12, PROPFIND, PROPFIND)       \
    XX(13, PROPPATCH, PROPPATCH)     \
    XX(14, SEARCH, SEARCH)           \
    XX(15, UNLOCK, UNLOCK)           \
    XX(16, BIND, BIND)               \
    XX(17, REBIND, REBIND)           \
    XX(18, UNBIND, UNBIND)           \
    XX(19, ACL, ACL)                 \
    /* subversion */                 \
    XX(20, REPORT, REPORT)           \
    XX(21, MKACTIVITY, MKACTIVITY)   \
    XX(22, CHECKOUT, CHECKOUT)       \
    XX(23, MERGE, MERGE)             \
    /* upnp */                       \
    XX(24, MSEARCH, M - SEARCH)      \
    XX(25, NOTIFY, NOTIFY)           \
    XX(26, SUBSCRIBE, SUBSCRIBE)     \
    XX(27, UNSUBSCRIBE, UNSUBSCRIBE) \
    /* RFC-5789 */                   \
    XX(28, PATCH, PATCH)             \
    XX(29, PURGE, PURGE)             \
    /* CalDAV */                     \
    XX(30, MKCALENDAR, MKCALENDAR)   \
    /* RFC-2068, section 19.6.1.2 */ \
    XX(31, LINK, LINK)               \
    XX(32, UNLINK, UNLINK)           \
    /* icecast */                    \
    XX(33, SOURCE, SOURCE)

        enum class HttpMethod
        {
#define XX(num, name, string) HTTP_##name = num,
            HTTP_METHOD_MAP(XX)
#undef XX
                HTTP_INVALID_METHOD
        };

        /**
         * @brief 展开后：
         *
         * enum class HttpMethod
         * {
         *   HTTP_DELETE = 0,
         *   HTTP_GET = 1,
         *   HTTP_HEAD = 2,
         *   HTTP_POST = 3,
         *   HTTP_PUT = 4,
         *   // ... 其余操作码
         * };
         */

        /**
         * @brief 将字符串转换为 HTTP 方法
         * HTTP_GET --> HttpMethod::HTTP_GET
         *
         * @param method HTTP 方法字符串
         * @return HttpMethod
         */
        HttpMethod StringToHttpMethod(const std::string &method);

        /**
         * @brief 将字符数组转换为 HTTP 方法
         *
         * @param method HTTP 方法字符数组
         * @return HttpMethod
         */
        HttpMethod CharsToHttpMethod(const char *method);

        /**
         * @brief 将 HTTP 方法转换为字符串
         *
         * @param method HTTP 方法枚举值
         * @return const char*
         */
        const char *HttpMethodToString(const HttpMethod &method);

        /**
         * @brief 将 HTTP 状态码转换为字符串
         *
         * @param status HTTP 状态码枚举值
         * @return const char*
         */
        const char *HttpStatusToString(const HttpStatus &status);

        // HTTP 头部字段不区分大小写，因此使用自定义比较器实现 case-insensitive map
        struct CaseInsensitiveLess
        {
            bool operator()(const std::string &lhs, const std::string &rhs) const
            {
                return strcasecmp(lhs.c_str(), rhs.c_str()) < 0;
            }
        };

        /**
         * @brief 从 map 中检查并获取值，并转换为指定类型，如果键不存在或转换失败则返回false，并将 value 设置为默认值
         *
         * @tparam MapType
         * @tparam T
         * @param m 查找的 map
         * @param key 要查找的键
         * @param value 输出参数，用于存储转换后的值
         * @param default_value 默认值，当键不存在或转换失败时返回
         * @return bool 如果键存在且转换成功返回 true，否则返回 false，并将 value 设置为 default_value
         */
        template <typename MapType, typename T>
        bool CheckgetAs(const MapType &m, const std::string &key, T &value, const T &default_value = T())
        {
            auto it = m.find(key);
            if (it == m.end())
            {
                value = default_value;
                return false;
            }
            try
            {
                value = boost::lexical_cast<T>(it->second);
                return true;
            }
            catch (...)
            {
                value = default_value;
            }

            return false;
        }

        /**
         * @brief 从 map 中获取值并转换为指定类型，如果键不存在或转换失败则返回默认值
         *
         * @tparam MapType
         * @tparam T
         * @param m 查找的 map
         * @param key 要查找的键
         * @param default_value 默认值，当键不存在或转换失败时返回
         * @return T
         */
        template <typename MapType, typename T>
        T getAs(const MapType &m, const std::string &key, const T &default_value = T())
        {
            auto it = m.find(key);
            if (it == m.end())
            {
                return default_value;
            }
            try
            {
                return boost::lexical_cast<T>(it->second);
            }
            catch (...)
            {
            }
            return default_value;
        }

        class HttpRequest
        {
        public:
            using ptr = std::shared_ptr<HttpRequest>;
            using MapType = std::map<std::string, std::string, CaseInsensitiveLess>;

            /**
             * @brief 构造函数，默认 HTTP 版本为 1.1，连接默认保持close状态
             * 
             * @param version 
             * @param close 
             */
            HttpRequest(uint8_t version = 0x11, bool close = true);

            bool isClose() const { return m_close; }
            void setClose(bool close) { m_close = close; }

            HttpMethod getMethod() const { return m_method; }
            uint8_t getVersion() const { return m_version; }

            const std::string &getPath() const { return m_path; }
            const std::string &getQuery() const { return m_query; }
            const std::string &getFragment() const { return m_fragment; }
            const std::string &getBody() const { return m_body; }

            const MapType &getHeaders() const { return m_headers; }
            const MapType &getParameters() const { return m_parameters; }
            const MapType &getCookies() const { return m_cookies; }

            void setMethod(const HttpMethod &method) { m_method = method; }
            void setVersion(const uint8_t &version) { m_version = version; }

            void setPath(const std::string &path) { m_path = path; }
            void setQuery(const std::string &query) { m_query = query; }
            void setFragment(const std::string &fragment) { m_fragment = fragment; }
            void setBody(const std::string &body) { m_body = body; }

            void setHeaders(const MapType &headers) { m_headers = headers; }
            void setParameters(const MapType &parameters) { m_parameters = parameters; }
            void setCookies(const MapType &cookies) { m_cookies = cookies; }

            std::string getHeader(const std::string &key, const std::string &default_value = "") const;
            std::string getParameter(const std::string &key, const std::string &default_value = "") const;
            std::string getCookie(const std::string &key, const std::string &default_value = "") const;

            void setHeader(const std::string &key, const std::string &value) { m_headers[key] = value; }
            void setParameter(const std::string &key, const std::string &value) { m_parameters[key] = value; }
            void setCookie(const std::string &key, const std::string &value) { m_cookies[key] = value; }

            void delHeader(const std::string &key) { m_headers.erase(key); }
            void delParameter(const std::string &key) { m_parameters.erase(key); }
            void delCookie(const std::string &key) { m_cookies.erase(key); }

            bool hasHeader(const std::string &key, std::string *value = nullptr) const;
            bool hasParameter(const std::string &key, std::string *value = nullptr) const;
            bool hasCookie(const std::string &key, std::string *value = nullptr) const;

            template <typename T>
            bool checkGetHeaderAs(const std::string &key, T &value, const T &default_value = T()) const
            {
                return CheckgetAs(m_headers, key, value, default_value);
            }
            template <typename T>
            T getHeaderAs(const std::string &key, const T &default_value = T()) const
            {
                return getAs(m_headers, key, default_value);
            }

            template <typename T>
            bool checkGetParameterAs(const std::string &key, T &value, const T &default_value = T()) const
            {
                return CheckgetAs(m_parameters, key, value, default_value);
            }
            template <typename T>
            T getParameterAs(const std::string &key, const T &default_value = T()) const
            {
                return getAs(m_parameters, key, default_value);
            }

            std::string toString() const;
            std::ostream &dump(std::ostream &os) const;

        private:
            HttpMethod m_method;
            uint8_t m_version;
            bool m_close;

            std::string m_path;
            std::string m_query;
            std::string m_fragment;
            std::string m_body;

            MapType m_headers;
            MapType m_parameters;
            MapType m_cookies;
        };

        class HttpResponse
        {
        public:
            using ptr = std::shared_ptr<HttpResponse>;
            using MapType = std::map<std::string, std::string, CaseInsensitiveLess>;
            HttpResponse(uint8_t version = 0x11, bool close = true);

            HttpStatus getStatus() const { return m_status; }
            uint8_t getVersion() const { return m_version; }
            const std::string &getBody() const { return m_body; }
            const std::string &getReason() const { return m_reason; }
            const MapType &getHeaders() const { return m_headers; }

            void setStatus(const HttpStatus &status) { m_status = status; }
            void setVersion(const uint8_t &version) { m_version = version; }
            void setBody(const std::string &body) { m_body = body; }
            void setReason(const std::string &reason) { m_reason = reason; }
            void setHeaders(const MapType &headers) { m_headers = headers; }

            bool isClose() const { return m_close; }
            void setClose(bool close) { m_close = close; }

            void setHeader(const std::string &key, const std::string &value) { m_headers[key] = value; }
            void delHeader(const std::string &key) { m_headers.erase(key); }
            std::string getHeader(const std::string &key, const std::string &default_value = "") const;

            template <typename T>
            bool checkGetHeaderAs(const std::string &key, T &value, const T &default_value = T()) const
            {
                return CheckgetAs(m_headers, key, value, default_value);
            }
            template <typename T>
            T getHeaderAs(const std::string &key, const T &default_value = T()) const
            {
                return getAs(m_headers, key, default_value);
            }

            std::ostream &dump(std::ostream &os) const;
            std::string toString() const;

        private:
            HttpStatus m_status;
            uint8_t m_version;
            bool m_close;
            std::string m_body;
            std::string m_reason;
            MapType m_headers;
        };

        std::ostream &operator<<(std::ostream &os, const HttpRequest &req);
        std::ostream &operator<<(std::ostream &os, const HttpResponse &rsp);

    } // namespace http

} // namespace tide

#endif // TIDE_HTTP_H__
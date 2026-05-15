#include "http_connection.h"

#include <strings.h>

#include "http.h"
#include "http_parser.h"
#include "../log.h"

namespace tide
{
    namespace http
    {
        static Logger::ptr g_logger = TIDE_LOG_NAME("system");

        // 提供静态方法，方便用户直接通过 URL 发起 GET/POST 请求，内部会创建 HttpRequest 对象并调用 DoRequest。

        // 接收一个 URL 字符串，解析成 Uri 对象，如果解析失败则返回错误结果，否则调用重载的 DoGET 方法。
        HttpResult::ptr HttpConnection::DoGET(const std::string url,
                                              uint64_t timout_ms,
                                              const std::map<std::string, std::string> &headrs,
                                              const std::string &body)
        {
            Uri::ptr uri_ptr = Uri::Create(url);
            if (!uri_ptr)
            {
                return std::make_shared<HttpResult>((int)HttpResult::Error::INVALID_URL, nullptr, "invalid url: " + url);
            }
            return DoGET(uri_ptr, timout_ms, headrs, body);
        }

        // 重载 DoGET，接受 Uri 对象作为参数，方便用户直接使用解析好的 Uri 对象发起请求。
        HttpResult::ptr HttpConnection::DoGET(Uri::ptr uri,
                                              uint64_t timout_ms,
                                              const std::map<std::string, std::string> &headrs,
                                              const std::string &body)
        {
            return DoRequest(HttpMethod::HTTP_GET, uri, timout_ms, headrs, body);
        }

        // 接收一个 URL 字符串，解析成 Uri 对象，如果解析失败则返回错误结果，否则调用重载的 DoPOST 方法。
        HttpResult::ptr HttpConnection::DoPOST(const std::string url,
                                               uint64_t timout_ms,
                                               const std::map<std::string, std::string> &headrs,
                                               const std::string &body)
        {
            Uri::ptr uri_ptr = Uri::Create(url);
            if (!uri_ptr)
            {
                return std::make_shared<HttpResult>((int)HttpResult::Error::INVALID_URL, nullptr, "invalid url: " + url);
            }
            return DoPOST(uri_ptr, timout_ms, headrs, body);
        }

        // 重载 DoPOST，接受 Uri 对象作为参数，方便用户直接使用解析好的 Uri 对象发起请求。
        HttpResult::ptr HttpConnection::DoPOST(Uri::ptr uri,
                                               uint64_t timout_ms,
                                               const std::map<std::string, std::string> &headrs,
                                               const std::string &body)
        {
            return DoRequest(HttpMethod::HTTP_POST, uri, timout_ms, headrs, body);
        }

        // 发送 HTTP 请求的核心方法，接收一个 HttpRequest 对象和一个 Uri 对象，负责将请求发送到服务器并等待响应。
        HttpResult::ptr HttpConnection::DoRequest(HttpMethod method,
                                                  const std::string url,
                                                  uint64_t timout_ms,
                                                  const std::map<std::string, std::string> &headrs,
                                                  const std::string &body)
        {
            Uri::ptr uri_ptr = Uri::Create(url);
            if (!uri_ptr)
            {
                return std::make_shared<HttpResult>((int)HttpResult::Error::INVALID_URL, nullptr, "invalid url: " + url);
            }
            return DoRequest(method, uri_ptr, timout_ms, headrs, body);
        }

        // 解析 Uri 对象，构造 HttpRequest 对象，设置请求方法、路径、请求头和请求体，然后调用重载的 DoRequest 方法发送请求。
        HttpResult::ptr HttpConnection::DoRequest(HttpMethod method,
                                                  Uri::ptr uri,
                                                  uint64_t timout_ms,
                                                  const std::map<std::string, std::string> &headrs,
                                                  const std::string &body)
        {
            tide::http::HttpRequest::ptr req(new tide::http::HttpRequest);
            req->setMethod(method);
            req->setPath(uri->getPath());

            bool has_host = false;
            // 设置请求头
            for (const auto &kv : headrs)
            {
                if (strcasecmp(kv.first.c_str(), "connection") == 0)
                {
                    if (strcasecmp(kv.second.c_str(), "keep-alive") == 0)
                    {
                        req->setClose(false);
                    }
                    continue;
                }

                if (!has_host && strcasecmp(kv.first.c_str(), "host") == 0)
                {
                    has_host = !kv.second.empty();
                }

                req->setHeader(kv.first, kv.second);
            }

            if (!has_host)
            {
                req->setHeader("host", uri->getHost());
            }

            // 设置请求体
            req->setBody(body);
            return DoRequest(req, uri, timout_ms);
        }

        HttpResult::ptr HttpConnection::DoRequest(HttpRequest::ptr req,
                                                  Uri::ptr uri,
                                                  uint64_t timout_ms)
        {
            Address::ptr addr = uri->createAddress();
            if (!addr)
            {
                return std::make_shared<HttpResult>((int)HttpResult::Error::INVALID_HOST, nullptr, "invalid host: " + uri->getHost());
            }

            Socket::ptr sock = Socket::CreateTCP(addr);
            if (!sock)
            {
                return std::make_shared<HttpResult>((int)HttpResult::Error::CONNECT_FAIL, nullptr, "create socket fail for: " + addr->toString());
            }

            if (!sock->connect(addr))
            {
                return std::make_shared<HttpResult>((int)HttpResult::Error::CONNECT_FAIL, nullptr, "connect fail for: " + addr->toString());
            }

            sock->setRecvTimeout(timout_ms);
            HttpConnection::ptr conn(new HttpConnection(sock));
            int rt = conn->sendRequest(req);
            if (rt == 0)
            {
                return std::make_shared<HttpResult>((int)HttpResult::Error::SEND_FAIL, nullptr, "connect closed by peer: " + addr->toString());
            }
            else if (rt < 0)
            {
                return std::make_shared<HttpResult>((int)HttpResult::Error::SEND_FAIL, nullptr, "send request fail");
            }

            HttpResponse::ptr rsp = conn->recvResponse();
            if (!rsp)
            {
                return std::make_shared<HttpResult>((int)HttpResult::Error::RECV_FAIL, nullptr, "recv response fail");
            }

            return std::make_shared<HttpResult>((int)HttpResult::Error::OK, rsp, "");
        }

        HttpConnection::HttpConnection(Socket::ptr sock, bool owner)
            : SocketStream(sock, owner)
        {
        }

        // 步骤：接收 HTTP 响应，解析 HTTP 响应，返回 HttpResponse 对象
        HttpResponse::ptr HttpConnection::recvResponse()
        {
            HttpResponseParser::ptr parser(new HttpResponseParser);
            uint64_t buff_size = HttpResponseParser::GetHttpResponseBufferSize();
            std::shared_ptr<char> buffer(new char[buff_size + 1], [](char *ptr)
                                         { delete[] ptr; });
            char *data = buffer.get();
            int offset = 0;

            // 先循环读取并解析响应头，直到头部完整结束。
            do
            {
                // 最大限度度读取数据到 buffer 中，避免溢出。
                int len = read(data + offset, buff_size - offset);
                if (len <= 0)
                {
                    close();
                    return nullptr;
                }
                len += offset; // 累计已读取数据长度
                data[len] = '\0';

                // 调用 HttpResponseParser 解析当前 buffer 中的数据，判断是否完成头部解析。
                size_t nparse = parser->execute(data, len, false);
                if (parser->hasError())
                {
                    close();
                    return nullptr;
                }
                offset = len - nparse;
                if (offset == (int)buff_size)
                {
                    close();
                    return nullptr;
                }
                if (parser->isFinished())
                {
                    break;
                }
            } while (true);
            TIDE_LOG_INFO(g_logger) << "recvResponse: header parsed, offset=" << offset;

            auto &client_parser = parser->getParser();
            // 根据传输方式分别处理响应体：chunked 或固定长度。
            if (client_parser.chunked)
            {
                std::string body;
                int len = offset;

                // chunked 编码需要按块持续读取，直到所有块解析完成。
                do
                {
                    do
                    {
                        int rt = read(data + len, buff_size - len);
                        if (rt <= 0)
                        {
                            close();
                            return nullptr;
                        }
                        len += rt;
                        data[len] = '\0';
                        size_t nparse = parser->execute(data, len, true);
                        if (parser->hasError())
                        {
                            close();
                            return nullptr;
                        }
                        len -= nparse;
                        if (len == (int)buff_size)
                        {
                            close();
                            return nullptr;
                        }
                    } while (!parser->isFinished());
                    len -= 2;
                    if (client_parser.content_len <= len)
                    {
                        body.append(data, client_parser.content_len);
                        memmove(data, data + client_parser.content_len, len - client_parser.content_len);
                        len -= client_parser.content_len;
                    }
                    else
                    {
                        body.append(data, len);
                        int left = client_parser.content_len - len;
                        while (left > 0)
                        {
                            int rt = read(data, left > (int)buff_size ? (int)buff_size : left);
                            if (rt <= 0)
                            {
                                close();
                                return nullptr;
                            }
                            body.append(data, rt);
                            left -= rt;
                        }
                        len = 0;
                    }
                } while (!client_parser.chunks_done);
                parser->getResponse()->setBody(body);
            }
            else
            {
                // 非 chunked 场景下，按 Content-Length 读取剩余响应体。
                int64_t length = parser->getContentLength();
                if (length > 0)
                {
                    std::string body;
                    body.resize(length);

                    int len = 0;
                    if (length >= offset)
                    {
                        memcpy(&body[0], data, offset);
                        len = offset;
                    }
                    else
                    {
                        memcpy(&body[0], data, length);
                        len = length;
                    }
                    length -= offset;
                    if (length > 0)
                    {
                        int rt = readFixSize(&body[len], length);
                        if (rt <= 0)
                        {
                            TIDE_LOG_WARN(g_logger) << "recvResponse: readFixSize failed for remaining body, rt=" << rt << " expected=" << length << " got=" << len;
                            if (len > 0)
                            {
                                body.resize(len);
                                parser->getResponse()->setBody(body);
                                return parser->getResponse();
                            }
                            close();
                            return nullptr;
                        }
                    }
                    parser->getResponse()->setBody(body);
                }
            }

            return parser->getResponse();
        }

        // 将 HttpRequest 序列化为 HTTP 报文并写入 socket。
        int HttpConnection::sendRequest(HttpRequest::ptr req)
        {
            std::stringstream ss;
            ss << *req;
            std::string data = ss.str();
            return writeFixSize(data.c_str(), data.size());
        }

        HttpConnectionPool::HttpConnectionPool(const std::string &host,
                                               const std::string &vhost,
                                               uint32_t port,
                                               uint32_t maxSize,
                                               uint32_t maxAliveTime,
                                               uint32_t maxRequest)
            : m_host(host), m_vhost(vhost), m_port(port), m_maxSize(maxSize), m_maxAliveTime(maxAliveTime), m_maxRequest(maxRequest)
        {
        }

        HttpConnectionPool::ptr HttpConnectionPool::getConnection()
        {
        }

        void HttpConnectionPool::ReleasePtr(HttpConnection *ptr, HttpConnectionPool *pool)
        {
        }

        HttpResult::ptr HttpConnectionPooldoGET(const std::string &url,
                                                uint64_t timeout_ms,
                                                const std::map<std::string, std::string> &headers,
                                                const std::string &body)
        {
        }

        HttpResult::ptr HttpConnectionPool::doGETr(Uri::ptr uri,
                                                   uint64_t timeout_ms,
                                                   const std::map<std::string, std::string> &headers,
                                                   const std::string &body)
        {
            std::stringstream ss;
            ss << uri->getPath() 
            << (uri->getQuery().empty() ? "" : "?" + uri->getQuery())
            << (uri->getFragment().empty() ? "" : "#" + uri->getFragment());
            return doGET(ss.str(), timeout_ms, headers, body);
        }

        HttpResult::ptr HttpConnectionPool::doPOST(const std::string &url,
                                                   uint64_t timeout_ms,
                                                   const std::map<std::string, std::string> &headers,
                                                   const std::string &body)
        {
        }

        HttpResult::ptr HttpConnectionPool::doPOSTr(Uri::ptr uri,
                                                   uint64_t timeout_ms,
                                                   const std::map<std::string, std::string> &headers,
                                                   const std::string &body)
        {
            std::stringstream ss;
            ss << uri->getPath() 
            << (uri->getQuery().empty() ? "" : "?" + uri->getQuery())
            << (uri->getFragment().empty() ? "" : "#" + uri->getFragment());
            return doPOST(ss.str(), timeout_ms, headers, body);
        }

        HttpResult::ptr HttpConnectionPool::DoRequest(HttpMethod method,
                                      const std::string &url,
                                      uint64_t timeout_ms,
                                      const std::map<std::string, std::string> &headers = {},
                                      const std::string &body = "")
        {

        }
        HttpResult::ptr HttpConnectionPool::DoRequestR(HttpMethod method,
                                    Uri::ptr uri,
                                    uint64_t timeout_ms,
                                    const std::map<std::string, std::string> &headers = {},
                                    const std::string &body = "")
        {

        }

        HttpRequest::ptr HttpConnectionPool::DoRequest(HttpRequest::ptr req,
                                    Uri::ptr uri,
                                    uint64_t timeout_ms,
                                    const std::map<std::string, std::string> &headers = {},
                                    const std::string &body = "")
        {

        }


                

    } // namespace http

} // namespace tide

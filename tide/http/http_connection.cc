#include "http_connection.h"

#include "http.h"
#include "http_parser.h"
#include "../log.h"

namespace tide
{
    namespace http
    {
        static Logger::ptr g_logger = TIDE_LOG_NAME("system");

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
                len += offset;  // 累计已读取数据长度
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

            auto& client_parser = parser->getParser();
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
                    if(client_parser.content_len <= len)
                    {
                        body.append(data, client_parser.content_len);
                        memmove(data, data + client_parser.content_len, len - client_parser.content_len);
                        len -= client_parser.content_len;
                    }else{
                        body.append(data, len);
                        int left = client_parser.content_len - len;
                        while(left > 0)
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

    } // namespace http

} // namespace tide
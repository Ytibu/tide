#include "http_session.h"

#include "http.h"
#include "http_parser.h"

namespace tide
{
    namespace http
    {
        HttpSession::HttpSession(Socket::ptr sock, bool owner)
            : SocketStream(sock, owner)
        {
        }

        // 步骤：接收 HTTP 请求，解析 HTTP 请求，返回 HttpRequest 对象
        HttpRequest::ptr HttpSession::recvRequest()
        {
            HttpRequestParser::ptr parser(new HttpRequestParser);
            uint64_t buff_size = HttpRequestParser::GetHttpRequestBufferSize();
            std::shared_ptr<char> buffer(new char[buff_size], [](char *ptr)
                                         { delete[] ptr; });
            char *data = buffer.get();
            int offset = 0;
            do
            {
                int len = read(data + offset, buff_size - offset);
                if (len <= 0)
                {
                    close();
                    return nullptr;
                }
                len += offset;
                size_t nparse = parser->execute(data, len);
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

            int64_t content_length = parser->getContentLength();
            if (content_length > 0)
            {
                std::string body;
                body.resize(content_length);

                if (content_length > offset)
                {
                    body.append(data, offset);
                }
                else
                {
                    body.append(data, content_length);
                }
                content_length -= offset;
                if (content_length > 0)
                {
                    if (readFixSize(&body[offset], content_length) <= 0)
                    {
                        close();
                        return nullptr;
                    }
                }
                parser->getRequest()->setBody(body);
            }

            return parser->getRequest();
        }

        int HttpSession::sendResponse(HttpResponse::ptr rsp)
        {
            std::stringstream ss;
            ss << *rsp;
            std::string data = ss.str();
            return writeFixSize(data.c_str(), data.size());
        }

    } // namespace http

} // namespace tide
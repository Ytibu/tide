#include "../tide/log.h"
#include "../tide/http/http_parser.h"
#include "../tide/http/http.h"


static tide::Logger::ptr g_logger = TIDE_LOG_ROOT();

static char data[] = "POST /index.html HTTP/1.1\r\n"
                     "Host: www.baidu.com\r\n"
                     "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/58.0.3029.110 Safari/537.36\r\n"
                     "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8\r\n"
                     "Accept-Language: zh-CN,zh;q=0.8\r\n"
                     "Accept-Encoding: gzip, deflate, sdch\r\n"
                     "Connection: keep-alive\r\n"
                     "content-length: 11\r\n"
                     "hello world"
                     "\r\n";

void test_uri()
{
    tide::http::HttpRequestParser parser;
    std::string str(data);
    size_t s =  parser.execute(&data[0], str.size());
    TIDE_LOG_INFO(g_logger) << "execute size=" << s << " total=" << str.size()
        << " isFinished=" << parser.isFinished() << " hasError=" << parser.hasError()
        << " content-length=" << parser.getContentLength();

    TIDE_LOG_INFO(g_logger) << parser.getRequest()->toString();
}

static char data2[] = "HTTP/1.1 200 OK\r\n"
                     "Date: Wed, 21 Oct 2015 07:28:00 GMT\r\n"
                     "Server: Apache/2.4.1 (Unix)\r\n"
                     "Content-Type: text/html; charset=UTF-8\r\n"
                     "Content-Length: 11\r\n"
                     "Connection: keep-alive\r\n"
                     "<html>\r\n"
                     "<meta http-equiv=\"refresh\" content=\"0;url=http://www.baidu.com/\">\r\n"
                     "</html>\r\n";

void test_reponse()
{
    std::string str = data2;
    tide::http::HttpResponseParser parser;
    size_t s = parser.execute(&str[0], str.size());

    TIDE_LOG_INFO(g_logger) << "execute size=" << s << " total=" << str.size()
        << " isFinished=" << parser.isFinished() << " hasError=" << parser.hasError()
        << " content-length=" << parser.getContentLength();
    str.resize(str.size() - s);
    TIDE_LOG_INFO(g_logger) << parser.getResponse()->toString();
    TIDE_LOG_INFO(g_logger) << "remain data: " << str;
}
int main()
{
    // test_uri();
    test_reponse();
    return 0;
}
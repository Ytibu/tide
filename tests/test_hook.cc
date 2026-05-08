#include "../tide/tide.h"

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

tide::Logger::ptr g_logger = TIDE_LOG_ROOT();

void test_hook()
{

    tide::IOManager iom(2);

    iom.schedule([]()
                 {
        tide::set_hook_enable(true);
        sleep(2);
        TIDE_LOG_INFO(g_logger) << "test_hook sleep 2"; });

    iom.schedule([]()
                 {
        tide::set_hook_enable(true);
        sleep(3);
        TIDE_LOG_INFO(g_logger) << "test_hook sleep 3"; });

    TIDE_LOG_INFO(g_logger) << "test_hook finished";
}

void test_hook2()
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    addr.sin_addr.s_addr = inet_addr("124.237.177.164");

    int rt = connect(sockfd, (const struct sockaddr *)&addr, sizeof(addr));
    if (rt)
    {
        TIDE_LOG_ERROR(g_logger) << "connect failed rt=" << rt << " errno=" << errno << " errstr=" << strerror(errno);
    }
    const char *msg = "GET / HTTP/1.0\r\n\r\n";
    rt = send(sockfd, msg, strlen(msg), 0);
    TIDE_LOG_INFO(g_logger) << "connect success";

    if(rt <= 0)
    {
        TIDE_LOG_ERROR(g_logger) << "send failed rt=" << rt << " errno=" << errno << " errstr=" << strerror(errno);
    }

    std::string buff;
    buff.resize(4096);
    rt = recv(sockfd, &buff[0], buff.size(), 0);
    if(rt <= 0)
    {
        TIDE_LOG_ERROR(g_logger) << "recv failed rt=" << rt << " errno=" << errno << " errstr=" << strerror(errno);
    }
    buff.resize(rt);
    TIDE_LOG_INFO(g_logger) << "recv success rt=" << rt << " data=\n" << buff;
}

int main()
{
    // test_hook();
    //test_hook2();
    tide::IOManager iom;
    iom.schedule(test_hook2);
    return 0;
}
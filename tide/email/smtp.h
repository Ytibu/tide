#ifndef TIDE_EMAIL_SMTP_H__
#define TIDE_EMAIL_SMTP_H__

#include "../socket_stream.h"
#include "email.h"
#include <sstream>

namespace tide {

struct SmtpResult {
    typedef std::shared_ptr<SmtpResult> ptr;
    enum Result {
        OK = 0,
        IO_ERROR = -1
    };

    /**
     * @brief 构造返回结果对象
     * 
     * @param r 
     * @param m 
     */
    SmtpResult(int r, const std::string& m)
        :result(r)
        ,msg(m) {
    }

    int result;
    std::string msg;
};

class SmtpClient : public tide::SocketStream {
public:
    typedef std::shared_ptr<SmtpClient> ptr;

    /**
     * @brief 创建 SmtpClient 对象并连接到指定的 SMTP 服务器，ssl 参数表示是否使用 SSL/TLS 连接
     * 
     * @param host 
     * @param port 
     * @param ssl 
     * @return SmtpClient::ptr 
     */
    static SmtpClient::ptr Create(const std::string& host, uint32_t port, bool ssl= false);

    /**
     * @brief 发送邮件，timeout_ms 参数表示发送邮件的超时时间，debug 参数表示是否输出调试信息
     * 
     * @param email 
     * @param timeout_ms 
     * @param debug 
     * @return SmtpResult::ptr 
     */
    SmtpResult::ptr send(EMail::ptr email, int64_t timeout_ms = 1000, bool debug = false);

    std::string getDebugInfo();
private:
    SmtpResult::ptr doCmd(const std::string& cmd, bool debug);
private:
    SmtpClient(Socket::ptr sock);
private:
    std::string m_host;
    std::stringstream m_ss;
    bool m_authed = false;
};

}

#endif
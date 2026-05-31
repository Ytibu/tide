#include "smtp.h"
#include "../log.h"
#include "../util/hash_util.h"

#include <string.h>
#include <string>
#include <set>
#include <memory>
#include <list>

namespace tide
{

    static tide::Logger::ptr g_logger = TIDE_LOG_NAME("system");

    SmtpClient::SmtpClient(Socket::ptr sock)
        : tide::SocketStream(sock)
    {
    }

    SmtpClient::ptr SmtpClient::Create(const std::string &host, uint32_t port, bool ssl)
    {
        tide::IPAddress::ptr addr = tide::Address::LookupAnyIPAddress(host);
        if (!addr)
        {
            TIDE_LOG_ERROR(g_logger) << "invalid smtp server: " << host << ":" << port
                                     << " ssl=" << ssl;
            return nullptr;
        }
        addr->setPort(port);
        Socket::ptr sock;
        if (ssl)
        {
            sock = tide::SSLSocket::CreateTCP(addr);
        }
        else
        {
            sock = tide::Socket::CreateTCP(addr);
        }
        if (!sock->connect(addr))
        {
            TIDE_LOG_ERROR(g_logger) << "connect smtp server: " << host << ":" << port
                                     << " ssl=" << ssl << " fail";
            return nullptr;
        }
        std::string buf;
        buf.resize(1024);

        SmtpClient::ptr rt(new SmtpClient(sock));
        int len = rt->read(&buf[0], buf.size());
        if (len <= 0)
        {
            return nullptr;
        }
        buf.resize(len);
        if (tide::TypeUtil::Atoi(buf) != 220)
        {
            return nullptr;
        }
        rt->m_host = host;
        return rt;
    }

    SmtpResult::ptr SmtpClient::doCmd(const std::string &cmd, bool debug)
    {
        if (writeFixSize(cmd.c_str(), cmd.size()) <= 0)
        {
            return std::make_shared<SmtpResult>(SmtpResult::IO_ERROR, "write io error");
        }
        std::string buf;
        buf.resize(4096);
        auto len = read(&buf[0], buf.size());
        if (len <= 0)
        {
            return std::make_shared<SmtpResult>(SmtpResult::IO_ERROR, "read io error");
        }
        buf.resize(len);
        if (debug)
        {
            m_ss << "C: " << cmd;
            m_ss << "S: " << buf;
        }

        int code = tide::TypeUtil::Atoi(buf);
        if (code >= 400)
        {
            // return std::make_shared<SmtpResult>(code, tide::replace( buf.substr(buf.find(' ') + 1), "\r\n", ""));
            std::string msg = buf.substr(buf.find(' ') + 1);
            tide::replace(msg, "\r\n", "");
            return std::make_shared<SmtpResult>(code, msg);
        }
        return nullptr;
    }

    SmtpResult::ptr SmtpClient::send(EMail::ptr email, int64_t timeout_ms, bool debug)
    {
#define DO_CMD()                \
    result = doCmd(cmd, debug); \
    if (result)                 \
    {                           \
        return result;          \
    }

        Socket::ptr sock = getSocket();
        if (sock)
        {
            sock->setRecvTimeout(timeout_ms);
            sock->setSendTimeout(timeout_ms);
        }

        SmtpResult::ptr result;
        std::string cmd = "HELO " + m_host + "\r\n";
        DO_CMD();
        if (!m_authed && !email->getFromEMailAddress().empty())
        {
            cmd = "AUTH LOGIN\r\n";
            DO_CMD();
            auto pos = email->getFromEMailAddress().find('@');
            cmd = tide::base64encode(email->getFromEMailAddress().substr(0, pos)) + "\r\n";
            DO_CMD();
            cmd = tide::base64encode(email->getFromEMailPasswd()) + "\r\n";
            DO_CMD();

            m_authed = true;
        }

        cmd = "MAIL FROM: <" + email->getFromEMailAddress() + ">\r\n";
        DO_CMD();
        std::set<std::string> targets;
#define XX(fun)                  \
    for (auto &i : email->fun()) \
    {                            \
        targets.insert(i);       \
    }
        XX(getToEMailAddress);
        XX(getCcEMailAddress);
        XX(getBccEMailAddress);
#undef XX
        for (auto &i : targets)
        {
            cmd = "RCPT TO: <" + i + ">\r\n";
            DO_CMD();
        }

        cmd = "DATA\r\n";
        DO_CMD();

        auto &entitys = email->getEntitys();

        std::stringstream ss;
        ss << "From: <" << email->getFromEMailAddress() << ">\r\n"
           << "To: ";
#define XX(fun)                               \
    do                                        \
    {                                         \
        auto &v = email->fun();               \
        for (size_t i = 0; i < v.size(); ++i) \
        {                                     \
            if (i)                            \
            {                                 \
                ss << ",";                    \
            }                                 \
            ss << "<" << v[i] << ">";         \
        }                                     \
        if (!v.empty())                       \
        {                                     \
            ss << "\r\n";                     \
        }                                     \
    } while (0);
        XX(getToEMailAddress);
        if (!email->getCcEMailAddress().empty())
        {
            ss << "Cc: ";
            XX(getCcEMailAddress);
        }
        ss << "Subject: " << email->getTitle() << "\r\n";
        std::string boundary;
        if (!entitys.empty())
        {
            boundary = tide::random_string(16);
            ss << "Content-Type: multipart/mixed;boundary="
               << boundary << "\r\n";
        }
        ss << "MIME-Version: 1.0\r\n";
        if (!boundary.empty())
        {
            ss << "\r\n--" << boundary << "\r\n";
        }
        ss << "Content-Type: text/html;charset=\"utf-8\"\r\n"
           << "\r\n"
           << email->getBody() << "\r\n";
        for (auto &i : entitys)
        {
            ss << "\r\n--" << boundary << "\r\n";
            ss << i->toString();
        }
        if (!boundary.empty())
        {
            ss << "\r\n--" << boundary << "--\r\n";
        }
        ss << "\r\n.\r\n";
        cmd = ss.str();
        DO_CMD();
#undef XX
#undef DO_CMD
        return std::make_shared<SmtpResult>(0, "ok");
    }

    std::string SmtpClient::getDebugInfo()
    {
        return m_ss.str();
    }

}
#if !defined(TIDE_HTTP_WS_SERVLET_H__)
#define TIDE_HTTP_WS_SERVLET_H__

#include <memory>

#include "servlet.h"
#include "ws_session.h"

namespace tide
{
    namespace http
    {
        class WSServlet : public Servlet
        {
        public:
            using ptr = std::shared_ptr<WSServlet>;

            WSServlet(const std::string &name) : Servlet(name) {}
            virtual ~WSServlet() {}

            virtual int32_t handle(tide::http::HttpRequest::ptr req, tide::http::HttpResponse::ptr rsp, tide::http::HttpSession::ptr session) override
            {
                return 0;
            }

            virtual int32_t onConnect(tide::http::HttpRequest::ptr req, tide::http::WSSession::ptr session) = 0;
            virtual int32_t onClose(tide::http::HttpRequest::ptr req, tide::http::WSSession::ptr session) = 0;

            virtual int32_t handle(tide::http::HttpRequest::ptr req, tide::http::WSFrameMessage::ptr session, tide::http::WSSession::ptr ws_session) = 0;

            std::string getName() const { return m_name; }

        protected:
            std::string m_name;
        };

        class FunctionWSServlet : public WSServlet
        {
        public:
            using ptr = std::shared_ptr<FunctionWSServlet>;

            using callback = std::function<int32_t(tide::http::HttpRequest::ptr header, tide::http::WSFrameMessage::ptr msg, tide::http::WSSession::ptr session)>;

            using on_connect_cb = std::function<int32_t(tide::http::HttpRequest::ptr req, tide::http::WSSession::ptr session)>;

            using on_close_cb = std::function<int32_t(tide::http::HttpRequest::ptr req, tide::http::WSSession::ptr session)>;

            FunctionWSServlet(callback cb, on_connect_cb connect_cb = nullptr, on_close_cb close_cb = nullptr);

            virtual int32_t handle(tide::http::HttpRequest::ptr header, tide::http::WSFrameMessage::ptr msg, tide::http::WSSession::ptr session) override;

            virtual int32_t onConnect(tide::http::HttpRequest::ptr req, tide::http::WSSession::ptr session) override;

            virtual int32_t onClose(tide::http::HttpRequest::ptr req, tide::http::WSSession::ptr session) override;

        protected:
            callback m_cb;
            on_connect_cb m_connect_cb;
            on_close_cb m_close_cb;
        };

        class WSServletDispatch : public ServletDispatch
        {
        public:
            using ptr = std::shared_ptr<WSServletDispatch>;
            typedef RWMutex RWMutexType;

            WSServletDispatch();
            void addServlet(const std::string &uri, FunctionWSServlet::callback cb, FunctionWSServlet::on_connect_cb connect_cb = nullptr, FunctionWSServlet::on_close_cb close_cb = nullptr);
            void addGlobServlet(const std::string &uri, FunctionWSServlet::callback cb, FunctionWSServlet::on_connect_cb connect_cb = nullptr, FunctionWSServlet::on_close_cb close_cb = nullptr);
            WSServlet::ptr getWSServlet(const std::string &uri);
        };
    }
}

#endif // TIDE_HTTP_WS_SERVLET_H__
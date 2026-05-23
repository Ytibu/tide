#include "ws_servlet.h"

#include "../log.h"
#include "servlet.h"

namespace tide
{
    namespace http
    {
        FunctionWSServlet::FunctionWSServlet(callback cb, on_connect_cb connect_cb, on_close_cb close_cb)
            : WSServlet("FunctionWSServlet"), m_cb(cb), m_connect_cb(connect_cb), m_close_cb(close_cb)
        {
        }

        int32_t FunctionWSServlet::handle(tide::http::HttpRequest::ptr header, tide::http::WSFrameMessage::ptr msg, tide::http::WSSession::ptr session)
        {
            if (m_cb)
            {
                m_cb(header, msg, session);
            }
            return 0;
        }

        int32_t FunctionWSServlet::onConnect(tide::http::HttpRequest::ptr req, tide::http::WSSession::ptr session)
        {
            if (m_connect_cb)
            {
                return m_connect_cb(req, session);
            }
            return 0;
        }

        int32_t FunctionWSServlet::onClose(tide::http::HttpRequest::ptr req, tide::http::WSSession::ptr session)
        {
            if (m_close_cb)
            {
                return m_close_cb(req, session);
            }
            return 0;
        }

        WSServletDispatch::WSServletDispatch()
        {
            m_name = "WSServletDispatch";
        }

        void WSServletDispatch::addServlet(const std::string &uri, FunctionWSServlet::callback cb, FunctionWSServlet::on_connect_cb connect_cb, FunctionWSServlet::on_close_cb close_cb)
        {
            ServletDispatch::addServlet(uri, std::make_shared<FunctionWSServlet>(cb, connect_cb, close_cb));
        }
        void WSServletDispatch::addGlobServlet(const std::string &uri, FunctionWSServlet::callback cb, FunctionWSServlet::on_connect_cb connect_cb, FunctionWSServlet::on_close_cb close_cb)
        {
            ServletDispatch::addGlobServlet(uri, std::make_shared<FunctionWSServlet>(cb, connect_cb, close_cb));
        }
        WSServlet::ptr WSServletDispatch::getWSServlet(const std::string &uri)
        {
            auto slt = getMatchedServlet(uri);
            return std::dynamic_pointer_cast<WSServlet>(slt);
        }
    }
}
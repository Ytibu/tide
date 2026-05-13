#include "servlet.h"

#include "http.h"

#include <fnmatch.h>

namespace tide
{
    namespace http
    {
        Servlet::Servlet(const std::string &name)
        {
        }
        Servlet::~Servlet()
        {
        }

        // function servlet
        FunctionServlet::FunctionServlet(Callback cb)
            : Servlet("FunctionServlet"), m_cb(cb)
        {
        }
        int32_t FunctionServlet::handle(tide::http::HttpRequest::ptr req, tide::http::HttpResponse::ptr rsp, tide::http::HttpSession::ptr session)
        {
            return m_cb ? m_cb(req, rsp, session) : 0;
        }

        // servlet dispatch
        ServletDispatch::ServletDispatch()
            : Servlet("ServletDispatch")
        {
            m_default.reset(new NotFoundServlet());
        }

        int32_t ServletDispatch::handle(tide::http::HttpRequest::ptr req, tide::http::HttpResponse::ptr rsp, tide::http::HttpSession::ptr session)
        {
            auto slt =  getMatchedServlet(req->getPath());
            if(slt){
                slt->handle(req, rsp, session);
            }

            return 0;
        }

        void ServletDispatch::addServlet(const std::string &path, Servlet::ptr slt)
        {
            RWMutexType::WriteLock lock(m_mutex);
            m_datas[path] = slt;
        }
        void ServletDispatch::addServlet(const std::string &path, FunctionServlet::Callback cb)
        {
            RWMutexType::WriteLock lock(m_mutex);
            m_datas[path].reset(new FunctionServlet(cb));
        }
        void ServletDispatch::addGlobServlet(const std::string &path, Servlet::ptr slt)
        {
            RWMutexType::WriteLock lock(m_mutex);
            for (auto it = m_globs.begin(); it != m_globs.end(); ++it)
            {
                if (it->first == path)
                {
                    m_globs.erase(it);
                    break;
                }
            }
            m_globs.push_back(std::make_pair(path, slt));
        }
        void ServletDispatch::addGlobServlet(const std::string &path, FunctionServlet::Callback cb)
        {
            addGlobServlet(path, FunctionServlet::ptr(new FunctionServlet(cb)));
        }

        void ServletDispatch::delServlet(const std::string &path)
        {
            RWMutexType::WriteLock lock(m_mutex);
            m_datas.erase(path);
        }

        void ServletDispatch::delGlobServlet(const std::string &path)
        {
            RWMutexType::WriteLock lock(m_mutex);
            for (auto it = m_globs.begin(); it != m_globs.end(); ++it)
            {
                if (it->first == path)
                {
                    m_globs.erase(it);
                    break;
                }
            }
        }

        // 直接查找，如果找到则返回对应的 Servlet，否则返回 nullptr
        Servlet::ptr ServletDispatch::getServlet(const std::string &path)
        {
            RWMutexType::ReadLock lock(m_mutex);
            auto it = m_datas.find(path);
            return it == m_datas.end() ? nullptr : it->second;
        }

        // 通配符查找，如果找到则返回对应的 Servlet，否则返回 nullptr
        Servlet::ptr ServletDispatch::getGlobServlet(const std::string &path)
        {
            RWMutexType::ReadLock lock(m_mutex);
            for (const auto &pair : m_globs)
            {
                if (pair.first == path)
                {
                    return pair.second;
                }
            }
            return nullptr;
        }

        Servlet::ptr ServletDispatch::getMatchedServlet(const std::string &path)
        {
            RWMutexType::ReadLock lock(m_mutex);
            auto it = m_datas.find(path);
            if (it != m_datas.end())
                return it->second;
            for (const auto &pair : m_globs)
            {
                if (!fnmatch(pair.first.c_str(), path.c_str(), 0))
                    return pair.second;
            }
            return m_default;
        }

        // not found
        NotFoundServlet::NotFoundServlet()
            : Servlet("NotFoundServlet")
        {
        }

        int32_t NotFoundServlet::handle(tide::http::HttpRequest::ptr req, tide::http::HttpResponse::ptr rsp, tide::http::HttpSession::ptr session)
        {
            static const std::string body = "<!DOCTYPE html><head><title>404 Not Found</title> <head> <body> <center><h1> 404 Not Found</h1> </center> <hr> <center> tide/1.11.1</center> </body> </html > ";
            rsp->setStatus(tide::http::HttpStatus::HTTP_STATUS_NOT_FOUND);
            rsp->setHeader("Content-Type", "text/html");
            rsp->setBody(body);
            return 0;
        }

    } // namespace http
} // namespace tide

#include "servlet.h"

#include "http.h"
#include "../log.h"

#include <fnmatch.h>

namespace tide
{
    namespace http
    {
        static tide::Logger::ptr g_logger = TIDE_LOG_NAME("system");

        // Servlet 是一个抽象类，定义了处理 HTTP 请求的接口
        Servlet::Servlet(const std::string &name)
            : m_name(name)
        {
        }

        // 析构函数
        Servlet::~Servlet()
        {
        }

        // 利用回调函数构造 FunctionServlet 对象
        FunctionServlet::FunctionServlet(Callback cb)
            : Servlet("FunctionServlet"), m_cb(cb)
        {
        }

        // 处理HTTP请求，调用回调函数来处理请求
        int32_t FunctionServlet::handle(tide::http::HttpRequest::ptr req, tide::http::HttpResponse::ptr rsp, tide::http::HttpSession::ptr session)
        {
            return m_cb ? m_cb(req, rsp, session) : 0;
        }

        // 构造函数，初始化默认的 Servlet 对象为 NotFoundServlet 对象
        ServletDispatch::ServletDispatch()
            : Servlet("ServletDispatch")
        {
            m_default.reset(new NotFoundServlet());
        }

        // 处理HTTP请求，首先根据请求路径查找是否有匹配的 Servlet，如果有则调用对应的 Servlet 来处理请求，如果没有则调用默认的 Servlet 来处理请求
        int32_t ServletDispatch::handle(tide::http::HttpRequest::ptr req, tide::http::HttpResponse::ptr rsp, tide::http::HttpSession::ptr session)
        {
            auto slt =  getMatchedServlet(req->getPath());
            int32_t rt = 0;
            if(slt){
                rt = slt->handle(req, rsp, session);
            }
            return rt;
        }

        // 添加 Servlet，直接添加到 m_datas 中，key 是请求路径，value 是对应的 Servlet 对象
        void ServletDispatch::addServlet(const std::string &path, Servlet::ptr slt)
        {
            RWMutexType::WriteLock lock(m_mutex);
            m_datas[path] = slt;
        }
        
        // 添加 FunctionServlet，通过回调函数来处理 HTTP 请求
        void ServletDispatch::addServlet(const std::string &path, FunctionServlet::Callback cb)
        {
            RWMutexType::WriteLock lock(m_mutex);
            m_datas[path].reset(new FunctionServlet(cb));
        }

        // 添加通配符 Servlet，添加到 m_globs 中
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

       // 添加通配符 FunctionServlet，通过回调函数来处理 HTTP 请求
        void ServletDispatch::addGlobServlet(const std::string &path, FunctionServlet::Callback cb)
        {
            addGlobServlet(path, FunctionServlet::ptr(new FunctionServlet(cb)));
        }

       // 删除 Servlet，从 m_datas 中删除 key 是 path 的 Servlet 对象
        void ServletDispatch::delServlet(const std::string &path)
        {
            RWMutexType::WriteLock lock(m_mutex);
            m_datas.erase(path);
        }

        // 删除通配符 Servlet，从 m_globs 中删除 key 是 path 的 Servlet 对象
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

       // 查找 Servlet，从 m_datas 中查找 key 是 path 的 Servlet 对象，如果找到则返回对应的 Servlet 对象，如果没有找到则返回 nullptr
        Servlet::ptr ServletDispatch::getServlet(const std::string &path)
        {
            RWMutexType::ReadLock lock(m_mutex);
            auto it = m_datas.find(path);
            return it == m_datas.end() ? nullptr : it->second;
        }

        // 查找通配符 Servlet，从 m_globs 中查找 key 是 path 的 Servlet 对象，如果找到则返回对应的 Servlet 对象，如果没有找到则返回 nullptr
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

       // 查找匹配的 Servlet，先在 m_datas 中查找，如果没有找到则在 m_globs 中查找，如果还没有找到则返回默认的 Servlet 对象
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

        // 利用字符串name构造 Servlet 对象
        NotFoundServlet::NotFoundServlet()
            : Servlet("NotFoundServlet")
        {
        }

        // 处理 HTTP 请求，返回 404 Not Found 响应
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

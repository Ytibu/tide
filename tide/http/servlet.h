#ifndef TIDE_SERVLET_H__
#define TIDE_SERVLET_H__

#include <string>
#include <memory>
#include <functional>
#include <vector>
#include <unordered_map>

#include "http.h"
#include "http_session.h"
#include "../thread.h"

namespace tide
{
    namespace http
    {
        class Servlet{
        public:
            using ptr = std::shared_ptr<Servlet>;

            Servlet(const std::string &name);
            virtual ~Servlet();

            virtual int32_t handle(tide::http::HttpRequest::ptr req, tide::http::HttpResponse::ptr rsp, tide::http::HttpSession::ptr session) = 0;
            virtual std::string getName() const { return m_name; }

        protected:
            std::string m_name;
        };


        class FunctionServlet : public Servlet{
        public:
            using ptr = std::shared_ptr<FunctionServlet>;
            using Callback = std::function<int32_t(tide::http::HttpRequest::ptr req, tide::http::HttpResponse::ptr rsp, tide::http::HttpSession::ptr session)>;

            FunctionServlet(Callback cb);
            virtual int32_t handle(tide::http::HttpRequest::ptr req, tide::http::HttpResponse::ptr rsp, tide::http::HttpSession::ptr session) override;

        private:
            Callback m_cb;
        };

        class ServletDispatch : public Servlet{
        public:
            using ptr = std::shared_ptr<ServletDispatch>;
            using RWMutexType = RWMutex;

            ServletDispatch();
            virtual int32_t handle(tide::http::HttpRequest::ptr req, tide::http::HttpResponse::ptr rsp, tide::http::HttpSession::ptr session) override;
            
            void addServlet(const std::string &path, Servlet::ptr slt);
            void addServlet(const std::string &path, FunctionServlet::Callback cb);
            void addGlobServlet(const std::string &path, Servlet::ptr slt);
            void addGlobServlet(const std::string &path, FunctionServlet::Callback cb);

            void delServlet(const std::string &path);
            void delGlobServlet(const std::string &path);

            Servlet::ptr getServlet(const std::string &path);
            Servlet::ptr getGlobServlet(const std::string &path);

            Servlet::ptr getDefault() const { return m_default; }
            void setDefault(Servlet::ptr v) { m_default = v; }

            Servlet::ptr getMatchedServlet(const std::string &path);

        private:
            RWMutexType m_mutex;
            std::unordered_map<std::string, Servlet::ptr> m_datas;
            std::vector<std::pair<std::string, Servlet::ptr>> m_globs;
            // 默认的 Servlet，当没有匹配到任何 Servlet 时，会使用默认的 Servlet 来处理请求
            Servlet::ptr m_default;
        };

        class NotFoundServlet : public Servlet{
        public:
            using ptr = std::shared_ptr<NotFoundServlet>;

            NotFoundServlet();
            virtual int32_t handle(tide::http::HttpRequest::ptr req, tide::http::HttpResponse::ptr rsp, tide::http::HttpSession::ptr session) override;
        };

    }
}

#endif // TIDE_SERVLET_H__
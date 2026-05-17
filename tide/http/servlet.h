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

        /**
         * @brief servlet层 通过匹配请求路径来分发HTTP请求到对应的Servlet来处理，并支持通过回调函数来处理HTTP请求
         * Servlet是一个抽象类，定义了处理HTTP请求的接口，
         * FunctionServlet是一个简单的Servlet，它通过一个回调函数来处理HTTP请求，
         * ServletDispatch是一个Servlet，它维护了一个Servlet的映射表，根据请求路径来分发HTTP请求到对应的Servlet来处理，
         * NotFoundServlet是一个Servlet，当请求的路径没有匹配到任何Servlet时，会使用NotFoundServlet来处理请求，返回404 Not Found响应
         * 
         */


        // Servlet 是一个抽象类，定义了处理 HTTP 请求的接口
        class Servlet{
        public:
            using ptr = std::shared_ptr<Servlet>;

            /**
             * @brief 利用字符串name构造 Servlet 对象
             * 
             * @param name 
             */
            Servlet(const std::string &name);

            /**
             * @brief 析构函数
             * 
             */
            virtual ~Servlet();

            /**
             * @brief 处理HTTP请求
             * 
             * @param req 
             * @param rsp 
             * @param session 
             * @return int32_t 
             */
            virtual int32_t handle(tide::http::HttpRequest::ptr req, tide::http::HttpResponse::ptr rsp, tide::http::HttpSession::ptr session) = 0;

            /**
             * @brief Get the Name object
             * 
             * @return std::string 
             */
            virtual std::string getName() const { return m_name; }

        protected:
            /**
             * @brief 方便继承的子类构造函数调用，存储 Servlet 的名称
             * 
             */
            std::string m_name;
        };

        // FunctionServlet 是一个简单的 Servlet，它通过一个回调函数来处理 HTTP 请求
        class FunctionServlet : public Servlet{
        public:
            using ptr = std::shared_ptr<FunctionServlet>;

            /**
             * @brief 用于处理 HTTP 请求的回调函数类型，参数是 HTTP 请求对象、HTTP 响应对象和 HTTP 会话对象，返回值是一个整数，表示处理结果
             * 
             */
            using Callback = std::function<int32_t(tide::http::HttpRequest::ptr req, tide::http::HttpResponse::ptr rsp, tide::http::HttpSession::ptr session)>;

            /**
             * @brief 利用回调函数构造 FunctionServlet 对象
             * 
             * @param cb 
             */
            FunctionServlet(Callback cb);

            /**
             * @brief 处理HTTP请求，调用回调函数来处理请求
             * 
             * @param req 
             * @param rsp 
             * @param session 
             * @return int32_t 
             */
            virtual int32_t handle(tide::http::HttpRequest::ptr req, tide::http::HttpResponse::ptr rsp, tide::http::HttpSession::ptr session) override;

        private:
            Callback m_cb;
        };

        // ServletDispatch 是一个 Servlet，它维护了一个 Servlet 的映射表，根据请求路径来分发 HTTP 请求到对应的 Servlet 来处理
        class ServletDispatch : public Servlet{
        public:
            using ptr = std::shared_ptr<ServletDispatch>;
            using RWMutexType = RWMutex;

            /**
             * @brief 构造函数，初始化默认的 Servlet 对象
             * 
             */
            ServletDispatch();

            /**
             * @brief 处理HTTP请求，首先根据请求路径查找是否有匹配的 Servlet，如果有则调用对应的 Servlet 来处理请求，如果没有则调用默认的 Servlet 来处理请求
             * 
             * @param req 
             * @param rsp 
             * @param session 
             * @return int32_t 
             */
            virtual int32_t handle(tide::http::HttpRequest::ptr req, tide::http::HttpResponse::ptr rsp, tide::http::HttpSession::ptr session) override;
            
            /**
             * @brief 添加 Servlet，直接添加到 m_datas 中，key 是请求路径，value 是对应的 Servlet 对象
             * 
             * @param path 
             * @param slt 
             */
            void addServlet(const std::string &path, Servlet::ptr slt);

            /**
             * @brief 添加 FunctionServlet，通过回调函数来处理 HTTP 请求
             * 
             * @param path 
             * @param cb 
             */
            void addServlet(const std::string &path, FunctionServlet::Callback cb);

            /**
             * @brief 添加通配符 Servlet，添加到 m_globs 中
             * 
             * @param path 
             * @param slt 
             */
            void addGlobServlet(const std::string &path, Servlet::ptr slt);

            /**
             * @brief 添加通配符 FunctionServlet，通过回调函数来处理 HTTP 请求
             * 
             * @param path 
             * @param cb 
             */
            void addGlobServlet(const std::string &path, FunctionServlet::Callback cb);

            /**
             * @brief 删除 Servlet，从 m_datas 中删除 key 是 path 的 Servlet 对象
             * 
             * @param path 
             */
            void delServlet(const std::string &path);

            /**
             * @brief 删除通配符 Servlet，从 m_globs 中删除 key 是 path 的 Servlet 对象
             * 
             * @param path 
             */
            void delGlobServlet(const std::string &path);

            /**
             * @brief 查找 Servlet，从 m_datas 中查找 key 是 path 的 Servlet 对象，如果找到则返回对应的 Servlet 对象，如果没有找到则返回 nullptr
             * 
             * @param path 
             * @return Servlet::ptr 
             */
            Servlet::ptr getServlet(const std::string &path);

            /**
             * @brief 查找通配符 Servlet，从 m_globs 中查找 key 是 path 的 Servlet 对象，如果找到则返回对应的 Servlet 对象，如果没有找到则返回 nullptr
             * 
             * @param path 
             * @return Servlet::ptr 
             */
            Servlet::ptr getGlobServlet(const std::string &path);

            /**
             * @brief 获取默认的 Servlet 对象，当没有匹配到任何 Servlet 时，会使用默认的 Servlet 来处理请求
             * 
             * @return Servlet::ptr 
             */
            Servlet::ptr getDefault() const { return m_default; }

            /**
             * @brief 设置默认的 Servlet 对象，当没有匹配到任何 Servlet 时，会使用默认的 Servlet 来处理请求
             * 
             * @param v 
             */
            void setDefault(Servlet::ptr v) { m_default = v; }

            /**
             * @brief 查找匹配的 Servlet，先在 m_datas 中查找，如果没有找到则在 m_globs 中查找，如果还没有找到则返回默认的 Servlet 对象
             * 
             * @param path 
             * @return Servlet::ptr 
             */
            Servlet::ptr getMatchedServlet(const std::string &path);

        private:
            RWMutexType m_mutex;
            // 存储精确匹配的 Servlet
            std::unordered_map<std::string, Servlet::ptr> m_datas;
            // 存储通配符匹配的 Servlet，使用 vector 来存储是因为通配符匹配需要按照添加的顺序来匹配
            std::vector<std::pair<std::string, Servlet::ptr>> m_globs;
            // 默认的 Servlet，当没有匹配到任何 Servlet 时，会使用默认的 Servlet 来处理请求
            Servlet::ptr m_default;
        };

        // NotFoundServlet 是一个 Servlet，当请求的路径没有匹配到任何 Servlet 时，会使用 NotFoundServlet 来处理请求，返回 404 Not Found 响应
        class NotFoundServlet : public Servlet{
        public:
            using ptr = std::shared_ptr<NotFoundServlet>;

            /**
             * @brief 构造函数，初始化 Servlet 的名称为 "NotFoundServlet"
             * 
             */
            NotFoundServlet();

            /**
             * @brief 处理 HTTP 请求，返回 404 Not Found 响应
             * 
             * @param req 
             * @param rsp 
             * @param session 
             * @return int32_t 
             */
            virtual int32_t handle(tide::http::HttpRequest::ptr req, tide::http::HttpResponse::ptr rsp, tide::http::HttpSession::ptr session) override;
        };

    }
}

#endif // TIDE_SERVLET_H__
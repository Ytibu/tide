#ifndef TIDE_APPLICATION_H__
#define TIDE_APPLICATION_H__

#include <vector>

#include "http/http_server.h"

namespace tide
{
    class Application
    {
    public:
        /**
         * @brief 构造函数，初始化成员变量并设置全局实例指针。
         * 
         */
        Application();
        
        /**
         * @brief 获取全局应用程序实例的静态成员函数，返回一个指向 Application 对象的指针。
         * 
         * @return Application* 
         */
        static Application* GetInstance() { return s_Instance; }

        /**
         * @brief 初始化函数，接受命令行参数 argc 和 argv，解析命令行参数并加载到环境变量中，
         * 如果解析错误打印帮助手册。根据用户请求的运行方式进行相应的处理，并检查PID文件是否存在且正在运行，
         * 如果是，则说明服务器已经在运行，打印错误日志并返回false。
         * 最后，根据用户请求的配置文件路径加载配置文件中的配置项到配置管理器中。
         * 
         * @param argc 
         * @param argv 
         * @return true 
         * @return false 
         */
        bool init(int argc, char **argv);

        /**
         * @brief 运行函数，执行应用程序的主逻辑，返回一个布尔值表示运行是否成功。
         * 
         * @return true 
         * @return false 
         */
        bool run();

    private:
        /**
         * @brief 主函数，执行应用程序的主逻辑。
         * 
         * @param argc 
         * @param argv 
         * @return int 
         */
        int main(int argc, char **argv);

        /**
         * @brief 运行纤程函数，执行应用程序的纤程逻辑，返回一个整数表示运行结果。
         * 
         * @return int 
         */
        int run_fiber();
    
    private:
        int m_argc = 0;
        char **m_argv = nullptr;
        std::vector<tide::http::HttpServer::ptr> m_httpservers;
        static Application* s_Instance;
    };
}

#endif // TIDE_APPLICATION_H__
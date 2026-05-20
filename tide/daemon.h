#ifndef TIDE_DAEMON_H__
#define TIDE_DAEMON_H__

#include <unistd.h>
#include <stdint.h>

#include <functional>
#include <string>

#include "singleton.h"

namespace tide
{
    /**
     * @brief 进程信息结构体，包含父进程ID、主进程ID、父进程启动时间、主进程启动时间和重启次数
     * 
     */
    struct ProcessInfo
    {
        pid_t parent_id;
        pid_t main_id;
        uint64_t parent_start_time = 0;
        uint64_t main_start_time = 0;
        uint64_t restart_count = 0;

        std::string toString() const;
    };

    /**
     * @brief 进程信息管理类，使用单例模式，提供全局访问点，可以获取和更新进程信息
     * 
     */
    using ProcessInfoMgr = tide::Singleton<ProcessInfo>;

    /**
     * @brief 可以选择以守护进程的方式启动程序，或者以普通的方式启动程序
     * 
     * @param argc 参数个数
     * @param argv 参数列表
     * @param main_cb 主函数回调，参数与main函数相同，返回值为int
     * @param is_daemon 是否以守护进程的方式启动程序
     * @return int 返回main_cb的返回值
     */
    int start_daemon(int argc, char* argv[], std::function<int(int argc, char* argv[])> main_cb, bool is_daemon);
}


#endif // TIDE_DAEMON_H__
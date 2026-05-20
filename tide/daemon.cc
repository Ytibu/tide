#include "daemon.h"

#include <time.h>
#include <sys/wait.h>
#include <string.h>
#include <unistd.h>

#include <sstream>

#include "log.h"
#include "config.h"

namespace tide
{

    static tide::Logger::ptr g_logger = TIDE_LOG_NAME("daemon");
    static tide::ConfigVar<uint32_t>::ptr g_daemon_restart_interval 
        = tide::Config::Lookup("daemon.restart_interval", (uint32_t)5, "daemon restart interval");

    std::string ProcessInfo::toString() const
    {
        std::stringstream ss;
        ss << "[ parent_id=" << parent_id
           << " main_id=" << main_id
           << " parent_start_time=" << tide::Time2Str(parent_start_time)
           << " main_start_time=" << tide::Time2Str(main_start_time)
           << " restart_count=" << restart_count << " ]";
        return ss.str();
    }

    // 实际的启动函数，根据是否以守护进程的方式启动程序，调用不同的函数
    static int real_start(int argc, char *argv[], std::function<int(int argc, char *argv[])> main_cb)
    {
        return main_cb(argc, argv);
    }

    // 实际的守护进程启动函数，根据不同的操作系统实现不同的守护进程启动方式
    static int real_daemon(int argc, char *argv[], std::function<int(int argc, char *argv[])> main_cb)
    {
        int ret = daemon(1, 0);
        if (ret != 0)
        {
            TIDE_LOG_ERROR(g_logger) << "daemon failed , ret=" << ret
                                     << " errno=" << errno
                                     << " errstr=" << strerror(errno);
            return -1;
        }
        
        ProcessInfoMgr::GetInstance()->parent_id = getpid();
        ProcessInfoMgr::GetInstance()->parent_start_time = time(0);
        while (true)
        {
            pid_t pid = fork();
            if (pid < 0)
            {
                TIDE_LOG_ERROR(g_logger) << "fork failed , pid=" << pid
                                         << " errno=" << errno
                                         << " errstr=" << strerror(errno);
                return -1;
            }
            else if (pid == 0)
            {
                // 子进程，执行主函数回调
                ProcessInfoMgr::GetInstance()->main_id = getpid();
                ProcessInfoMgr::GetInstance()->main_start_time = time(0);
                TIDE_LOG_INFO(g_logger) << "process start, pid=" << ProcessInfoMgr::GetInstance()->main_id;
                return main_cb(argc, argv);
            }
            else
            {
                // 父进程，等待子进程退出
                int status = 0;
                waitpid(pid, &status, 0);
                if (status)
                {
                    TIDE_LOG_ERROR(g_logger) << "process exit, pid=" << pid << ", code=" << status;
                }
                else
                {
                    TIDE_LOG_INFO(g_logger) << "process exit, pid=" << pid << " is normal";
                    break;
                }
                ProcessInfoMgr::GetInstance()->restart_count++;
                sleep(g_daemon_restart_interval->getValue());
            }
        }

        return 0;
    }

    // 根据是否以守护进程的方式启动程序，调用不同的函数
    int start_daemon(int argc, char *argv[], std::function<int(int argc, char *argv[])> main_cb, bool is_daemon)
    {
        if (is_daemon)
        {
            return real_daemon(argc, argv, main_cb);
        }
        return real_start(argc, argv, main_cb);
    }
}
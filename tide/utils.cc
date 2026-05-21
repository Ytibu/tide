#include "utils.h"

#include <cstring>
#include <sstream>

#include <unistd.h>
#include <sys/syscall.h>
#include <stdint.h>
#include <execinfo.h>
#include <sys/time.h>
#include <dirent.h>
#include <sys/stat.h>
#include <signal.h>

#include "tide.h"

namespace tide
{
    tide::Logger::ptr g_logger = TIDE_LOG_NAME("system");

    pid_t GetThreadId()
    {
        return syscall(SYS_gettid);
    }

    uint32_t GetFiberId()
    {
        return tide::Fiber::GetFiberId();
    }

    void Backtrace(std::vector<std::string> &bt, int size, int skip)
    {
        void **array = (void **)malloc(sizeof(void *) * size);
        size_t s = ::backtrace(array, size);
        char **strings = ::backtrace_symbols(array, s);
        if (strings == NULL)
        {
            TIDE_LOG_ERROR(g_logger) << "backtrace_symbols error";
            free(strings);
            return;
        }

        for (size_t i = skip; i < s; ++i)
        {
            bt.push_back(strings[i]);
        }

        free(strings);
        free(array);
    }

    std::string BacktraceToString(int size, int skip, const std::string &prefix)
    {
        std::vector<std::string> bt;
        Backtrace(bt, size, skip);
        std::stringstream ss;
        for (size_t i = 0; i < bt.size(); ++i)
        {
            ss << prefix << bt[i] << std::endl;
        }
        return ss.str();
    }

    uint64_t GetCurrentMS()
    {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        return tv.tv_sec * 1000ul + tv.tv_usec / 1000;
    }

    uint64_t GetCurrentUS()
    {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        return tv.tv_sec * 1000 * 1000ul + tv.tv_usec;
    }

    std::string Time2Str(time_t ts, const std::string &format)
    {
        struct tm tm;
        localtime_r(&ts, &tm);
        char buf[64];
        strftime(buf, sizeof(buf), format.c_str(), &tm);
        return buf;
    }

    void FSUtil::ListAllFile(std::vector<std::string> &files, const std::string &path, const std::string &subfix)
    {
        // 判断路径是否可读
        if (access(path.c_str(), R_OK) != 0)
        {
            return;
        }

        // 打开目录
        DIR *dir = opendir(path.c_str());
        if (dir == nullptr)
        {
            return;
        }

        // 循环读取目录项
        struct dirent *dp = nullptr;
        while ((dp = readdir(dir)) != nullptr)
        {
            // 判断目录项类型为目录
            if (dp->d_type == DT_DIR)
            {
                // 跳过当前目录和父目录
                if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0)
                {
                    continue;
                }
                ListAllFile(files, path + "/" + dp->d_name, subfix);
            }

            // 判断目录项类型为普通文件
            else if (dp->d_type == DT_REG)
            {
                std::string filename(dp->d_name);
                if (subfix.empty())
                {
                    files.push_back(path + "/" + filename);
                }
                else
                {
                    if (filename.size() < subfix.size())
                    {
                        continue;
                    }
                    if (filename.substr(filename.size() - subfix.size()) == subfix)
                    {
                        files.push_back(path + "/" + filename);
                    }
                }
            }
            else if (dp->d_type == DT_UNKNOWN)
            {
                struct stat st;
                if (stat((path + "/" + dp->d_name).c_str(), &st) != 0)
                {
                    continue;
                }
                if (S_ISDIR(st.st_mode))
                {
                    // 跳过当前目录和父目录
                    if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0)
                    {
                        continue;
                    }
                    ListAllFile(files, path + "/" + dp->d_name, subfix);
                }
                else if (S_ISREG(st.st_mode))
                {
                    std::string filename(dp->d_name);
                    if (subfix.empty())
                    {
                        files.push_back(path + "/" + filename);
                    }
                    else
                    {
                        if (filename.size() < subfix.size())
                        {
                            continue;
                        }
                        if (filename.substr(filename.size() - subfix.size()) == subfix)
                        {
                            files.push_back(path + "/" + filename);
                        }
                    }
                }
            }
        }

        // 关闭目录
        closedir(dir);
    }

    static int __lstat(const char *file, struct stat *st = nullptr)
    {
        struct stat lst;
        int ret = lstat(file, &lst);
        if (st)
        {
            *st = lst;
        }
        return ret;
    }

    static int __mkdir(const char *dirname)
    {
        if (access(dirname, F_OK) == 0)
        {
            return 0;
        }
        return mkdir(dirname, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    }

    bool FSUtil::Mkdir(const std::string &dirname)
    {
        if (__lstat(dirname.c_str()) == 0)
        {
            return true;
        }
        char *path = strdup(dirname.c_str());
        char *ptr = strchr(path + 1, '/');
        do
        {
            for (; ptr; *ptr = '/', ptr = strchr(ptr + 1, '/'))
            {
                *ptr = '\0';
                if (__mkdir(path) != 0)
                {
                    break;
                }
            }
            if (ptr != nullptr)
            {
                break;
            }
            else if (__mkdir(path) != 0)
            {
                break;
            }
            free(path);
            return true;
        } while (0);
        free(path);
        return false;
    }

    bool FSUtil::IsRunningPidfile(const std::string &pidfile)
    {
        if (__lstat(pidfile.c_str()) != 0)
        {
            return false;
        }
        std::ifstream ifs(pidfile);
        std::string line;
        if (!ifs || !std::getline(ifs, line))
        {
            return false;
        }
        if (line.empty())
        {
            return false;
        }
        pid_t pid = atoi(line.c_str());
        if (pid <= 1)
        {
            return false;
        }
        if (kill(pid, 0) != 0)
        {
            return false;
        }
        return true;
    }

} // namespace tide
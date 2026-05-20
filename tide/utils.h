#ifndef TIDE_UTILS_H__
#define TIDE_UTILS_H__

#include <vector>
#include <string>
#include <cstdint>
#include <time.h>

#include <sys/types.h>

namespace tide
{
    pid_t GetThreadId();

    uint32_t GetFiberId();

    void Backtrace(std::vector<std::string>& bt, int size, int skip = 1);

    std::string BacktraceToString(int size = 1, int skip = 2, const std::string& prefix = "");

    // 时间获取
    uint64_t GetCurrentMS();
    uint64_t GetCurrentUS();

    std::string Time2Str(time_t ts = time(0), const std::string& format = "%Y-%m-%d %H:%M:%S");

    class FSUtil
    {
    public:
        /**
         * @brief 获取指定路径下的所有文件，递归获取子目录下的文件
         * 
         * @param files 存储文件路径的vector
         * @param path 需要获取文件的路径
         * @param subfix 文件后缀，默认为空字符串，表示获取所有文件
         */
        static void ListAllFile(std::vector<std::string>& files, const std::string& path, const std::string& subfix);

        static bool IsRunningPidfile(const std::string& pidfile);

        static bool Mkdir(const std::string& path);
    };

} // namespace tide

#endif // __UTILS_H__
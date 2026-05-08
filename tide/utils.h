#ifndef TIDE_UTILS_H__
#define TIDE_UTILS_H__

#include <vector>
#include <string>
#include <cstdint>

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

} // namespace tide

#endif // __UTILS_H__
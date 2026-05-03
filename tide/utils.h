#ifndef __UTILS_H__
#define __UTILS_H__

#include <vector>
#include <string>
#include <cstdint>

#include <sys/types.h>

namespace tide
{
    pid_t GetThreadId();

    uint32_t GetFiberId();

    void Backtrace(std::vector<std::string>& bt, int size, int skip = 1);

    std::string BacktraceToString(int size, int skip = 2, const std::string& prefix = "");

} // namespace tide

#endif // __UTILS_H__
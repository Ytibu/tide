#ifndef __UTILS_H__
#define __UTILS_H__


#include <sys/types.h>
#include <cstdint>

namespace tide
{
    pid_t GetThreadId();

    uint32_t GetFiberId();
} // namespace tide

#endif // __UTILS_H__
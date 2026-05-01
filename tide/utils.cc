#include "utils.h"

#include <unistd.h>
#include <sys/syscall.h>
#include <stdint.h>
#include <cstring>

namespace tide
{
    pid_t GetThreadId()
    {
        return syscall(SYS_gettid);
    }

    uint32_t GetFiberId()
    {
        return 1;
    }
} // namespace tide
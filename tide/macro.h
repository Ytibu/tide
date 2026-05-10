#ifndef __MACRO_H__
#define __MACRO_H__

#include <string.h>
#include <assert.h>
#include "tide.h"

namespace tide{

#if defined __GNUC__ || defined __llvm__
#   define TIDE_LIKELY(x)      __builtin_expect(!!(x), 1)
#   define TIDE_UNLIKELY(x)     __builtin_expect(!!(x), 0)
#else
#   define TIDE_LIKELY(x)       (x)
#   define TIDE_UNLIKELY(x)         (x)
#endif

#define TIDE_ASSERT(x) \
    if(TIDE_UNLIKELY(!(x))){ \
        TIDE_LOG_ERROR(TIDE_LOG_ROOT()) << "ASSERTION: " #x \
            << "\nbacktrace:\n" \
            << tide::BacktraceToString(100, 2, " "); \
            assert(x); \
    }

#define TIDE_ASSERT2(x, w) \
    if(TIDE_UNLIKELY(!(x))){ \
        TIDE_LOG_ERROR(TIDE_LOG_ROOT()) << "ASSERTION: " #x \
            << "\n" << w \
            << "\nbacktrace:\n" \
            << tide::BacktraceToString(100, 2, " "); \
            assert(x); \
    }

};

#endif // __MACRO_H__
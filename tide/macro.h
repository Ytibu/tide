#ifndef __MACRO_H__
#define __MACRO_H__

#include <string.h>
#include <assert.h>
#include "tide.h"

namespace tide{
    
#define TIDE_ASSERT(x) \
    if(!(x)){ \
        TIDE_LOG_ERROR(TIDE_LOG_ROOT()) << "ASSERTION: " #x \
            << "\nbacktrace:\n" \
            << tide::BacktraceToString(100, 2, " "); \
            assert(x); \
    }

#define TIDE_ASSERT2(x, w) \
    if(!(x)){ \
        TIDE_LOG_ERROR(TIDE_LOG_ROOT()) << "ASSERTION: " #x \
            << "\n" << w \
            << "\nbacktrace:\n" \
            << tide::BacktraceToString(100, 2, " "); \
            assert(x); \
    }

};

#endif // __MACRO_H__
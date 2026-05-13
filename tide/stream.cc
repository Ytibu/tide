#include "stream.h"

namespace tide
{

    int Stream::readFixSize(void *buffer, size_t length)
    {
        size_t left = length;
        char *buf = (char *)buffer;
        while (left > 0)
        {
            int len = read(buf, left);
            if (len <= 0)
            {
                return len;
            }
            left -= len;
            buf += len;
        }
        return length - left;
    }
    int Stream::readFixSize(ByteArray::ptr ba, size_t length)
    {
        size_t left = length;
        while (left > 0)
        {
            int len = read(ba, left);
            if (len <= 0)
            {
                return len;
            }
            left -= len;
        }
        return length - left;
    }
    int Stream::writeFixSize(const void *buffer, size_t length)
    {
        size_t left = length;
        const char *buf = (const char *)buffer;
        while (left > 0)
        {
            int len = write(buf, left);
            if (len <= 0)
            {
                return len;
            }
            left -= len;
            buf += len;
        }
        return length - left;
    }
    int Stream::writeFixSize(ByteArray::ptr ba, size_t length)
    {
        size_t left = length;
        while (left > 0)
        {
            int len = write(ba, left);
            if (len <= 0)
            {
                return len;
            }
            left -= len;
        }
        return length - left;
    }

}
#ifndef TIDE_ENDIAN_H__
#define TIDE_ENDIAN_H__

#include <type_traits>

#include <byteswap.h>
#include <stdint.h>

namespace tide
{
#define TIDE_LITTLE_ENDIAN 1
#define TIDE_BIG_ENDIAN 2

    template<class T>
    typename std::enable_if<sizeof(T) == sizeof(uint64_t), T>::type
    byteswap(T value)
    {
        return (T)bswap_64((uint64_t)value);
    }

    template<class T>
    typename std::enable_if<sizeof(T) == sizeof(uint32_t), T>::type
    byteswap(T value)
    {
        return (T)bswap_32((uint32_t)value);
    }

    template<class T>
    typename std::enable_if<sizeof(T) == sizeof(uint16_t), T>::type
    byteswap(T value)
    {
        return (T)bswap_16((uint16_t)value);
    }
#if __BYTE_ORDER == __BIG_ENDIAN
    #define TIDE_BYTE_ORDER TIDE_BIG_ENDIAN
#elif __BYTE_ORDER == __LITTLE_ENDIAN
    #define TIDE_BYTE_ORDER TIDE_LITTLE_ENDIAN
#else
    #error "Unknown byte order"
#endif

#if TIDE_BYTE_ORDER == TIDE_BIG_ENDIAN
    template<class T>
    T byteswapOnLittleEndian(T value)
    {
        return byteswap(value);
    }

    template<class T>
    T byteswapOnBigEndian(T value)
    {
        return value;
    }

#else
    
    template<class T>
    T byteswapOnLittleEndian(T value)
    {
        return value;
    }

    template<class T>
    T byteswapOnBigEndian(T value)
    {
        return byteswap(value);
    }
#endif

}

#endif  // TIDE_ENDIAN_H__
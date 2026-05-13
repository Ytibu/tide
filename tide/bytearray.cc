#include "bytearray.h"

#include <string>
#include <stdexcept>
#include <fstream>
#include <iomanip>

#include <string.h>

#include "endian.h"
#include "log.h"

namespace tide
{
    static tide::Logger::ptr g_logger = TIDE_LOG_NAME("system");

    ByteArray::Node::Node(size_t s) : m_next(nullptr), m_ptr(new char[s]), m_size(s)
    {
    }

    ByteArray::Node::Node() : m_next(nullptr), m_ptr(nullptr), m_size(0)
    {
    }

    ByteArray::Node::~Node()
    {
        if (m_ptr)
        {
            delete[] m_ptr;
        }
    }

    ByteArray::ByteArray(size_t base_size)
        : m_root(new Node(base_size)), m_cur(m_root), m_endian(TIDE_BIG_ENDIAN), m_baseSize(base_size), m_position(0), m_capacity(base_size), m_size(0)
    {
    }

    ByteArray::~ByteArray()
    {
        Node *tmp = m_root;
        while (tmp)
        {
            Node *next = tmp->m_next;
            delete tmp;
            tmp = next;
        }
    }

    void ByteArray::writeFint8(int8_t value)
    {
        write(&value, sizeof(value));
    }
    void ByteArray::writeFuint8(uint8_t value)
    {
        write(&value, sizeof(value));
    }
    void ByteArray::writeFint16(int16_t value)
    {
        if (m_endian != TIDE_BYTE_ORDER)
        {
            value = byteswap(value);
        }
        write(&value, sizeof(value));
    }
    void ByteArray::writeFuint16(uint16_t value)
    {
        if (m_endian != TIDE_BYTE_ORDER)
        {
            value = byteswap(value);
        }
        write(&value, sizeof(value));
    }
    void ByteArray::writeFint32(int32_t value)
    {
        if (m_endian != TIDE_BYTE_ORDER)
        {
            value = byteswap(value);
        }
        write(&value, sizeof(value));
    }
    void ByteArray::writeFuint32(uint32_t value)
    {
        if (m_endian != TIDE_BYTE_ORDER)
        {
            value = byteswap(value);
        }
        write(&value, sizeof(value));
    }
    void ByteArray::writeFint64(int64_t value)
    {
        if (m_endian != TIDE_BYTE_ORDER)
        {
            value = byteswap(value);
        }
        write(&value, sizeof(value));
    }
    void ByteArray::writeFuint64(uint64_t value)
    {
        if (m_endian != TIDE_BYTE_ORDER)
        {
            value = byteswap(value);
        }
        write(&value, sizeof(value));
    }

    static uint32_t EncodeZigzag32(const int32_t v)
    {
        if (v < 0)
        {
            return ((uint32_t)(-v)) * 2 - 1;
        }
        else
        {
            return v * 2;
        }
    }

    static uint64_t EncodeZigzag64(const int64_t v)
    {
        if (v < 0)
        {
            return ((uint64_t)(-v)) * 2 - 1;
        }
        else
        {
            return v * 2;
        }
    }

    static int32_t DecodeZigzag32(const uint32_t v)
    {
        return (v >> 1) ^ -(v & 1);
    }

    static int64_t DecodeZigzag64(const uint64_t v)
    {
        return (v >> 1) ^ -(v & 1);
    }

    void ByteArray::writeInt32(int32_t value)
    {
        writeUint32(EncodeZigzag32(value));
    }
    void ByteArray::writeUint32(uint32_t value)
    {
        uint8_t tmp[5];
        uint8_t i = 0;
        while (value >= 0x80)
        {
            tmp[i++] = (uint8_t)(value & 0x7F) | 0x80;
            value >>= 7;
        }
        tmp[i] = (uint8_t)value;
        write(tmp, i + 1);
    }

    void ByteArray::writeInt64(int64_t value)
    {
        writeUint64(EncodeZigzag64(value));
    }
    void ByteArray::writeUint64(uint64_t value)
    {
        uint8_t tmp[10];
        uint8_t i = 0;
        while (value >= 0x80)
        {
            tmp[i++] = (uint8_t)(value & 0x7F) | 0x80;
            value >>= 7;
        }
        tmp[i] = (uint8_t)value;
        write(tmp, ++i);
    }

    void ByteArray::writeFloat(float value)
    {
        uint32_t v;
        memcpy(&v, &value, sizeof(value));
        writeFuint32(v);
    }
    void ByteArray::writeDouble(double value)
    {
        uint64_t v;
        memcpy(&v, &value, sizeof(value));
        writeFuint64(v);
    }

    void ByteArray::writeStringF16(const std::string &value)
    {
        writeFuint16(value.size());
        write(value.c_str(), value.size());
    }
    void ByteArray::writeStringF32(const std::string &value)
    {
        writeFuint32(value.size());
        write(value.c_str(), value.size());
    }
    void ByteArray::writeStringF64(const std::string &value)
    {
        writeFuint64(value.size());
        write(value.c_str(), value.size());
    }
    void ByteArray::writeStringVint(const std::string &value)
    {
        writeUint64(value.size());
        write(value.c_str(), value.size());
    }

    void ByteArray::writeStringWithoutLength(const std::string &value)
    {
        write(value.c_str(), value.size());
    }

    // read
    int8_t ByteArray::readFint8()
    {
        int8_t v;
        read(&v, sizeof(v));
        return v;
    }
    uint8_t ByteArray::readFuint8()
    {
        uint8_t v;
        read(&v, sizeof(v));
        return v;
    }
    int16_t ByteArray::readFint16()
    {
        int16_t v;
        read(&v, sizeof(v));

        return (m_endian == TIDE_BYTE_ORDER) ? v : byteswap(v);
    }
    uint16_t ByteArray::readFuint16()
    {
        uint16_t v;
        read(&v, sizeof(v));
        return (m_endian == TIDE_BYTE_ORDER) ? v : byteswap(v);
    }
    int32_t ByteArray::readFint32()
    {
        int32_t v;
        read(&v, sizeof(v));
        return (m_endian == TIDE_BYTE_ORDER) ? v : byteswap(v);
    }
    uint32_t ByteArray::readFuint32()
    {
        uint32_t v;
        read(&v, sizeof(v));
        return (m_endian == TIDE_BYTE_ORDER) ? v : byteswap(v);
    }
    int64_t ByteArray::readFint64()
    {
        int64_t v;
        read(&v, sizeof(v));
        return (m_endian == TIDE_BYTE_ORDER) ? v : byteswap(v);
    }
    uint64_t ByteArray::readFuint64()
    {
        uint64_t v;
        read(&v, sizeof(v));
        return (m_endian == TIDE_BYTE_ORDER) ? v : byteswap(v);
    }

    int32_t ByteArray::readInt32()
    {
        return DecodeZigzag32(readUint32());
    }
    uint32_t ByteArray::readUint32()
    {
        uint32_t result = 0;
        for (int i = 0; i < 32; i += 7)
        {
            uint8_t b = readFuint8();
            if (b < 0x80)
            {
                result |= ((uint32_t)b) << i;
                break;
            }
            else
            {
                result |= ((uint32_t)(b & 0x7F)) << i;
            }
        }
        return result;
    }
    int64_t ByteArray::readInt64()
    {
        return DecodeZigzag64(readUint64());
    }
    uint64_t ByteArray::readUint64()
    {
        uint64_t result = 0;
        for (int i = 0; i < 64; i += 7)
        {
            uint8_t b = readFuint8();
            if (b < 0x80)
            {
                result |= ((uint64_t)b) << i;
                break;
            }
            else
            {
                result |= ((uint64_t)(b & 0x7F)) << i;
            }
        }
        return result;
    }

    float ByteArray::readFloat()
    {
        uint32_t v = readFuint32();
        float value;
        memcpy(&value, &v, sizeof(value));
        return value;
    }
    double ByteArray::readDouble()
    {
        uint64_t v = readFuint64();
        double value;
        memcpy(&value, &v, sizeof(value));
        return value;
    }

    std::string ByteArray::readStringF16()
    {
        uint16_t len = readFuint16();
        std::string result(len, '\0');
        read(&result[0], len);
        return result;
    }
    std::string ByteArray::readStringF32()
    {
        uint32_t len = readFuint32();
        std::string result(len, '\0');
        read(&result[0], len);
        return result;
    }
    std::string ByteArray::readStringF64()
    {
        uint64_t len = readFuint64();
        std::string result(len, '\0');
        read(&result[0], len);
        return result;
    }
    std::string ByteArray::readStringVint()
    {
        uint64_t len = readUint64();
        std::string result(len, '\0');
        read(&result[0], len);
        return result;
    }

    void ByteArray::clear()
    {
        m_position = m_size = 0;
        m_capacity = m_baseSize;
        Node *tmp = m_root->m_next;
        while (tmp)
        {
            m_cur = m_root;
            tmp = tmp->m_next;
            delete m_cur;
        }
        m_cur = m_root;
        m_root->m_next = nullptr;
    }

    // 数据读写接口
    void ByteArray::write(const void *buf, size_t size)
    {
        if (size == 0)
            return;
        addCapacity(size);

        size_t npos = m_position % m_baseSize;
        size_t ncap = m_cur->m_size - npos;
        size_t bpos = 0;

        while (size > 0)
        {
            if (ncap >= size)
            {
                memcpy(m_cur->m_ptr + npos, (const char *)buf + bpos, size);
                m_position += size;
                bpos += size;
                size = 0;
            }
            else
            {
                memcpy(m_cur->m_ptr + npos, (const char *)buf + bpos, ncap);
                m_position += ncap;
                bpos += ncap;
                size -= ncap;
                m_cur = m_cur->m_next;
                npos = 0;
                ncap = m_cur->m_size;
            }
        }

        if (m_position > m_size)
        {
            m_size = m_position;
        }
    }

    void ByteArray::read(void *buf, size_t size)
    {
        if (size > getReadSize())
        {
            throw std::out_of_range("not enough data to read");
        }

        size_t npos = m_position % m_baseSize;
        size_t ncap = m_cur->m_size - npos;
        size_t bpos = 0;
        while (size > 0)
        {
            if (ncap >= size)
            {
                memcpy((char *)buf + bpos, m_cur->m_ptr + npos, size);
                if (m_cur->m_size == npos + size)
                {
                    m_cur = m_cur->m_next;
                }
                m_position += size;
                bpos += size;
                size = 0;
            }
            else
            {
                memcpy((char *)buf + bpos, m_cur->m_ptr + npos, ncap);
                m_position += ncap;
                bpos += ncap;
                size -= ncap;
                m_cur = m_cur->m_next;
                npos = 0;
                ncap = m_cur->m_size;
            }
        }
    }

    void ByteArray::read(void *buf, size_t size, size_t position) const
    {
        if (size > getReadSize())
        {
            throw std::out_of_range("not enough data to read");
        }

        size_t npos = position % m_baseSize;
        size_t ncap = m_cur->m_size - npos;
        size_t bpos = 0;
        Node *cur = m_cur;
        while (size > 0)
        {
            if (ncap >= size)
            {
                memcpy((char *)buf + bpos, cur->m_ptr + npos, size);
                if (cur->m_size == (npos + size))
                {
                    cur = cur->m_next;
                }
                position += size;
                bpos += size;
                size = 0;
            }
            else
            {
                memcpy((char *)buf + bpos, cur->m_ptr + npos, ncap);
                position += ncap;
                bpos += ncap;
                size -= ncap;
                cur = cur->m_next;
                ncap = cur->m_size;
                npos = 0;
            }
        }
    }
    // 位置相关
    void ByteArray::setPosition(size_t pos)
    {
        if (pos > m_capacity)
        {
            throw std::out_of_range("position out of range");
        }

        m_position = pos;
        if(m_position > m_size)
        {
            m_size = m_position;
        }
        m_cur = m_root;
        while (pos > m_cur->m_size)
        {
            pos -= m_cur->m_size;
            m_cur = m_cur->m_next;
        }
    }

    void ByteArray::setLittleEndian(bool val)
    {
        m_endian = val ? TIDE_LITTLE_ENDIAN : TIDE_BIG_ENDIAN;
    }
    bool ByteArray::isLittleEndian() const
    {
        return m_endian == TIDE_LITTLE_ENDIAN;
    }

    std::string ByteArray::toString() const
    {
        std::string str;
        str.resize(getReadSize());
        if (str.empty())
        {
            return str;
        }
        read(&str[0], str.size(), m_position);
        return str;
    }
    std::string ByteArray::toHexString() const
    {
        std::string str = toString();
        std::stringstream ss;

        for (size_t i = 0; i < str.size(); ++i)
        {
            if (i != 0 && i % 16 == 0)
            {
                ss << std::endl;
            }
            ss << std::setw(2) << std::setfill('0') << std::hex << (int)(uint8_t)str[i] << " ";
        }
        return ss.str();
    }

    uint64_t ByteArray::getReadBufferSize(std::vector<iovec> &buffers, uint64_t len)
    {
        len = len > getReadSize() ? getReadSize() : len;
        if (len == 0)
            return 0;
        uint64_t size = len;

        size_t npos = m_position % m_baseSize;
        size_t ncap = m_cur->m_size - npos;
        struct iovec iov;
        Node *cur = m_cur;

        while (len > 0)
        {
            if (ncap >= size)
            {
                iov.iov_base = cur->m_ptr + npos;
                iov.iov_len = len;
                len = 0;
            }
            else
            {
                iov.iov_base = cur->m_ptr + npos;
                iov.iov_len = ncap;
                len -= ncap;
                cur = cur->m_next;
                npos = 0;
                ncap = cur->m_size;
            }
            buffers.push_back(iov);
        }
        return size;
    }
    uint64_t ByteArray::getReadBufferSize(std::vector<iovec> &buffers, uint64_t len, uint64_t position) const
    {
        len = len > getReadSize() ? getReadSize() : len;
        if (len == 0)
            return 0;
        uint64_t size = len;

        size_t npos = position % m_baseSize;
        size_t count = position / m_baseSize;
        Node *cur = m_root;
        while (count > 0)
        {
            cur = cur->m_next;
            --count;
        }

        size_t ncap = cur->m_size - npos;
        struct iovec iov;
        while (len > 0)
        {
            if (ncap >= len)
            {
                iov.iov_base = cur->m_ptr + npos;
                iov.iov_len = len;
                len = 0;
            }
            else
            {
                iov.iov_base = cur->m_ptr + npos;
                iov.iov_len = ncap;
                len -= ncap;
                cur = cur->m_next;
                npos = 0;
                ncap = cur->m_size;
            }
            buffers.push_back(iov);
        }
        return size;
    }
    uint64_t ByteArray::getWriteBuffers(std::vector<iovec> &buffers, uint64_t len)
    {
        if (len == 0)
            return 0;
        addCapacity(len);
        uint64_t size = len;

        size_t npos = m_position % m_baseSize;
        size_t ncap = m_cur->m_size - npos;
        struct iovec iov;
        Node *cur = m_cur;

        while (len > 0)
        {
            if (ncap >= len)
            {
                iov.iov_base = cur->m_ptr + npos;
                iov.iov_len = len;
                len = 0;
            }
            else
            {
                iov.iov_base = cur->m_ptr + npos;
                iov.iov_len = ncap;
                len -= ncap;
                cur = cur->m_next;
                npos = 0;
                ncap = cur->m_size;
            }
            buffers.push_back(iov);
        }
        return size;
    }

    // 数据读取或写入文件，方便错误展示与调试
    bool ByteArray::writeToFile(const std::string &name) const
    {
        std::ofstream ofs(name, std::ios::binary | std::ios::trunc);
        if (!ofs)
        {
            TIDE_LOG_ERROR(g_logger) << "open file " << name << " failed";
            return false;
        }

        int64_t readSize = getReadSize();
        int64_t pos = m_position;
        Node *cur = m_cur;
        while (readSize > 0)
        {
            size_t diff = pos % m_baseSize;
            int64_t cap = (int64_t)(cur->m_size - diff);
            int64_t len = readSize < cap ? readSize : cap;
            ofs.write(cur->m_ptr + diff, len);
            cur = cur->m_next;
            pos += len;
            readSize -= len;
        }

        return true;
    }
    bool ByteArray::readFromFile(const std::string &name)
    {
        std::ifstream ifs(name, std::ios::binary);
        if (!ifs)
        {
            TIDE_LOG_ERROR(g_logger) << "open file " << name << " failed";
            return false;
        }

        std::shared_ptr<char> buf(new char[m_baseSize], [](char *ptr)
                                  { delete[] ptr; });
        while (ifs)
        {
            ifs.read(buf.get(), m_baseSize);
            std::streamsize len = ifs.gcount();
            if (len > 0)
            {
                write(buf.get(), len);
            }
        }

        return true;
    }

    void ByteArray::addCapacity(size_t size)
    {
        if (size == 0 || getCapacity() >= size)
        {
            return;
        }

        size_t old_cap = getCapacity();
        size = size - old_cap;
        size_t count = (size / m_baseSize) + (((size % m_baseSize) > old_cap) ? 1 : 0);

        Node *tmp = m_root;
        while (tmp->m_next)
        {
            tmp = tmp->m_next;
        }

        Node *first = nullptr;
        for (size_t i = 0; i < count; ++i)
        {
            tmp->m_next = new Node(m_baseSize);
            if (first == nullptr)
            {
                first = tmp->m_next;
            }
            tmp = tmp->m_next;
            m_capacity += m_baseSize;
        }

        if (old_cap == 0)
        {
            m_cur = first;
        }
    }
}
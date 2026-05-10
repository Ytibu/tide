#ifndef TIDE_BYTEARRAY_H__
#define TIDE_BYTEARRAY_H__

#include <memory>
#include <string>
#include <vector>

#include <sys/uio.h>

namespace tide
{
    class ByteArray
    {
    public:
        using ptr = std::shared_ptr<ByteArray>;
        struct Node
        {
            Node *m_next;
            char *m_ptr;
            size_t m_size;

            Node(size_t s);
            Node();
            ~Node();
        };

        ByteArray(size_t base_size = 4096);
        ~ByteArray();

        // write
        void writeFint8(int8_t value);
        void writeFuint8(uint8_t value);
        void writeFint16(int16_t value);
        void writeFuint16(uint16_t value);
        void writeFint32(int32_t value);
        void writeFuint32(uint32_t value);
        void writeFint64(int64_t value);
        void writeFuint64(uint64_t value);

        void writeInt32(int32_t value);
        void writeUint32(uint32_t value);
        void writeInt64(int64_t value);
        void writeUint64(uint64_t value);

        void writeFloat(float value);
        void writeDouble(double value);

        // length:int16 + data:string
        void writeStringF16(const std::string &value);
        // length:int32 + data:string
        void writeStringF32(const std::string &value);
        // length:int64 + data:string
        void writeStringF64(const std::string &value);
        // length:vint + data:string
        void writeStringVint(const std::string &value);

        void writeStringWithoutLength(const std::string &value);

        // read
        int8_t readFint8();
        uint8_t readFuint8();
        int16_t readFint16();
        uint16_t readFuint16();
        int32_t readFint32();
        uint32_t readFuint32();
        int64_t readFint64();
        uint64_t readFuint64();

        int32_t readInt32();
        uint32_t readUint32();
        int64_t readInt64();
        uint64_t readUint64();

        float readFloat();
        double readDouble();

        std::string readStringF16();
        std::string readStringF32();
        std::string readStringF64();
        std::string readStringVint();

        // 重置
        void clear();

        // 数据读写接口
        void write(const void *buf, size_t size);
        void read(void *buf, size_t size);
        void read(void *buf, size_t size, size_t position) const;

        // 位置相关
        void setPosition(size_t pos);
        size_t getPosition() const { return m_position; }

        // 获取基础大小和可读大小
        size_t getBaseSize() const { return m_baseSize; }
        size_t getReadSize() const { return m_size - m_position; }

        void setLittleEndian(bool val);
        bool isLittleEndian() const;

        // 数据读取或写入文件，方便错误展示与调试
        bool writeToFile(const std::string &name) const;
        bool readFromFile(const std::string &name);

        std::string toString() const;
        std::string toHexString() const;

        uint64_t getReadBufferSize(std::vector<iovec>& buffers, uint64_t len = ~0ull);
        uint64_t getReadBufferSize(std::vector<iovec>& buffers, uint64_t len, uint64_t position) const;
        uint64_t getWriteBuffers(std::vector<iovec>& buffers, uint64_t len);
        size_t getSize() const { return m_size; }

    private:
        // 容量获取与设置
        void addCapacity(size_t size);
        size_t getCapacity() const { return m_capacity - m_position; }

    private:
        Node *m_root;
        Node *m_cur;
        int8_t m_endian;   // 大小端
        size_t m_baseSize; // 原有长度
        size_t m_position; // 当前位置
        size_t m_capacity; // 容量
        size_t m_size;     // 长度
    };
}

#endif // TIDE_BYTEARRAY_H__
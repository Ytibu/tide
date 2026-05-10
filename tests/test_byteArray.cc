#include "../tide/bytearray.h"
#include "../tide/log.h"
#include "../tide/macro.h"
#include <vector>

static tide::Logger::ptr g_logger = TIDE_LOG_ROOT();


void test(){
#define xxx(type, len, write_fun, read_fun, base_len) {\
    std::vector<type> vec; \
    for(int i = 0; i < len; ++i) { \
        vec.push_back(rand()); \
    } \
    tide::ByteArray::ptr ba(new tide::ByteArray(base_len)); \
    for(auto& i : vec) { \
        ba->write_fun(i); \
    } \
    ba->setPosition(0); \
    for(size_t i = 0; i < vec.size(); ++i) { \
        type v = ba->read_fun(); \
        TIDE_ASSERT(v == vec[i]); \
    } \
    TIDE_ASSERT(ba->getReadSize() == 0); \
    TIDE_LOG_INFO(g_logger) << #write_fun "/" #read_fun \
                    " (" #type " ) len=" << len \
                    << " base_len=" << base_len \
                    << " size=" << ba->getSize(); \
}

    xxx(int8_t,  100, writeFint8, readFint8, 1);
    xxx(uint8_t, 100, writeFuint8, readFuint8, 1);
    xxx(int16_t,  100, writeFint16,  readFint16, 1);
    xxx(uint16_t, 100, writeFuint16, readFuint16, 1);
    xxx(int32_t,  100, writeFint32,  readFint32, 1);
    xxx(uint32_t, 100, writeFuint32, readFuint32, 1);
    xxx(int64_t,  100, writeFint64,  readFint64, 1);
    xxx(uint64_t, 100, writeFuint64, readFuint64, 1);

    xxx(int32_t,  100, writeInt32,  readInt32, 1);
    xxx(uint32_t, 100, writeUint32, readUint32, 1);
    xxx(int64_t,  100, writeInt64,  readInt64, 1);
    xxx(uint64_t, 100, writeUint64, readUint64, 1);
#undef xxx

#define xxx(type, len, write_fun, read_fun, base_len) {\
    std::vector<type> vec; \
    for(int i = 0; i < len; ++i) { \
        vec.push_back(rand()); \
    } \
    tide::ByteArray::ptr ba(new tide::ByteArray(base_len)); \
    for(auto& i : vec) { \
        ba->write_fun(i); \
    } \
    ba->setPosition(0); \
    for(size_t i = 0; i < vec.size(); ++i) { \
        type v = ba->read_fun(); \
        TIDE_ASSERT(v == vec[i]); \
    } \
    TIDE_ASSERT(ba->getReadSize() == 0); \
    TIDE_LOG_INFO(g_logger) << #write_fun "/" #read_fun \
                    " (" #type " ) len=" << len \
                    << " base_len=" << base_len \
                    << " size=" << ba->getSize(); \
    ba->setPosition(0); \
    TIDE_ASSERT(ba->writeToFile("/tmp/" #type "_" #len "-" #read_fun ".dat")); \
    tide::ByteArray::ptr ba2(new tide::ByteArray(base_len * 2)); \
    TIDE_ASSERT(ba2->readFromFile("/tmp/" #type "_" #len "-" #read_fun ".dat")); \
    ba2->setPosition(0); \
    TIDE_ASSERT(ba->toString() == ba2->toString()); \
    TIDE_ASSERT(ba->getPosition() == 0); \
    TIDE_ASSERT(ba2->getPosition() == 0); \
}
    xxx(int8_t,  100, writeFint8, readFint8, 1);
    xxx(uint8_t, 100, writeFuint8, readFuint8, 1);
    xxx(int16_t,  100, writeFint16,  readFint16, 1);
    xxx(uint16_t, 100, writeFuint16, readFuint16, 1);
    xxx(int32_t,  100, writeFint32,  readFint32, 1);
    xxx(uint32_t, 100, writeFuint32, readFuint32, 1);
    xxx(int64_t,  100, writeFint64,  readFint64, 1);
    xxx(uint64_t, 100, writeFuint64, readFuint64, 1);

    xxx(int32_t,  100, writeInt32,  readInt32, 1);
    xxx(uint32_t, 100, writeUint32, readUint32, 1);
    xxx(int64_t,  100, writeInt64,  readInt64, 1);
    xxx(uint64_t, 100, writeUint64, readUint64, 1);

#undef xxx
}

int main()
{
    test();
    return 0;
}
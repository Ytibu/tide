#ifndef TIDE_DB_DB_H__
#define TIDE_DB_DB_H__

#include <memory>
#include <string>
#include <cstdint>

namespace tide
{

    /**
     * @brief 数据库抽象层接口定义
     *
     * 本文件定义了一组纯虚接口，作为数据库操作的统一抽象层。
     * 这些接口让上层代码可以与不同数据库实现（如 SQLite、MySQL 等）解耦。
     */

    
    // 负责表示查询结果集的访问接口（读取列、行、类型等）
    class ISQLData
    {
    public:
        typedef std::shared_ptr<ISQLData> ptr;
        virtual ~ISQLData() {}

        virtual int getErrno() const = 0;
        virtual const std::string &getErrStr() const = 0;

        virtual int getDataCount() = 0;
        virtual int getColumnCount() = 0;
        virtual int getColumnBytes(int idx) = 0;
        virtual int getColumnType(int idx) = 0;
        virtual std::string getColumnName(int idx) = 0;

        virtual bool isNull(int idx) = 0;
        virtual int8_t getInt8(int idx) = 0;
        virtual uint8_t getUint8(int idx) = 0;
        virtual int16_t getInt16(int idx) = 0;
        virtual uint16_t getUint16(int idx) = 0;
        virtual int32_t getInt32(int idx) = 0;
        virtual uint32_t getUint32(int idx) = 0;
        virtual int64_t getInt64(int idx) = 0;
        virtual uint64_t getUint64(int idx) = 0;
        virtual float getFloat(int idx) = 0;
        virtual double getDouble(int idx) = 0;
        virtual std::string getString(int idx) = 0;
        virtual std::string getBlob(int idx) = 0;
        virtual time_t getTime(int idx) = 0;
        virtual bool next() = 0;
    };

    
    // 负责执行更新
    class ISQLUpdate
    {
    public:
        virtual ~ISQLUpdate() {}
        virtual int execute(const char *format, ...) = 0;
        virtual int execute(const std::string &sql) = 0;
        virtual int64_t getLastInsertId() = 0;
    };


    // 负责执行查询
    class ISQLQuery
    {
    public:
        virtual ~ISQLQuery() {}
        virtual ISQLData::ptr query(const char *format, ...) = 0;
        virtual ISQLData::ptr query(const std::string &sql) = 0;
    };


    // 预处理语句接口，封装 prepare / bind / execute / query
    class IStmt
    {
    public:
        typedef std::shared_ptr<IStmt> ptr;
        virtual ~IStmt() {}

        // bind 接口，支持按位置绑定和按名称绑定
        virtual int bindInt8(int idx, const int8_t &value) = 0;
        virtual int bindUint8(int idx, const uint8_t &value) = 0;
        virtual int bindInt16(int idx, const int16_t &value) = 0;
        virtual int bindUint16(int idx, const uint16_t &value) = 0;
        virtual int bindInt32(int idx, const int32_t &value) = 0;
        virtual int bindUint32(int idx, const uint32_t &value) = 0;
        virtual int bindInt64(int idx, const int64_t &value) = 0;
        virtual int bindUint64(int idx, const uint64_t &value) = 0;
        virtual int bindFloat(int idx, const float &value) = 0;
        virtual int bindDouble(int idx, const double &value) = 0;
        virtual int bindString(int idx, const char *value) = 0;
        virtual int bindString(int idx, const std::string &value) = 0;
        virtual int bindBlob(int idx, const void *value, int64_t size) = 0;
        virtual int bindBlob(int idx, const std::string &value) = 0;
        virtual int bindTime(int idx, const time_t &value) = 0;
        virtual int bindNull(int idx) = 0;

        /**
         * @brief 执行操作
         * 
         * @return int 
         */
        virtual int execute() = 0;

        /**
         * @brief 执行预处理语句，适用于 SELECT 等查询操作，返回结果集接口
         * 
         * @return int64_t 
         */
        virtual int64_t getLastInsertId() = 0;

        /**
         * @brief 执行查询操作，返回结果集接口
         * 
         * @return ISQLData::ptr 
         */
        virtual ISQLData::ptr query() = 0;

        virtual int getErrno() = 0;
        virtual std::string getErrStr() = 0;
    };


    // 事务接口，表示事务操作的开始/提交/回滚
    class ITransaction : public ISQLUpdate
    {
    public:
        typedef std::shared_ptr<ITransaction> ptr;
        virtual ~ITransaction() {};

        /**
         * @brief 开始一个事务接口
         * 
         * @return true 
         * @return false 
         */
        virtual bool begin() = 0;

        /**
         * @brief 提交事务接口
         * 
         * @return true 
         * @return false 
         */
        virtual bool commit() = 0;

        /**
         * @brief 回滚事务接口
         * 
         * @return true 
         * @return false 
         */
        virtual bool rollback() = 0;
    };


    // 聚合了更新和查询接口，并提供 prepare 与事务工厂
    class IDB : public ISQLUpdate, public ISQLQuery
    {
    public:
        typedef std::shared_ptr<IDB> ptr;
        virtual ~IDB() {}

        /**
         * @brief 创建预处理语句对象
         * 
         * @param stmt 
         * @return IStmt::ptr 
         */
        virtual IStmt::ptr prepare(const std::string &stmt) = 0;

        virtual int getErrno() = 0;
        virtual std::string getErrStr() = 0;

        /**
         * @brief 打开一个事务对象，默认不自动提交
         * 
         * @param auto_commit 默认不自动提交
         * @return ITransaction::ptr 
         */
        virtual ITransaction::ptr openTransaction(bool auto_commit = false) = 0;
    };

}

#endif
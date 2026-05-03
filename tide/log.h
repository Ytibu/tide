#ifndef __TIDE_LOG_H__
#define __TIDE_LOG_H__

#include <list>
#include <vector>
#include <memory>
#include <string>
#include <fstream>
#include <sstream>
#include <map>

#include <stdint.h>

#include "singleton.h"
#include "utils.h"
#include "thread.h"

#define TIDE_LOG_LEVEL(logger, level) \
    if (logger->getLevel() <= level) \
        tide::LogEventWrap(tide::LogEvent::ptr(new tide::LogEvent(logger, level, \
            __FILE__, __LINE__, 0, tide::GetThreadId(), tide::GetFiberId(), time(0)))).getSS()

#define TIDE_LOG_DEBUG(logger) TIDE_LOG_LEVEL(logger, tide::LogLevel::DEBUG)
#define TIDE_LOG_INFO(logger)  TIDE_LOG_LEVEL(logger, tide::LogLevel::INFO)
#define TIDE_LOG_WARN(logger)  TIDE_LOG_LEVEL(logger, tide::LogLevel::WARN)
#define TIDE_LOG_ERROR(logger) TIDE_LOG_LEVEL(logger, tide::LogLevel::ERROR)
#define TIDE_LOG_FATAL(logger) TIDE_LOG_LEVEL(logger, tide::LogLevel::FATAL)


#define TIDE_LOG_FMT_LEVEL(logger, level, fmt, ...) \
    if (logger->getLevel() <= level) \
        tide::LogEventWrap(tide::LogEvent::ptr(new tide::LogEvent(logger, level, \
            __FILE__, __LINE__, 0, tide::GetThreadId(), tide::GetFiberId(), time(0)))).getEvent()->format(fmt, __VA_ARGS__)

#define TIDE_LOG_FMT_DEBUG(logger, fmt, ...) TIDE_LOG_FMT_LEVEL(logger, tide::LogLevel::DEBUG, fmt, __VA_ARGS__)
#define TIDE_LOG_FMT_INFO(logger, fmt, ...)  TIDE_LOG_FMT_LEVEL(logger, tide::LogLevel::INFO, fmt, __VA_ARGS__)
#define TIDE_LOG_FMT_WARN(logger, fmt, ...)  TIDE_LOG_FMT_LEVEL(logger, tide::LogLevel::WARN, fmt, __VA_ARGS__)
#define TIDE_LOG_FMT_ERROR(logger, fmt, ...) TIDE_LOG_FMT_LEVEL(logger, tide::LogLevel::ERROR, fmt, __VA_ARGS__)
#define TIDE_LOG_FMT_FATAL(logger, fmt, ...) TIDE_LOG_FMT_LEVEL(logger, tide::LogLevel::FATAL, fmt, __VA_ARGS__)

#define TIDE_LOG_ROOT() tide::LoggerMgr::GetInstance()->getRoot()
#define TIDE_LOG_NAME(name) tide::LoggerMgr::GetInstance()->getLogger(name)

namespace tide
{
    // 前向声明
    
    class Logger;
    class LoggerManager;
    class LogEvent;
    class LogEventWrap;
    class LogFormatter;
    class LogAppender;
    class SpinkLock;

    /////////////////////////////////////////////////////////////////
    /**
     * LogLevel日志等级
     */
    class LogLevel
    {
    public:
        enum Level
        {
            UNKNOWN = 0,
            DEBUG = 1,
            INFO = 2,
            WARN = 3,
            ERROR = 4,
            FATAL = 5
        };
        static const char *ToString(LogLevel::Level level);
        static LogLevel::Level FromString(const std::string &str);
    };



    ///////////////////////////////////////////////////////////////
    /**
     * LogEvent 日志事件
     */
    class LogEvent
    {
    public:
        using ptr = std::shared_ptr<LogEvent>;
        LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level, const char *file, int32_t line, uint32_t elapse,
                 uint32_t threadId, uint32_t fiberId, uint64_t time);

        const char *getFile() const { return m_file; }
        int32_t getLine() const { return m_line; }
        uint32_t getElapse() const { return m_elapse; }
        uint32_t getThreadId() const { return m_threadId; }
        uint32_t getFiberId() const { return m_fiberId; }
        uint64_t getTime() const { return m_time; }
        std::string getContent() const { return m_ss.str(); }
        std::shared_ptr<Logger> getLogger() const { return m_logger; }
        LogLevel::Level getLevel() const { return m_level; }
        std::string getThreadName() const{return "m_threadName";}

        std::stringstream& getSS() {return m_ss;}

        void format(const char *fmt, ...);
        void format(const char *fmt, va_list al);

    private:
        const char *m_file = nullptr; // 文件名
        int32_t m_line = 0;           // 行号
        uint32_t m_elapse = 0;        // 程序启动到现在的时间
        uint32_t m_threadId = 0;      // 线程id
        uint32_t m_fiberId = 0;       // 协程id
        uint64_t m_time;              // 时间戳
        std::string m_threadName;     // 线程名称
        std::stringstream m_ss;

        LogLevel::Level m_level;
        std::shared_ptr<Logger> m_logger;
    };

    class LogEventWrap
    {
        public:
            LogEventWrap(LogEvent::ptr e);
            ~LogEventWrap();
            std::ostream& getSS();
            LogEvent::ptr getEvent() const { return m_event;}
        private:
            LogEvent::ptr m_event;
    };


    ////////////////////////////////////////////////////////////////
    /**
     * LogFormatter 日志格式器
     */
    class LogFormatter
    {
    public:
        using ptr = std::shared_ptr<LogFormatter>;

        LogFormatter(const std::string &pattern);
        std::string format(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event);

    public:
        class FormatItem
        {
        public:
            using ptr = std::shared_ptr<FormatItem>;

            virtual ~FormatItem() {}
            virtual void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) = 0;
        };
        void init();
        bool isError() const { return m_error; }
        const std::string getPattern() const { return m_pattern; }

    private:
        std::string m_pattern;
        std::vector<FormatItem::ptr> m_items;
        bool m_error = false;
    };

    ////////////////////////////////////////////////////////////////////
    /**
     * 日志输出地址
     */
    class LogAppender
    {
        friend class Logger;
    public:
        using MutexType = Mutex;
        using ptr = std::shared_ptr<LogAppender>;

        virtual ~LogAppender(){}
        virtual void log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) = 0;

        void setLevel(LogLevel::Level val) { m_level = val; }
        LogLevel::Level getLevel() const { return m_level; }

        void setFormatter(LogFormatter::ptr val);
        LogFormatter::ptr getFormatter();
        
        virtual std::string toYamlString() = 0;

    protected:
        LogLevel::Level m_level = LogLevel::DEBUG;
        LogFormatter::ptr m_formatter;
        bool m_hasFormatter  = false;
        MutexType m_mutex;
    };
    /**
     * 日志终端输出 继承 日志输出
     */
    class StdoutLogAppender : public LogAppender
    {
    public:
        using ptr = std::shared_ptr<StdoutLogAppender>;
        virtual void log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override;
        virtual std::string toYamlString()  override;
    };

    /**
     * 日志文件输出 继承 日志输出
     */
    class FileLogAppender : public LogAppender
    {
    public:
        using ptr = std::shared_ptr<FileLogAppender>;
        FileLogAppender(const std::string &filename);
        virtual void log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override;
        bool reopen();
        virtual std::string toYamlString() override;

    private:
        std::string m_fileName;
        std::ofstream m_filestream;
        uint64_t m_lastTime;
    };

    ///////////////////////////////////////////
    /**
     * Logger 日志器
     */
    class Logger : public std::enable_shared_from_this<Logger>
    {
    friend class LoggerManager;
    public:
        using MutexType = Mutex;
        using ptr = std::shared_ptr<Logger>;

        Logger(const std::string &name = "root");
        void log(LogLevel::Level level, LogEvent::ptr event);

        void debug(LogEvent::ptr event);
        void info(LogEvent::ptr event);
        void warn(LogEvent::ptr event);
        void error(LogEvent::ptr event);
        void fatal(LogEvent::ptr event);

        void addAppender(LogAppender::ptr appender);
        void delAppender(LogAppender::ptr appender);
        void clearAppenders();

        void setLevel(LogLevel::Level val);
        LogLevel::Level getLevel() const { return m_level; }

        void setFormatter(const std::string& val);
        void setFormatter(LogFormatter::ptr val);
        LogFormatter::ptr getFormatter();

        const std::string &getName() const { return m_name; }

        std::string toYamlString();

    private:
        std::string m_name;                      // 日志名称
        LogLevel::Level m_level;                 // 日志级别
        MutexType m_mutex;
        std::list<LogAppender::ptr> m_appenders; // Appender集合
        LogFormatter::ptr m_formatter;           // 默认Formatter
        Logger::ptr m_root;
    };


    /**
     * LoggerManager 日志管理器
     */
    class LoggerManager
    {
        public:
            using MutexType = Mutex;
            LoggerManager();
            void init();

            Logger::ptr getLogger(const std::string& name);
            Logger::ptr getRoot() const;

            std::string toYamlString();

        private:
            std::map<std::string, Logger::ptr> m_loggers;
            Logger::ptr m_root;
            MutexType m_mutex;
    };
    using LoggerMgr = tide::SingletonPtr<LoggerManager>;

} // namespace tide
#endif // __TIDE_LOG_H__

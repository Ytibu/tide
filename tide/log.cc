#include "log.h"

#include <iostream>
#include <sstream>
#include <utility>
#include <tuple>
#include <string>
#include <map>
#include <set>
#include <functional>
#include <memory>

#include <stdarg.h>

#include "config.h"

namespace tide
{

    //////////////////////////////////////////////////////////////////////
    /**
     * LogEvent 日志事件
     */
    LogEvent::LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level, const char *file,
                       int32_t line, uint32_t elapse, uint32_t threadId, uint32_t fiberId, uint64_t time)
        : m_file(file), m_line(line), m_elapse(elapse), m_threadId(threadId),
          m_fiberId(fiberId), m_time(time), m_level(level), m_logger(logger)
    {
    }

    LogEventWrap::LogEventWrap(LogEvent::ptr e)
        : m_event(e)
    {
    }

    LogEventWrap::~LogEventWrap()
    {
        if (m_event)
            m_event->getLogger()->log(m_event->getLevel(), m_event);
    }

    std::ostream &LogEventWrap::getSS()
    {
        return m_event->getSS();
    }

    void LogEvent::format(const char *fmt, ...)
    {
        va_list al;
        va_start(al, fmt);
        format(fmt, al);
        va_end(al);
    }

    void LogEvent::format(const char *fmt, va_list al)
    {
        char *buf = nullptr;
        int len = vasprintf(&buf, fmt, al);
        if (len != -1)
        {
            m_ss << std::string(buf, len);
            free(buf);
        }
    }

    /////////////////////////////////////////////////////////////////////////
    /**
     * LogLevel 日志等级
     */
    const char *LogLevel::ToString(LogLevel::Level level)
    {
        switch (level)
        {
#define XX(name)         \
    case LogLevel::name: \
        return #name;
            break;
            XX(DEBUG)
            XX(INFO)
            XX(WARN)
            XX(ERROR)
            XX(FATAL)
#undef XX
        default:
            return "UNKNOWN";
        }
        return "UNKNOWN";
    }

    LogLevel::Level LogLevel::FromString(const std::string &str)
    {
#define XX(name) \
        if(str == #name){  \
            return LogLevel::name; \
        }
        XX(DEBUG);
        XX(INFO);
        XX(WARN);
        XX(ERROR);
        XX(FATAL);
        return LogLevel::UNKNOWN;
#undef XX
    }

    ///////////////////////////////////////////////////////////////////////////
    /**
     * LogFormatter 日志格式输出
     */
    LogFormatter::LogFormatter(const std::string &pattern)
        : m_pattern(pattern)
    {
        init();
    }

    std::string LogFormatter::format(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event)
    {
        std::stringstream ss;
        for (auto &it : m_items)
        {
            it->format(ss, logger, level, event);
        }
        return ss.str();
    }

    /**
     * FormatItem 格式节点
     */
    LogFormatter::FormatItem::FormatItem(const std::string &fmt)
    {
    }
    LogFormatter::FormatItem::~FormatItem()
    {
    }

    /**
     * 格式节点继承器
     */
    class MessageFormatItem : public LogFormatter::FormatItem
    {
    public:
        MessageFormatItem(const std::string &str = "") {}
        void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override
        {
            os << event->getContent();
        }
    };

    class LevelFormatItem : public LogFormatter::FormatItem
    {
    public:
        LevelFormatItem(const std::string &str = "")
        {
        }
        void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override
        {
            os << LogLevel::ToString(level);
        }
    };

    class ElapseFormatItem : public LogFormatter::FormatItem
    {
    public:
        ElapseFormatItem(const std::string &str = "")
        {
        }
        void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override
        {
            os << event->getElapse();
        }
    };

    class NameFormatItem : public LogFormatter::FormatItem
    {
    public:
        NameFormatItem(const std::string &str = "")
        {
        }
        void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override
        {
            os << event->getLogger()->getName();
        }
    };

    class ThreadIdFormatItem : public LogFormatter::FormatItem
    {
    public:
        ThreadIdFormatItem(const std::string &str = "")
        {
        }
        void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override
        {
            os << event->getThreadId();
        }
    };

    class FiberIdFormatItem : public LogFormatter::FormatItem
    {
    public:
        FiberIdFormatItem(const std::string &str = "")
        {
        }
        void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override
        {
            os << event->getFiberId();
        }
    };

    class ThreadNameFormatItem : public LogFormatter::FormatItem
    {
    public:
        ThreadNameFormatItem(const std::string &str = "") {}
        void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override
        {
            os << event->getThreadName();
        }
    };

    class DateTimeFormatItem : public LogFormatter::FormatItem
    {
    public:
        DateTimeFormatItem(const std::string &format = "%Y-%m-%d %H:%M:%S")
            : m_format(format)
        {
            if (m_format.empty())
            {
                m_format = "%Y-%m-%d %H:%M:%S";
            }
        }
        void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override
        {
            struct tm tm;
            time_t time = event->getTime();
            localtime_r(&time, &tm);
            char buf[64];
            strftime(buf, sizeof(buf), m_format.c_str(), &tm);
            os << buf;
        }

    private:
        std::string m_format;
    };

    class FilenameFormatItem : public LogFormatter::FormatItem
    {
    public:
        FilenameFormatItem(const std::string &str = "")
        {
        }
        void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override
        {
            os << event->getFile();
        }
    };

    class LineFormatItem : public LogFormatter::FormatItem
    {
    public:
        LineFormatItem(const std::string &str = "")
        {
        }
        void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override
        {
            os << event->getLine();
        }
    };

    class NewLineFormatItem : public LogFormatter::FormatItem
    {
    public:
        NewLineFormatItem(const std::string &str = "")
        {
        }
        void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override
        {
            os << std::endl;
        }
    };

    class StringFormatItem : public LogFormatter::FormatItem
    {
    public:
        StringFormatItem(const std::string &str)
            : m_string(str)
        {
        }
        void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override
        {
            os << m_string;
        }

    private:
        std::string m_string;
    };

    class TabFormatItem : public LogFormatter::FormatItem
    {
    public:
        TabFormatItem(const std::string & = "")
        {
        }
        void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override
        {
            os << "\t";
        }
    };

    /**
     * 日志输出
     */
    Logger::Logger(const std::string &name)
        : m_name(name), m_level(LogLevel::DEBUG)
    {
        m_formatter.reset(new LogFormatter("%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"));

        if (m_name == "root")
        {
            m_appenders.push_back(LogAppender::ptr(new StdoutLogAppender));
        }
    }

    void Logger::addAppender(LogAppender::ptr appender)
    {
        if (!appender->getLogFormatter())
        {
            appender->setFormatter(m_formatter);
        }
        m_appenders.push_back(appender);
    }
    void Logger::delAppender(LogAppender::ptr appender)
    {
        for (auto it = m_appenders.begin(); it != m_appenders.end(); ++it)
        {
            if (*it == appender)
            {
                m_appenders.erase(it);
                break;
            }
        }
    }

    void Logger::clearAppenders()
    {
        m_appenders.clear();
    }

    void Logger::setFormatter(const std::string &val)
    {
        tide::LogFormatter::ptr new_val(new tide::LogFormatter(val));
        if (new_val->isError())
        {
            std::cout << "Logger setFormatter name=" << m_name << " value=" << val << " invalid formatter" << std::endl;
            return;
        }
        m_formatter = new_val;
    }
    void Logger::setFormatter(LogFormatter::ptr val)
    {
        m_formatter = val;
    }

    void Logger::log(LogLevel::Level level, LogEvent::ptr event)
    {
        if (level >= m_level)
        {
            auto self = shared_from_this();
            if (!m_appenders.empty())
            {
                for (auto &it : m_appenders)
                {
                    it->log(self, level, event);
                }
            }
            else
            {
                if (m_root)
                {
                    m_root->log(level, event);
                }
            }
        }
    }

    void Logger::debug(LogEvent::ptr event)
    {
        log(LogLevel::DEBUG, event);
    }
    void Logger::info(LogEvent::ptr event)
    {
        log(LogLevel::INFO, event);
    }

    void Logger::warn(LogEvent::ptr event)
    {
        log(LogLevel::WARN, event);
    }
    void Logger::error(LogEvent::ptr event)
    {
        log(LogLevel::ERROR, event);
    }

    void Logger::fatal(LogEvent::ptr event)
    {
        log(LogLevel::FATAL, event);
    }

    ////////////////////////////////////////
    /**
     * 日志输出地址
     */

    LogAppender::~LogAppender()
    {
    }

    /**
     * 日志终端打印
     */
    void StdoutLogAppender::log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event)
    {
        if (level >= logger->getLevel())
        {
            std::cout << getLogFormatter()->format(logger, level, event);
        }
    }

    /**
     * 日志写入文件
     */
    FileLogAppender::FileLogAppender(const std::string &filename)
        : m_fileName(filename)
    {
        reopen();
    }

    //!!!!!!!!!!!!!!! m_level m_formatter
    void FileLogAppender::log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event)
    {
        if (level >= logger->getLevel())
        {
            m_filestream << getLogFormatter()->format(logger, level, event);
        }
    }

    bool FileLogAppender::reopen()
    {
        if (m_filestream)
        {
            m_filestream.close();
        }
        m_filestream.open(m_fileName);
        return static_cast<bool>(m_filestream);
    }

    ///////////////////////////////////////////////////

    /**
     * LoggerManager 日志管理器
     */
    LoggerManager::LoggerManager()
    {
        m_root.reset(new Logger);
        m_root->addAppender(LogAppender::ptr(new StdoutLogAppender));
        init();
    }
    Logger::ptr LoggerManager::getLogger(const std::string &name)
    {
        auto it = m_loggers.find(name);
        if (it != m_loggers.end())
        {
            return it->second;
        }
        Logger::ptr logger(new Logger(name));
        logger->m_root = m_root;
        m_loggers[name] = logger;
        return logger;
    }

    struct LogAppenderDefine
    {
        int type = 0; // 1 File 2 Stdout
        LogLevel::Level level = LogLevel::DEBUG;
        std::string formatter;
        std::string file;

        bool operator==(const LogAppenderDefine &oth) const
        {
            return type == oth.type && level == oth.level && formatter == oth.formatter && file == oth.file;
        }
    };

    struct LogDefine
    {
        std::string name;
        LogLevel::Level level = LogLevel::DEBUG;
        std::string formatter;
        std::vector<LogAppenderDefine> appenders;

        bool operator==(const LogDefine &oth) const
        {
            return name == oth.name && level == oth.level && formatter == oth.formatter && appenders == oth.appenders;
        }

        bool operator<(const LogDefine &oth) const
        {
            return name < oth.name;
        }
    };


    template<>
    class LexicalCast<std::string, std::set<LogDefine>>
    {
    public:
        std::set<LogDefine> operator()(const std::string &v)
        {
            YAML::Node node = YAML::Load(v);
            std::set<LogDefine> se;
            std::stringstream ss;
            for(size_t i = 0; i < se.size(); ++i){
                auto n = node[i];
                if(n["name"].IsDefined()){
                    std::cout << "log config error: name is null, " << n << std::endl;
                    continue;
                }
                LogDefine ld;
                ld.name = n["name"].as<std::string>();
                ld.level = LogLevel::FromString(n["level"].IsDefined() ? n["level"].as<std::string>() : "");
                if(n["formatter"].IsDefined()){
                    ld.formatter = n["formatter"].as<std::string>();
                }
                if(n["appenders"].IsDefined()){
                    for(size_t x = 0; x < n["appenders"].size(); ++x){
                        auto a = n["appenders"][x];
                        
                        if(!a["type"].IsDefined()){
                            std::cout << "log config error: appender type is null, " << a << std::endl;
                            continue;
                        }
                        std::string type = a["type"].as<std::string>();
                        LogAppenderDefine lad;
                        if(type == "FileLogAppender"){
                            lad.type = 1;
                            if(!a["file"].IsDefined()){
                                std::cout << "log config error: appender file is null, " << a << std::endl;
                                continue;
                            }
                            lad.file = a["file"].as<std::string>();
                            if(n["formatter"].IsDefined()){
                                lad.formatter = n["formatter"].as<std::string>();
                            }
                        }else if(type == "StdoutLogAppender"){
                            lad.type = 2;
                        }else{
                            std::cout << "log config error: appender type is invalid, " << a << std::endl;
                            continue;
                        }

                        ld.appenders.push_back(lad);
                    }
                }
            }
            return se;
        }
    };

    template<>
    class LexicalCast<std::set<LogDefine>, std::string>
    {
    public:
        std::string operator()(const std::set<LogDefine> &v){
            YAML::Node node;
            for(auto &i : v){
                YAML::Node n;
                if(!n["name"].IsDefined()){
                    std::cout << "log config error: name is null, " << n << std::endl;
                    continue;
                }
                LogDefine ld;
                ld.name = n["name"].as<std::string>();
                n["level"] = LogLevel::ToString(i.level);
                n["formatter"] = i.formatter;
                for(auto &a : i.appenders){
                    YAML::Node na;
                    na["type"] = a.type;
                    na["level"] = LogLevel::ToString(a.level);
                    na["formatter"] = a.formatter;
                    na["file"] = a.file;
                    n["appenders"].push_back(na);
                }
                node.push_back(n);
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    tide::ConfigVar<std::set<LogDefine>> g_log_defines = tide::Config::Lookup("logs", std::set<LogDefine>(), "logs config");

    struct LogIniter
    {
        LogIniter()
        {
            g_log_defines->addListener(0xF1E231, [](const std::set<LogDefine> &old_value, const std::set<LogDefine> &new_value){
                TIDE_LOG_INFO(TIDE_LOG_ROOT()) << "on_logger_conf_changed";
                for(auto &i : new_value){
                    auto it = old_value.find(i);
                    tide::Logger::ptr logger;
                    if(it == old_value.end()){
                        // 新增Logger
                        logger.reset(new tide::Logger(i.name));
                    }else{
                        // 修改Logger
                        if(!(i == *it)){
                        logger = TIDE_LOG_NAME(i.name);
                        }
                    }

                    logger->setLevel(i.level);
                    if(!i.formatter.empty()){
                        logger->setFormatter(i.formatter);
                    }

                    logger->clearAppenders();
                    for(auto &a : i.appenders){
                        tide::LogAppender::ptr appender;
                        if(a.type == 1){
                            appender.reset(new tide::FileLogAppender(a.file));
                        }else if(a.type == 2){
                            appender.reset(new tide::StdoutLogAppender);
                        }
                        appender->setLevel(a.level);
                        logger->addAppender(appender);
                    }

                }

                for(auto &i : old_value){
                    auto it = new_value.find(i);
                    if(it == new_value.end()){
                        // 假删除Logger：设置到无法触发状态
                        auto logger =  TIDE_LOG_NAME(i.name);
                        logger->setLevel((LogLevel::Level)100);
                        logger->clearAppenders();
                    }
                } });
        }
    };

    static LogIniter __log_init;

    void LoggerManager::init()
    {
        m_root = getLogger("root");
    }

    ////////////////////////////////////////////////
    ////////////////////////////////////////////////

    void LogFormatter::init()
    {
        std::vector<std::tuple<std::string, std::string, int>> vec;
        std::string nstr;
        for (size_t i = 0; i < m_pattern.size(); ++i)
        {
            if (m_pattern[i] != '%')
            {
                nstr.append(1, m_pattern[i]);
                continue;
            }

            if ((i + 1) < m_pattern.size())
            {
                if (m_pattern[i + 1] == '%')
                {
                    nstr.append(1, '%');
                    continue;
                }
            }

            size_t n = i + 1;
            int fmt_status = 0;
            size_t fmt_begin = 0;

            std::string str;
            std::string fmt;
            while (n < m_pattern.size())
            {
                if (!fmt_status && (!isalpha(m_pattern[n]) && m_pattern[n] != '{' && m_pattern[n] != '}'))
                {
                    str = m_pattern.substr(i + 1, n - i - 1);
                    break;
                }

                if (fmt_status == 0)
                {
                    if (m_pattern[n] == '{')
                    {
                        str = m_pattern.substr(i + 1, n - i - 1);
                        fmt_status = 1; // 解析格式
                        fmt_begin = n;
                        ++n;
                        continue;
                    }
                }
                else if (fmt_status == 1)
                {
                    if (m_pattern[n] == '}')
                    {
                        fmt = m_pattern.substr(fmt_begin + 1, n - fmt_begin - 1);
                        fmt_status = 0;
                        ++n;
                        break;
                    }
                }
                ++n;

                if (n == m_pattern.size())
                {
                    if (str.empty())
                    {
                        str = m_pattern.substr(i + 1);
                    }
                }
            }

            if (fmt_status == 0)
            {
                if (!nstr.empty())
                {
                    vec.push_back(std::make_tuple(nstr, std::string(), 0));
                    nstr.clear();
                }
                vec.push_back(std::make_tuple(str, fmt, 1));
                i = n - 1;
            }
            else if (fmt_status == 1)
            {
                std::cout << "pattern parse error: " << m_pattern << " - " << m_pattern.substr(i) << std::endl;
                m_error = true;
                vec.push_back(std::make_tuple("<<pattern_error>>", fmt, 0));
            }
        }

        if (!nstr.empty())
            vec.push_back(std::make_tuple(nstr, std::string(), 0));

        static std::map<std::string, std::function<LogFormatter::FormatItem::ptr(const std::string &str)>> s_format_items = {
#define XX(str, C)                                                                             \
    {                                                                                          \
        #str, [](const std::string &fmt) { return LogFormatter::FormatItem::ptr(new C(fmt)); } \
    }
            XX(m, MessageFormatItem),
            XX(p, LevelFormatItem),
            XX(r, ElapseFormatItem),
            XX(c, NameFormatItem),
            XX(t, ThreadIdFormatItem),
            XX(n, NewLineFormatItem),
            XX(d, DateTimeFormatItem),
            XX(f, FilenameFormatItem),
            XX(l, LineFormatItem),
            XX(T, TabFormatItem),
            XX(F, FiberIdFormatItem),
            XX(N, ThreadNameFormatItem)
#undef XX
        };
        for (auto &i : vec)
        {
            if (std::get<2>(i) == 0)
            {
                m_items.push_back(FormatItem::ptr(new StringFormatItem(std::get<0>(i))));
            }
            else
            {
                auto it = s_format_items.find(std::get<0>(i));
                if (it == s_format_items.end())
                {
                    m_items.push_back(FormatItem::ptr(new StringFormatItem("<<error_format %" + std::get<0>(i) + ">>")));
                    m_error = true;
                }
                else
                {
                    m_items.push_back(it->second(std::get<1>(i)));
                }
            }
        }
    }

} // namespace tide

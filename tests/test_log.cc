#include <memory>
#include <iostream>

#include "../tide/utils.h"
#include "../tide/log.h"

int main() {
    
    tide::Logger::ptr logger(new tide::Logger);
    logger->addAppender(tide::LogAppender::ptr(new tide::StdoutLogAppender));

    tide::FileLogAppender::ptr file_appender(new tide::FileLogAppender("./log/log.txt"));
    tide::LogFormatter::ptr fmt(new tide::LogFormatter("%d%T%p%T%m%n"));
    file_appender->setFormatter(fmt);
    file_appender->setLevel(tide::LogLevel::ERROR);

    logger->addAppender(file_appender);

    // tide::LogEvent::ptr event(new tide::LogEvent(
    //     logger,
    //     tide::LogLevel::DEBUG,
    //     __FILE__,
    //     __LINE__,
    //     0, // elapse
    //     tide::GetThreadId(),
    //     2, // fiber id or whatever the 6th param is
    //     time(0)
    // ));


    //logger->log(tide::LogLevel::DEBUG, event);

    TIDE_LOG_DEBUG(logger) << "!Hello, World!";
    TIDE_LOG_INFO(logger) <<  "!!Hello, World!";
    TIDE_LOG_WARN(logger) <<  "!!!Hello, World!";
    TIDE_LOG_ERROR(logger) << "!!!!Hello, World!";
    TIDE_LOG_FATAL(logger) << "!!!!!Hello, World!";

    TIDE_LOG_FMT_ERROR(logger, "Hello, %s!", "World");
    TIDE_LOG_FMT_FATAL(logger, "Hello, %s!", "World");

    auto log = tide::LoggerMgr::GetInstance()->getLogger("xx");
    TIDE_LOG_INFO(log) << "Hello, LoggerManager!";

    return 0;
}
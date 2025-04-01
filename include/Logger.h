#ifndef __LOGGER_H__
#define __LOGGER_H__

#include <string>
#include <cstdlib>
#include "noncopyable.h"

// LOG_INFO(%s,%d,arg1,arg2)
#define LOG_INFO(logmsgformat, ...)                       \
    do                                                    \
    {                                                     \
        Logger &logger = Logger::instance();               \
        logger.setLogLevel(INFO);                         \
        char buf[1024] = {0};                             \
        snprintf(buf, 1024, logmsgformat, ##__VA_ARGS__); \
        logger.log(buf);                                  \
    } while (0)

#define LOG_ERROR(logmsgformat, ...)                      \
    do                                                    \
    {                                                     \
        Logger &logger = Logger::instance();               \
        logger.setLogLevel(ERROR);                        \
        char buf[1024] = {0};                             \
        snprintf(buf, 1024, logmsgformat, ##__VA_ARGS__); \
        logger.log(buf);                                  \
    } while (0)

#define LOG_FATAL(logmsgformat, ...)                      \
    do                                                    \
    {                                                     \
        Logger &logger = Logger::instance();              \
        logger.setLogLevel(FATAL);                        \
        char buf[1024] = {0};                             \
        snprintf(buf, 1024, logmsgformat, ##__VA_ARGS__); \
        logger.log(buf);                                  \
        std::exit(-1);                                    \
    } while (0)

#ifdef MUDEBUG
#define LOG_DEBUG(logmsgformat, ...)                      \
    do                                                    \
    {                                                     \
        Logger &logger = Logger::instance();               \
        logger.setLogLevel(DEBUG);                        \
        char buf[1024] = {0};                             \
        snprintf(buf, 1024, logmsgformat, ##__VA_ARGS__); \
        logger.log(buf);                                  \
    } while (0)
#else
#define LOG_DEBUG(logmsgformat, ...)
#endif

enum LogLevel
{
    INFO,
    ERROR,
    FATAL,
    DEBUG
};

class Logger:noncopyable
{
    public:
        static Logger &instance();
        //记录日志
        void log(std::string msg);
        //设置日志等级
        void setLogLevel(int level);

    private:
        int logLevel_;
        Logger(){}
};

#endif
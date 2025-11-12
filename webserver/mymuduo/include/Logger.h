#pragma once

#include<string>

#include "noncopyable.h"
#include "TimeStamp.h"


#define LOG_INFO(msg, ...)\
    do\
    {\
        Logger& log = Logger::instance();\
        log.setlogLevel(INFO); \
        char buf[1024] = {0};\
        snprintf(buf, 1024, msg, ##__VA_ARGS__); \
        log.logmsg(buf); \
    } while (0)
    


#define LOG_ERROR(msg, ...)\
do\
{\
    Logger& log = Logger::instance();\
    log.setlogLevel(ERROR); \
    char buf[1024] = {0};\
    snprintf(buf, 1024, msg, ##__VA_ARGS__); \
    log.logmsg(buf); \
} while (0)

#define LOG_FATAL(msg, ...)\
do\
{\
    Logger& log = Logger::instance();\
    log.setlogLevel(FATAL); \
    char buf[1024] = {0};\
    snprintf(buf, 1024, msg, ##__VA_ARGS__); \
    log.logmsg(buf); \
    exit(-1);\
} while (0)
    
#ifdef DEBBUG
#define LOG_DEBUG(msg, ...)\
do\
{\
    Logger& log = Logger::instance();\
    log.setlogLevel(DEBUG); \
    char buf[1024] = {0};\
    snprintf(buf, 1024, msg, ##__VA_ARGS__); \
    log.logmsg(buf); \
} while (0)
#else
#define LOG_DEBUG(msg, ...)
#endif

enum{
    INFO,
    ERROR,
    FATAL,
    DEBUG,
};

class Logger : noncopyable
{
private:
    int logLevel_;
public:
    static Logger& instance();
    void setlogLevel(int level);
    void logmsg(std::string msg); 
};


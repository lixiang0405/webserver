//#include<string>
#include "Logger.h"
#include "TimeStamp.h"


#include<iostream>

Logger& Logger::instance(){
    static Logger log;
    return log;
}

void Logger::setlogLevel(int level){
    logLevel_ = level;
}

void Logger::logmsg(std::string msg){
    switch (logLevel_)
    {
    case INFO:
        std::cout << "[INFO]";
        /* code */
        break;
    case FATAL:
        std::cout << "[FATAL]";
        break;
    case ERROR:
        std::cout << "[ERROR]";
        break;
    case DEBUG:
        std::cout << "[DEBUG]";
        break;
        
    default:
        break;
    }

    std::cout << TimeStamp::now().to_string() << ": " << msg << std::endl;
} 

// #include<iostream>
// #include<string>
// int main(){
//     char msg[128] = "effsef";
//     LOG_INFO("%s", msg);
//     return 0;
// }
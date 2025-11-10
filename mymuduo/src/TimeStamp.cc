#include "TimeStamp.h"

TimeStamp::TimeStamp() : microSecondSinceEpoch_(0){}

TimeStamp::TimeStamp(int64_t t) : microSecondSinceEpoch_(t){}

TimeStamp TimeStamp::now(){
    return TimeStamp(time(NULL));
}

std::string TimeStamp::to_string() const{
    char buf[128] = {};
    tm *t = localtime(&microSecondSinceEpoch_);
    snprintf(buf, 128, "%4d/%02d/%02d %02d:%02d:%02d", 
    t -> tm_year + 1900,
    t -> tm_mon + 1,
    t -> tm_mday,
    t -> tm_hour,
    t -> tm_min, 
    t -> tm_sec);
    return buf;
}

// #include<iostream>
// int main(){
//     std::cout << TimeStamp::now().to_string() << std::endl;
//     return 0;
// }
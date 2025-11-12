#pragma once

#include<string>
#include<iostream>

class TimeStamp
{
private:
    /* data */
    int64_t microSecondSinceEpoch_;
public:
    TimeStamp(/* args */);
    explicit TimeStamp(int64_t t);
    static TimeStamp now();
    std::string to_string() const;
};
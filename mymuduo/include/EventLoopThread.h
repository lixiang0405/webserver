#pragma once

#include<mutex>
#include<condition_variable>
#include<functional>
#include<string>

#include "noncopyable.h"
#include "Thread.h"
class EventLoop;

class EventLoopThread : noncopyable
{
    using ThreadInitCallback = std::function<void(EventLoop*)>; 
private:
    void threadFunc();

    EventLoop *loop_;
    Thread thread_;
    bool exiting_;
    
    std::mutex mutex_;
    std::condition_variable cond_;

    ThreadInitCallback callback_;
public:
    EventLoopThread(const ThreadInitCallback &cb = ThreadInitCallback(), const std::string &name = std::string());
    ~EventLoopThread();

    EventLoop *startLoop();
};
